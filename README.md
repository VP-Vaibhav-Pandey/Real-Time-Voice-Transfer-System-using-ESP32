# Real-Time-Voice-Transfer-System-using-ESP32
Real-Time Voice Transfer System using two esp32 via bluetooth 
Objective
To develop a real-time voice transfer system using two ESP32 boards where one ESP32 captures audio from a microphone and sends it over Bluetooth to a second ESP32, which then plays the audio through a connected speaker.

Components Required
2 x ESP32 boards
REES52 MAX98357 I2S Audio Amplifier Module
INMP441 Omnidirectional Microphone I2S Interface Digital Output Sensor Module
4-ohm speaker
USB cables for powering the ESP32 boards
System Overview
Transmitter ESP32:

Captures audio from the INMP441 microphone using the I2S protocol.
Sends the captured audio data over Bluetooth using BLE (Bluetooth Low Energy).
Receiver ESP32:

Receives audio data over Bluetooth.
Processes the audio data to apply volume normalization, gain control, and noise reduction.
Plays the processed audio through the connected speaker using the MAX98357 I2S Audio Amplifier Module.
Approach
Transmitter ESP32:
Bluetooth Initialization: The ESP32 initializes BLE to scan for available devices and connect to the receiver ESP32.
I2S Configuration: Configures the I2S peripheral to capture audio data from the INMP441 microphone.
Audio Data Capture: Continuously reads audio data from the microphone and sends it over BLE to the receiver ESP32 in chunks.
Receiver ESP32:
Bluetooth Initialization: Sets up the ESP32 as a BLE server that the transmitter can connect to.
I2S Configuration: Configures the I2S peripheral to send audio data to the MAX98357 amplifier.
Data Reception and Processing: Receives audio data over BLE, processes it for volume normalization and noise reduction, and writes it to the I2S peripheral for playback.
Key Functionalities and Parameters
Volume Normalization:

Ensures that the audio signal maintains a consistent volume level.
Uses a target peak level (targetPeak) to adjust the volume of the audio signal so that its peak value matches the target.
Gain Control:

Adjusts the overall volume of the audio signal.
Uses a gain parameter (gain) to amplify or attenuate the audio signal.
Noise Reduction:

Captures a noise profile during the initial operation phase to identify and subtract background noise from the audio signal.
Averages the noise captured over a specified duration (noiseCaptureDuration) to create a noise profile which is then subtracted from the incoming audio.
Transmitter ESP32 Details
1. Bluetooth Initialization:

Initializes BLE and starts scanning for the receiver ESP32.
2. I2S Configuration:

Configures I2S with the following parameters:
Mode: Master, RX
Sample Rate: 16000 Hz
Bits per Sample: 16-bit
Channel Format: Only left channel
Communication Format: I2S MSB
DMA Buffer Count: 8
DMA Buffer Length: 64
3. Audio Data Capture and Transmission:

Captures audio data from the INMP441 microphone.
Sends audio data over BLE to the receiver in chunks of 512 bytes.
Receiver ESP32 Details
1. Bluetooth Initialization:

Sets up as a BLE server and waits for the transmitter to connect.
2. I2S Configuration:

Configures I2S with the following parameters:
Mode: Master, TX
Sample Rate: 16000 Hz
Bits per Sample: 16-bit
Channel Format: Only left channel
Communication Format: I2S MSB
DMA Buffer Count: 8
DMA Buffer Length: 64
3. Data Reception and Processing:

Receives audio data over BLE.
Applies volume normalization, gain control, and noise reduction.
Plays the processed audio through the connected speaker.
Volume Normalization and Gain Control
Volume Normalization:

Ensures that the audio signal maintains a consistent volume level.
Uses a target peak level (targetPeak) to adjust the volume of the audio signal so that its peak value matches the target.
Gain Control:

Adjusts the overall volume of the audio signal.
Uses a gain parameter (gain) to amplify or attenuate the audio signal.

Noise Reduction
Noise Profile Capture:

Captures background noise during the initial 5 seconds (noiseCaptureDuration) of operation.
Averages the noise captured to create a noise profile.
Noise Reduction:

Subtracts the captured noise profile from the incoming audio signal to reduce background noise.
Serial Commands for Adjustments
Target Peak Level Adjustment:

Send p<value> via the serial monitor (e.g., p30000) to adjust the target peak level.
Gain Adjustment:

Send g<value> via the serial monitor (e.g., g1.5) to adjust the gain.
Summary
This project demonstrates a method to capture, transmit, and play audio data using two ESP32 boards with BLE and I2S interfaces. The system includes features for volume normalization, gain control, and noise reduction to ensure high-quality audio playback. The approach involves capturing audio data on the transmitter, sending it over Bluetooth, receiving and processing it on the receiver, and playing it through a connected speaker. The setup allows for real-time voice transmission with adjustable parameters for audio quality improvement.
