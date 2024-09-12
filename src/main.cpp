// Include Library
#include <minh_quang2304-project-1_inferencing.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include "BMP085.h"
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Constant Define
#define ACC_RANGE 1 // 0: -/+2G; 1: +/-4G
#define CONVERT_G_TO_MS2 (9.81 / (16384 / (1. + ACC_RANGE)))
#define MAX_ACCEPTED_RANGE (2 * 9.81) + (2 * 9.81) * ACC_RANGE
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// UUID of services and characterristic for BLE
#define MOTION_SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define DEPTH_SERVICE_UUID "86aea1ba-edf6-4de6-b337-5babca1943a6"
#define PRESSURE_SERVICE_UUID "7723506f-9fbc-4326-b768-fa840af93bcd"

#define MOTION_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define MOTION2_CHARACTERISTIC_UUID "44a861fa-e8f4-4669-a9f9-d871664d01a8"
#define DEPTH_CHARACTERISTIC_UUID "563019db-678f-4ea9-beee-8399384eb3c7"
#define DEPTH2_CHARACTERISTIC_UUID "1895bd6d-d8d9-4f2a-859e-71c9a3c4d6ff"
#define PRESSURE_CHARACTERISTIC_UUID "ae0550ea-c5ba-4f10-933f-76cc3baa5d77"
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Initialize sensors library
MPU6050 imu;
int16_t ax, ay, az;
BMP085 barometer;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Declare variables
float temperature;
float pressure;
int32_t altitude;
bool deviceConnected = false;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Initialized BLE Characteristic for value exchange
BLECharacteristic *pCharacteristicValueMotion;
BLECharacteristic *pCharacteristicValueDepth;
BLECharacteristic *pCharacteristicValueMotion2;
BLECharacteristic *pCharacteristicValueDepth2;
BLECharacteristic *pCharacteristicValuePressure;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal

/**
 * @brief      Arduino setup function
 */

// Class to check for device Bluetooth connection
class MyServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer *pServer)
  {
    deviceConnected = false;
  }
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup()
{
  Serial.begin(115200);

  // Initialized sensors physically
  Serial.println("Initializing I2C devices...");
  Wire.begin();
  imu.initialize();
  barometer.initialize();
  delay(10);
  Serial.println("Testing device connections...");
  Serial.println(barometer.testConnection() ? "BMP085 connection successful" : "BMP085 connection failed");

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // Set IMU (gyrometer) offset for error evaluation
  imu.setXAccelOffset(-4732);
  imu.setYAccelOffset(4703);
  imu.setZAccelOffset(8867);
  imu.setXGyroOffset(61);
  imu.setYGyroOffset(-73);
  imu.setZGyroOffset(35);
  imu.setFullScaleAccelRange(ACC_RANGE);
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  if (EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME != 3)
  {
    ei_printf("ERR: EI_CLASSIFIER_RAW_SAMPLES_PER_FRAME should be equal to 3 (the 3 sensor axes)\n");
    return;
  }

  // Configure BLE

  // Init BLE server (inser the name of the BLE device)
  BLEDevice::init("ESP32 BLE");

  // Create BLE server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create BLE service
  BLEService *pService = pServer->createService(MOTION_SERVICE_UUID);

  // Create BLE characteristic
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
      MOTION_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  BLECharacteristic *pCharacteristic2 = pService->createCharacteristic(
      MOTION2_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  // Set BLE characteristic initial value
  pCharacteristic->setValue("Motion");
  pCharacteristic2->setValue("Motion");
  pService->start();
  pCharacteristicValueMotion = pCharacteristic;
  pCharacteristicValueMotion2 = pCharacteristic2;

  BLEService *pServiceDepth = pServer->createService(DEPTH_SERVICE_UUID);
  BLECharacteristic *pCharacteristicDepth = pServiceDepth->createCharacteristic(
      DEPTH_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  BLECharacteristic *pCharacteristicDepth2 = pServiceDepth->createCharacteristic(
      DEPTH2_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setValue("Depth");
  pCharacteristic2->setValue("Depth2");
  pServiceDepth->start();
  pCharacteristicValueDepth = pCharacteristicDepth;
  pCharacteristicValueDepth2 = pCharacteristicDepth2;

  BLEService *pServicePressure = pServer->createService(PRESSURE_SERVICE_UUID);
  BLECharacteristic *pCharacteristicPressure = pServicePressure->createCharacteristic(
      PRESSURE_CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ |
          BLECharacteristic::PROPERTY_WRITE);
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  pServicePressure->start();
  pCharacteristicPressure->setValue("Pressure");
  pCharacteristicValuePressure = pCharacteristicPressure;

  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  // Start BLE advertisement
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(MOTION_SERVICE_UUID);
  pAdvertising->addServiceUUID(DEPTH_SERVICE_UUID);
  pAdvertising->addServiceUUID(PRESSURE_SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

/**
 * @brief Return the sign of the number
 *
#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_ACCELEROMETER
#error "Invalid model for current sensor"
#endif
* @param number
 * @return int 1 if positive (or 0) -1 if negative
 */
float ei_get_sign(float number)
{
  return (number >= 0.0) ? 1.0 : -1.0;
}

/**
 * @brief      Get data and run inferencing
 *
 * @param[in]  debug  Get debug info if true
 */
void loop()
{
  ei_printf("\nStarting inferencing in 2 seconds...\n");

  delay(2000);

  ei_printf("Sampling...\n");

  // Allocate a buffer here for the values we'll read from the IMU
  float buffer[EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE] = {0};

  for (size_t ix = 0; ix < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE; ix += 3)
  {
    // Determine the next tick (and then sleep later)
    uint64_t next_tick = micros() + (EI_CLASSIFIER_INTERVAL_MS * 1000);

    imu.getAcceleration(&ax, &ay, &az);
    buffer[ix + 0] = ax;
    buffer[ix + 1] = ay;
    buffer[ix + 2] = az;
    buffer[ix + 0] *= CONVERT_G_TO_MS2;
    buffer[ix + 1] *= CONVERT_G_TO_MS2;
    buffer[ix + 2] *= CONVERT_G_TO_MS2;

    for (int i = 0; i < 3; i++)
    {
      if (fabs(buffer[ix + i]) > MAX_ACCEPTED_RANGE)
      {
        buffer[ix + i] = ei_get_sign(buffer[ix + i]) * MAX_ACCEPTED_RANGE;
      }
    }

    delayMicroseconds(next_tick - micros());
  }

  // Turn the raw buffer in a signal which we can the classify
  signal_t signal;
  int err = numpy::signal_from_buffer(buffer, EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE, &signal);
  if (err != 0)
  {
    ei_printf("Failed to create signal from buffer (%d)\n", err);
    return;
  }

  // Run the classifier
  ei_impulse_result_t result = {0};

  err = run_classifier(&signal, &result, debug_nn);
  if (err != EI_IMPULSE_OK)
  {
    ei_printf("ERR: Failed to run classifier (%d)\n", err);
    return;
  }

  // print the predictions
  ei_printf("Predictions ");
  ei_printf("(DSP: %d ms., Classification: %d ms., Anomaly: %d ms.)",
            result.timing.dsp, result.timing.classification, result.timing.anomaly);
  ei_printf(": \n");
  for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
  {
    ei_printf("    %s: %.5f\n", result.classification[ix].label, result.classification[ix].value);
  }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
  ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

  Serial.println(result.classification[0].label);
  Serial.println(result.classification[0].value);
  barometer.setControl(BMP085_MODE_TEMPERATURE);

  // read calibrated temperature value in degrees Celsius
  temperature = barometer.getTemperatureC();

  // request pressure (3x oversampling mode, high detail, 23.5ms delay)
  barometer.setControl(BMP085_MODE_PRESSURE_3);
  pressure = barometer.getPressure();
  altitude = barometer.getAltitude(pressure);

  static char k[6], k2[6], k3[6];

  // turn floating value to string to read data from the receiver
  dtostrf(result.classification[0].value, 6, 2, k);
  dtostrf(result.classification[1].value, 6, 2, k2);
  dtostrf(pressure, 6, 2, k3);

  // sending data via BLE through characteristic
  if (deviceConnected)
  {
    pCharacteristicValueMotion->setValue(result.classification[1].label);
    pCharacteristicValueMotion->notify();

    pCharacteristicValueMotion2->setValue(k2);
    pCharacteristicValueMotion2->notify();

    pCharacteristicValueDepth->setValue(result.classification[0].label);
    pCharacteristicValueDepth->notify();

    pCharacteristicValueDepth2->setValue(k);
    pCharacteristicValueDepth2->notify();

    pCharacteristicValuePressure->setValue(k3);
    pCharacteristicValuePressure->notify();
  }
#if EI_CLASSIFIER_HAS_ANOMALY == 1
  ei_printf("    anomaly score: %.3f\n", result.anomaly);
#endif

  // display measured values if appropriate
  Serial.println("Pressure: ");
  Serial.print("T/P/A\t");
  Serial.print(temperature);
  Serial.print("\t");
  Serial.print(pressure);
  Serial.print("\t");
  Serial.print(altitude);
  Serial.println("");
}
#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_ACCELEROMETER
#error "Invalid model for current sensor"
#endif