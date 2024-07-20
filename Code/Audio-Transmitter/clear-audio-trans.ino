#include "BLEDevice.h"
#include <driver/i2s.h>

static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
static BLEUUID charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLEAdvertisedDevice* myDevice;
BLERemoteCharacteristic* pRemoteCharacteristic;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {}
  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID: ");
    Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  // I2S configuration
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
    .sample_rate = 16000,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
    .communication_format = I2S_COMM_FORMAT_I2S_MSB,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false,
  };
  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);

  i2s_pin_config_t pin_config = {
    .bck_io_num = 26,
    .ws_io_num = 25,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = 22
  };
  i2s_set_pin(I2S_NUM_0, &pin_config);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  if (connected) {
    uint8_t data[6144];
    size_t bytes_read;
    esp_err_t result = i2s_read(I2S_NUM_0, (char *)data, sizeof(data), &bytes_read, portMAX_DELAY);
    if (result == ESP_OK && bytes_read > 0) {
      // BLE characteristic write has a limit on the maximum payload size, typically 512 bytes
      size_t offset = 0;
      while (bytes_read > 0) {
        size_t chunk_size = bytes_read > 512 ? 512 : bytes_read;
        pRemoteCharacteristic->writeValue(data + offset, chunk_size);
        offset += chunk_size;
        bytes_read -= chunk_size;
      }
      Serial.print("Sent audio data: ");
      Serial.println(offset);
    }
  } else if (doScan) {
    BLEDevice::getScan()->start(0);
  }

  delay(50);  // Reduced delay for lower latency
}