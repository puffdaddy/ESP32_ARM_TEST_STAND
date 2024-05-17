#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_INA228.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>

// Hardware definitions
#define DHTPIN 4
#define DHTTYPE DHT11
#define RELAY1_PIN 26
#define RELAY2_PIN 27
#define PWM1_PIN 32
#define PWM2_PIN 33

// I2C addresses
#define INA228_ADDR1 0x40
#define INA228_ADDR2 0x41

// Wi-Fi credentials
const char *ssid = "MotorTester3000";
const char *password = "test1234";

// INA228 instances
Adafruit_INA228 ina228_1 = Adafruit_INA228();
Adafruit_INA228 ina228_2 = Adafruit_INA228();

// OLED display instance
Adafruit_SSD1306 display(128, 64, &Wire);

// DHT sensor instance
DHT dht(DHTPIN, DHTTYPE);

// Web server instance
AsyncWebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

// Variables to store form data
String armSerialNumber;
String currentHours;
String operatorName;

// Variables for motor control
int pwmChannel1 = 0;
int pwmChannel2 = 1;
int pwmFrequency = 5000;
int pwmResolution = 8;

// Variables for data logging
File logFile;
bool isLogging = false;

// Function declarations
void initHardware();
void initWebServer();
void handleFormSubmit(AsyncWebServerRequest *request);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
void startTest(String testType);
void stopTest();
void logData();
void sendData();
void handleReviewPage(AsyncWebServerRequest *request);
void handleDownload(AsyncWebServerRequest *request);

void setup() {
    // Serial setup for debugging
    Serial.begin(115200);
    Serial.println("Starting setup...");

    // Initialize hardware
    initHardware();

    // Initialize web server
    initWebServer();

    // Initialize WebSocket
    webSocket.begin();
    webSocket.onEvent(onEvent);

    Serial.println("Setup complete.");
}

void loop() {
    webSocket.loop();
    if (isLogging) {
        logData();
    }
    sendData();
    delay(100);
}

void initHardware() {
    // Initialize I2C
    Wire.begin();

    // Initialize INA228 sensors
    if (!ina228_1.begin(INA228_ADDR1) || !ina228_2.begin(INA228_ADDR2)) {
        Serial.println("Failed to find INA228 chip");
        while (1);
    }

    // Initialize OLED display
    if (!display.begin(SSD1306_I2C_ADDRESS, OLED_RESET)) {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;);
    }
    display.display();
    delay(2000);
    display.clearDisplay();

    // Initialize DHT sensor
    dht.begin();

    // Initialize relays
    pinMode(RELAY1_PIN, OUTPUT);
    pinMode(RELAY2_PIN, OUTPUT);

    // Initialize PWM pins
    ledcSetup(pwmChannel1, pwmFrequency, pwmResolution);
    ledcAttachPin(PWM1_PIN, pwmChannel1);
    ledcSetup(pwmChannel2, pwmFrequency, pwmResolution);
    ledcAttachPin(PWM2_PIN, pwmChannel2);

    // Start SPIFFS
    if (!SPIFFS.begin()) {
        Serial.println("An error has occurred while mounting SPIFFS");
        return;
    }
}

void initWebServer() {
    // Connect to Wi-Fi
    WiFi.softAP(ssid, password);
    Serial.println("WiFi Access Point started");

    // Serve static files
    server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

    // Handle form submission
    server.on("/submit", HTTP_POST, handleFormSubmit);

    // Handle test page
    server.on("/test", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(SPIFFS, "/test.html", "text/html");
    });

    // Handle review page
    server.on("/review", HTTP_GET, handleReviewPage);

    // Handle file download
    server.on("/download", HTTP_GET, handleDownload);

    // Start server
    server.begin();
    Serial.println("Web server started");
}

void handleFormSubmit(AsyncWebServerRequest *request) {
    if (request->hasParam("armSerialNumber", true) && request->hasParam("currentHours", true) && request->hasParam("operatorName", true)) {
        armSerialNumber = request->getParam("armSerialNumber", true)->value();
        currentHours = request->getParam("currentHours", true)->value();
        operatorName = request->getParam("operatorName", true)->value();

        Serial.println("Form submitted:");
        Serial.println("Arm Serial Number: " + armSerialNumber);
        Serial.println("Current Hours: " + currentHours);
        Serial.println("Operator Name: " + operatorName);

        request->redirect("/test");
    } else {
        request->send(400, "text/html", "Please fill in all fields");
    }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        String message = (char*)data;

        if (message.indexOf("test") >= 0) {
            DynamicJsonDocument doc(1024);
            deserializeJson(doc, message);
            String testType = doc["test"];
            startTest(testType);
        }
    }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    switch (type) {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
    }
}

void startTest(String testType) {
    Serial.println("Starting test: " + testType);
    // Turn on warning light
    digitalWrite(RELAY1_PIN, HIGH);
    delay(5000); // Wait for 5 seconds

    if (testType == "full") {
        ledcWrite(pwmChannel1, 128); // Set PWM duty cycle to 50%
        ledcWrite(pwmChannel2, 128); // Set PWM duty cycle to 50%
    } else if (testType == "motor1") {
        ledcWrite(pwmChannel1, 128); // Set PWM duty cycle to 50%
        ledcWrite(pwmChannel2, 0);   // Turn off motor 2
    } else if (testType == "motor2") {
        ledcWrite(pwmChannel1, 0);   // Turn off motor 1
        ledcWrite(pwmChannel2, 128); // Set PWM duty cycle to 50%
    }

    // Open log file
    String fileName = "/" + armSerialNumber + "_" + currentHours + "_" + operatorName + ".csv";
    logFile = SPIFFS.open(fileName, FILE_WRITE);
    if (!logFile) {
        Serial.println("Failed to open log file for writing");
        return;
    }
    isLogging = true;

    // Placeholder for test execution code
    // Add code to log sensor data and manage test duration
}

void stopTest() {
    // Turn off motors
    ledcWrite(pwmChannel1, 0);
    ledcWrite(pwmChannel2, 0);

    // Turn off warning light
    digitalWrite(RELAY1_PIN, LOW);

    // Close log file
    if (logFile) {
        logFile.close();
    }
    isLogging = false;

    Serial.println("Test completed");

    // Redirect to review page
    server.on("/test-complete", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->redirect("/review");
    });
}

void logData() {
    // Read sensor data
    float voltage1 = ina228_1.readBusVoltage() / 1000.0;
    float current1 = ina228_1.readCurrent() / 1000.0;
    float voltage2 = ina228_2.readBusVoltage() / 1000.0;
    float current2 = ina228_2.readCurrent() / 1000.0;

    // Calculate average values
    float avgVoltage = (voltage1 + voltage2) / 2.0;
    float avgCurrent = (current1 + current2) / 2.0;

    // Log data to file
    if (logFile) {
        logFile.printf("%f,%f\n", avgVoltage, avgCurrent);
    }
}

void sendData() {
    // Read sensor data
    float voltage1 = ina228_1.readBusVoltage() / 1000.0;
    float current1 = ina228_1.readCurrent() / 1000.0;
    float voltage2 = ina228_2.readBusVoltage() / 1000.0;
    float current2 = ina228_2.readCurrent() / 1000.0;

    // Calculate average values
    float avgVoltage = (voltage1 + voltage2) / 2.0;
    float avgCurrent = (current1 + current2) / 2.0;

    // Prepare JSON data
    DynamicJsonDocument doc(1024);
    doc["voltage"] = avgVoltage;
    doc["current"] = avgCurrent;

    String jsonData;
    serializeJson(doc, jsonData);

    // Send data to all connected WebSocket clients
    webSocket.broadcastTXT(jsonData);
}

void handleReviewPage(AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/review.html", "text/html");
}

void handleDownload(AsyncWebServerRequest *request) {
    if (logFile) {
        logFile.close();
    }

    String fileName = "/" + armSerialNumber + "_" + currentHours + "_" + operatorName + ".csv";
    request->send(SPIFFS, fileName, "text/csv", true, "testdata.csv");
}
