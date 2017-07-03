#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

const int ONE_WIRE_PIN = D2;

// initialise OneWire library
OneWire oneWire(ONE_WIRE_PIN);

// initialise temperature library
DallasTemperature sensors(&oneWire);

byte deviceCount;
byte deviceAddress[8];
byte deviceIndex;

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
void printAddress(byte address[8]) {
  Serial.print(F("{ "));
  for (uint8_t i = 0; i < 8; i++) {
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

void readSensors() {
  // call sensors.requestTemperatures() to issue a global temperature request to all devices in the bus
  Serial.println(F("Requesting temperatures..."));
  sensors.requestTemperatures();

  for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
    if (sensors.getAddress(deviceAddress, deviceIndex)) {
      float temperature = sensors.getTempC(deviceAddress);
      Serial.print(F("Device "));
      Serial.print(deviceIndex);
      Serial.print(F("; address: "));
      printAddress(deviceAddress);
      Serial.print(F("; temperature: "));
      Serial.println(temperature);
    } else {
      Serial.print(F("Unable to read from device: "));
      Serial.println(deviceIndex);
    }
  }
}


void setup() {
  Serial.begin(115200);
  Serial.println("Starting...");

  sensorInit();
}

void loop() {
  readSensors();
  delay(2000);
}
