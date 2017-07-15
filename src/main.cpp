#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>

#include "config.h"

// amount of time in milliseconds to wait before closing connection with no data being receieved
#define IDLE_TIMEOUT_MS 3000

const int ONE_WIRE_PIN = D2;

// initialise OneWire library
OneWire oneWire(ONE_WIRE_PIN);

// initialise temperature library
DallasTemperature sensors(&oneWire);

const byte DEVICE_ADDRESS_LENGTH = 8;
byte deviceCount;
byte deviceIndex;

unsigned long interval; // polling interval in milliseconds
unsigned long lastRunTime;
unsigned long currentTime;

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

/**
 * Return the address of a device as a hexadecimal string.
 *
 * Example:
 *   0x28ff123456789abc
 */
String formatAddress(byte address[8]) {
   String s = "0x";
   for (uint8_t i = 0; i < 8; i++) {
    // zero-pad the address if necessary
    if (address[i] < 16) {
      s += "0";
    }
    s += String(address[i], HEX);
   }
   return s;
}

void readSensors(Reading readings[]) {
  // call sensors.requestTemperatures() to issue a global temperature request to all devices in the bus
  Serial.println(F("Requesting temperatures..."));
  sensors.requestTemperatures();

  for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
    Reading* reading = &readings[deviceIndex];
    if (sensors.getAddress(reading->deviceAddress, deviceIndex)) {
      float temperature = sensors.getTempC(reading->deviceAddress);
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

void wifiConnect() {
  Serial.print(F("Connecting to Wi-Fi network: "));
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(1000);
  }
  Serial.println(); // close off "connection dots" line

  Serial.print(F("Connected to Wi-Fi network with IP address: "));
  Serial.println(WiFi.localIP());
}

/**
 * Construct JSON message with sensor readings.
 *
 * Examples:
 *   { "readings": { "0x28ff123456789abc": 20.50 } }
 *   { "readings": { "0x28ff123456789abc": 18.06, "0x28ff123456789abd": 17.19 } }
 */
String constructMessage(Reading readings[]) {
  String message = "{ \"readings\": {";

  for (deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
      Reading reading = readings[deviceIndex];
      if (reading.valid) {
        String address = formatAddress(reading.deviceAddress);
        // readings are comma-separated
        message = message + " \"" + address + "\": " + String(reading.temperature) + ",";
      }
  }

  // remove trailing "," for last reading in comma-separated list
  if (message[message.length() - 1] == ',') {
    message.remove(message.length() - 1);
  }
  message += " } }";

  return message;
}

void sendReadings(Reading readings[]) {
  String message = constructMessage(readings);
  Serial.println(F("Sensor data message:"));
  Serial.println(message);

  WiFiClient client;

  Serial.print(F("Connecting to server "));
  Serial.print(API_HOST);
  Serial.print(F(" on port "));
  Serial.print(API_PORT);
  Serial.print(F(" with endpoint "));
  Serial.println(API_ENDPOINT);
  if (client.connect(API_HOST, API_PORT)) {
    // send request to server
    client.print(F("POST "));
    client.print(API_ENDPOINT);
    client.println(F(" HTTP/1.1"));
    client.print(F("Host: "));
    client.println(API_HOST);
    if (API_AUTH_VALUE != "") {
      Serial.println(F("Using HTTP Basic authentication"));
      client.print(F("Authorization: Basic "));
      client.println(API_AUTH_VALUE);
    } else {
      Serial.println(F("Not using authentication"));
    }
    client.println(F("Content-Type: application/json"));
    client.println(F("User-Agent: NodeMCU/1.0"));
    client.println(F("Connection: close"));
    client.print(F("Content-Length: "));
    client.println(message.length(), DEC);
    client.println();
    client.println(message);
    client.println();

    // read response data until either the connection is closed, or the idle timeout is reached
    Serial.println(F("-----------------------"));
    unsigned long lastRead = millis();
    while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
      while(client.available()) {
        char c = client.read();
        Serial.print(c);
        lastRead = millis();
      }
    }
    Serial.println(F("-----------------------"));
  } else {
    Serial.println(F("Unable to connect to server"));
  }

  // close connection to server
  client.stop();
}

/**
 * Read and process data from sensors (one cycle).
 * Will be performed in a loop.
 */
void run() {
  Reading readings[deviceCount];
  readSensors(readings);

  printReadings(readings);

  wifiConnect();
  sendReadings(readings);
  WiFi.disconnect();
}

void setup() {
  Serial.begin(115200);
  Serial.println(F("Starting..."));
  delay(100);

  sensorInit();

  // set WiFi to station mode and disconnect from an AP if it was previously connected
  WiFi.mode(WIFI_STA);
  if (WiFi.isConnected()) {
    WiFi.disconnect();
  }
  delay(100);

  // set polling interval
  interval = (unsigned long)POLLING_INTERVAL_S * 1000;
  Serial.print(F("Loop interval: "));
  Serial.print(POLLING_INTERVAL_S);
  Serial.print(F(" seconds => "));
  Serial.print(interval);
  Serial.println(F(" milliseconds"));

  // initialise to current time
  lastRunTime = millis();

  Serial.println(F("Setup done"));

  // start first run after setup
  run();
}

void loop() {
  // interval loop
  currentTime = millis();
  if (currentTime - lastRunTime > interval) {
    lastRunTime = currentTime;
    run();
  }
}
