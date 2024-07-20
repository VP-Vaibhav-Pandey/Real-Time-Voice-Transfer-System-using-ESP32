#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <driver/i2s.h>
#include <math.h>

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

bool deviceConnected = false;
uint8_t audioData[6144]; // Buffer to hold the complete audio data
size_t receivedBytes = 0;

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// Volume Normalization Parameters
float targetPeak = 30000.0; // Target peak level for the audio signal
// Gain Control Parameter
float gain = 1.0; // Initial gain

// Noise Reduction Parameters
int16_t noiseProfile[6144] = {0}; // Noise profile to be subtracted from the audio signal
bool noiseProfileCaptured = false;
const int noiseCaptureDuration = 5000; // Duration to capture noise profile in milliseconds
unsigned long noiseCaptureStartTime = 0;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("Client disconnected");
  }
};

class MyCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    std::string rxData = pCharacteristic->getValue();
    size_t length = rxData.length();

    if (receivedBytes + length <= sizeof(audioData)) {
      memcpy(audioData + receivedBytes, rxData.data(), length);
      receivedBytes += length;
    }

    if (receivedBytes >= sizeof(audioData)) {
      int16_t *audioSamples = (int16_t *)audioData;

      // Capture noise profile if not captured yet
      if (!noiseProfileCaptured) {
        if (millis() - noiseCaptureStartTime < noiseCaptureDuration) {
          for (int i = 0; i < receivedBytes / 2; i++) {
            noiseProfile[i] += audioSamples[i];
          }
        } else {
          for (int i = 0; i < receivedBytes / 2; i++) {
            noiseProfile[i] /= (noiseCaptureDuration / 1000 * 16); // Adjust based on your sample rate
          }
          noiseProfileCaptured = true;
          Serial.println("Noise profile captured");
        }
      } else {
        // Subtract noise profile from the audio signal
        for (int i = 0; i < receivedBytes / 2; i++) {
          audioSamples[i] -= noiseProfile[i];
        }

        // Apply gain
        for (int i = 0; i < receivedBytes / 2; i++) {
          audioSamples[i] = (int16_t)(audioSamples[i] * gain);
        }

        // Find the maximum absolute sample value
        int16_t maxSample = 0;
        for (int i = 0; i < receivedBytes / 2; i++) {
          if (abs(audioSamples[i]) > maxSample) {
            maxSample = abs(audioSamples[i]);
          }
        }

        // Calculate the normalization factor
        float normalizationFactor = targetPeak / (float)maxSample;

        // Apply the normalization factor to all samples
        for (int i = 0; i < receivedBytes / 2; i++) {
          audioSamples[i] = (int16_t)(audioSamples[i] * normalizationFactor);
        }
      }

      size_t bytes_written;
      esp_err_t result = i2s_write(I2S_NUM_0, audioSamples, receivedBytes, &bytes_written, portMAX_DELAY);
      if (result == ESP_OK && bytes_written > 0) {
        Serial.print("Received and wrote audio data: ");
        Serial.println(bytes_written);
      }
      receivedBytes = 0; // Reset the received bytes counter for the next batch
    }
  }
};

void setup() {
  Serial.begin(115200);

  BLEDevice::init("ESP32_Receiver");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ |
                      BLECharacteristic::PROPERTY_WRITE |
                      BLECharacteristic::PROPERTY_NOTIFY
                    );

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new MyCallbacks());

  pService->start();

  pServer->getAdvertising()->addServiceUUID(SERVICE_UUID);
  pServer->getAdvertising()->start();
  Serial.println("Waiting for a client connection to notify...");

  // I2S configuration
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
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
    .data_out_num = 22,
    .data_in_num = I2S_PIN_NO_CHANGE
  };
  i2s_set_pin(I2S_NUM_0, &pin_config);

  // Start capturing noise profile
  noiseCaptureStartTime = millis();
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    char cmd = input.charAt(0);
    float value = input.substring(1).toFloat();

    switch (cmd) {
      case 'p':
        targetPeak = value;
        Serial.print("Target peak level set to: ");
        Serial.println(targetPeak);
        break;
      case 'g':
        gain = value;
        Serial.print("Gain set to: ");
        Serial.println(gain);
        break;
      default:
        Serial.println("Unknown command. Use 'p' for target peak level, 'g' for gain.");
        break;
    }
  }

  // Handle BLE and I2S interactions in the callbacks
  delay(10);  // Reduced delay for lower latency
}
