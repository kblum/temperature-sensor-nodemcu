#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_PIN = D2;

// initialise OneWire library
OneWire oneWire(ONE_WIRE_PIN);

// initialise temperature library
DallasTemperature sensors(&oneWire);

const byte DEVICE_ADDRESS_LENGTH = 8;
byte deviceCount;
byte deviceIndex;

struct Reading {
  byte deviceAddress[DEVICE_ADDRESS_LENGTH];
  bool valid; // if a reading could be obtained from device
  float temperature; // measured in degrees Celsius
};

/**
 * Initialise all sensors on 1-Wire bus.
 */
void sensorInit() {
  Serial.print(F("Sensor bus on PIN "));
  Serial.println(ONE_WIRE_PIN, DEC);

  // start temperature library
  sensors.begin();

  deviceCount = sensors.getDeviceCount();
  Serial.print(F("Number of devices on bus: "));
  Serial.println(deviceCount, DEC);

  // report parasite power requirements
  Serial.print(F("Parasite power: "));
  if (sensors.isParasitePowerMode()) {
    Serial.println(F("on"));
  }
  else {
    Serial.println(F("off"));
  }

  Serial.println();
}

/**
 * Print the address of a device in hexadecimal format to the serial port.
 *
 * Example:
 *   { 0x28, 0xFF, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC }
 */
void printAddress(byte address[DEVICE_ADDRESS_LENGTH]) {
  Serial.print(F("{ "));
  for (uint8_t i = 0; i < DEVICE_ADDRESS_LENGTH; i++) {
    Serial.print(F("0x"));
    // zero-pad the address if necessary
    if (address[i] < 16) {
      Serial.print(F("0"));
    }
    Serial.print(address[i], HEX);
    // seperate each octet with a comma
    if (i < 7) {
      Serial.print(F(", "));
    }
  }
  Serial.print(F(" }"));
}

void readSensors(Reading readings[]) {
  // call sensors.requestTemperatures() to issue a global temperature request to all devices in the bus
  Serial.println(F("Requesting temperatures..."));
  sensors.requestTemperatures();

  for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
    Reading* reading = &readings[deviceIndex];
    if (sensors.getAddress(readings->deviceAddress, deviceIndex)) {
      float temperature = sensors.getTempC(readings->deviceAddress);
      reading->valid = true;
      reading->temperature = temperature;
    } else {
      // unable to read from device; therefore clear values
      reading->valid = false;
      memset(reading->deviceAddress, 0, sizeof(reading->deviceAddress));
      reading->temperature = 0;
      Serial.print(F("Unable to read from device: "));
      Serial.println(deviceIndex);
    }
  }
}

void printReadings(Reading readings[]) {
  Serial.println(F("Readings:"));
  for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
      Reading reading = readings[deviceIndex];
      if (reading.valid) {
        Serial.print(F("Device "));
        Serial.print(deviceIndex);
        Serial.print(F("; address: "));
        printAddress(reading.deviceAddress);
        Serial.print(F("; temperature: "));
        Serial.println(reading.temperature);
      }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting..."));

  sensorInit();

  Serial.println(F("Setup done"));
}

void loop() {
  Reading readings[deviceCount];
  readSensors(readings);

  printReadings(readings);

  delay(2000);
}
