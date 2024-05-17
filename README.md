# ESP32 Motor Test Bench

## Project Overview

This project is designed to test a twin motor arm from a drone. It uses an ESP32 development board as the main controller, along with INA228 chips for voltage and current measurements, an OLED display, and a DHT11 sensor. The ESP32 acts as a Wi-Fi access point and hosts a web interface for controlling the tests and displaying real-time data.

## Hardware Components

- ESP32dev board
- INA228 chips at addresses 0x40 and 0x41 over I2C
- 1.3-inch OLED display over I2C
- DHT11 sensor connected to GPIO pin 4
- Two GPIO outputs for triggering relays (pins 26 and 27)
- Two PWM outputs for controlling motors (pins 32 and 33)

## Functionality

- The ESP32 creates a Wi-Fi hotspot with SSID "MotorTester3000" and password "test1234".
- A web server hosted on the ESP32 provides a user interface for motor testing.
- The web interface includes pages for inputting test information, running tests, and reviewing test data.
- Real-time voltage and current readings from the INA228 sensors are displayed on the web interface.
- Sensor data and user inputs are logged to a file that can be downloaded.

## Installation

1. Install PlatformIO in Visual Studio Code.
2. Clone the repository: `git clone https://github.com/puffdaddy/ESP32_Motor_test_bench.git`
3. Open the project in Visual Studio Code.
4. Build and upload the code to your ESP32 board.

## Usage

1. Connect to the Wi-Fi hotspot created by the ESP32.
2. Open a web browser and go to `http://192.168.4.1`.
3. Follow the instructions on the web interface to run motor tests and view data.

## License

This project is licensed under the MIT License.
