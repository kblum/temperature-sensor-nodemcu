# NodeMCU Temperature Sensor

This project is a [NodeMCU](http://www.nodemcu.com/index_en.html) temperature sensor using a [DS18B20](http://www.maximintegrated.com/en/products/analog/sensors-and-sensor-interface/DS18B20.html) digital thermometer (1-Wire interface).

[PlatformIO](http://platformio.org) is used for development.


## Hardware

The 1-Wire sensor bus should be connected to digital I/O pin D2.


## Configuration

Create a `src/config.h` file and populate with the following settings (substituting in appropriate values):

```cpp
// Wi-Fi credentials
#define WLAN_SSID "SSID"
#define WLAN_PASS "PASSWORD"

// polling interval of loop in seconds
#define POLLING_INTERVAL_S 300

// API endpoint
#define API_HOST "HOST"
#define API_ENDPOINT "/api"
#define API_PORT 80
// empty string for no auth or base64 encoded API credentials
#define API_AUTH_VALUE ""
```

The configuration file is not under version control.


## API

The temperature readings are sent to a remote server through an HTTP/1.1 POST request. The data is sent in JSON format.

The following headers are included in the request:

* `Host`
* `Authorization` - HTTP Basic authentication (if configured)
* `User-Agent` - `NodeMCU/1.0`
* `Content-Type` - `application/json`
* `Connection` - `close`
* `Content-Length`

Examples of request body:

```json
{ "readings": { "0x28ff123456789abc": 20.50 } }
```

```json
{ "readings": { "0x28ff123456789abc": 18.06, "0x28ff123456789abd": 17.19 } }
```

The unique 64-bit serial code for each device (in hexadecimal format) is mapped to the temperature reading from the device.


## Development

Debug output will be written to the serial port, connecting with a baud rate of 115200.


The PlatformIO CLI tools can be installed on macOS using [Homebrew](https://brew.sh) as follows:

```sh
brew install platformio
```

If the device is not recognised, then install the [CP210x USB to UART Bridge VCP Drivers
](https://www.silabs.com/products/development-tools/software/usb-to-uart-bridge-vcp-drivers) from Silicon Labs. The extension must be approved on macOS.
