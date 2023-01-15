#include <Arduino.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <FS.h>
#include <WebSocketsServer.h>

// Setup wifi credentials
const char *ssid = "ESP32";
const char *password = "cocklmao";
char path[] = "/";
char host[] = "localhost";

WiFiClient client;

WiFiServer server(80);
WebSocketsServer webSocketServer(81);

String message;

// Define imu
Adafruit_MPU6050 mpu;

#define WIRE Wire
// define display
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &WIRE);

const bool displayIMU = true;

void writeTextToScreen(String str)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.print(str);
  yield();
  display.display();
}

#define USE_SERIAL Serial

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
	const uint8_t* src = (const uint8_t*) mem;
	USE_SERIAL.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
	for(uint32_t i = 0; i < len; i++) {
		if(i % cols == 0) {
			USE_SERIAL.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
		}
		USE_SERIAL.printf("%02X ", *src);
		src++;
	}
	USE_SERIAL.printf("\n");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    USE_SERIAL.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = webSocketServer.remoteIP(num);
    USE_SERIAL.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);

    // send message to client
    webSocketServer.sendTXT(num, "Connected");
  }
  break;
  case WStype_TEXT:
    USE_SERIAL.printf("[%u] get Text: %s\n", num, payload);
    message = String((char *)payload);

    // send message to client
    // webSocket.sendTXT(num, "message here");

    // send data to all connected clients
    // webSocket.broadcastTXT("message here");
    break;
  case WStype_BIN:
    USE_SERIAL.printf("[%u] get binary length: %u\n", num, length);
    hexdump(payload, length);

    // send message to client
    // webSocket.sendBIN(num, payload, length);
    break;

  default:
    USE_SERIAL.println("Other");
    Serial.println("Test");
  }
}

void setup()
{
  USE_SERIAL.begin(115200);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); // Address 0x3C for 128x32

  // Clear the buffer.
  display.clearDisplay();
  display.display();

  // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Initialize imu
  if (!mpu.begin())
  {
    Serial.println("Failed to find MPU6050 chip");
    while (1)
    {
      delay(10);
    }
  }

  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setMotionDetectionThreshold(1);
  mpu.setMotionDetectionDuration(5); // used to be 20
  mpu.setInterruptPinLatch(true);    // Keep it latched.  Will turn off when reinitialized.
  mpu.setInterruptPinPolarity(true);
  mpu.setMotionInterrupt(true);

  // setup wifi
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());
  server.begin();
  webSocketServer.begin();
  webSocketServer.onEvent(webSocketEvent);
}

void loop()
{

  if (displayIMU)
  {
    if (mpu.getMotionInterruptStatus())
    {
      // Get new sensor events with the readings
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      writeTextToScreen("A:" + String(a.acceleration.x) + "," + String(a.acceleration.y) + "," + String(a.acceleration.z) + "\nG:" + String(g.gyro.x) + "," + String(g.gyro.y) + "," + String(g.gyro.z) + "\nMovement: True");
    }
    else
    {
      sensors_event_t a, g, temp;
      mpu.getEvent(&a, &g, &temp);
      writeTextToScreen("A:" + String(a.acceleration.x) + "," + String(a.acceleration.y) + "," + String(a.acceleration.z) + "\nG:" + String(g.gyro.x) + "," + String(g.gyro.y) + "," + String(g.gyro.z) + "\nMovement: False\nMRM: " + message);
    }
  }

  webSocketServer.loop();
}
