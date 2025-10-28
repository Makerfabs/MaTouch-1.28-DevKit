/*
Library version:
Arduino IDE 2.3.6
esp32 V3.2.0
GFX Library for Arduino v1.6.0

Tools:
USB CDC On Boot: Enabled
*/

#include <Arduino_GFX_Library.h>
#include <WiFi.h>
#include <WebServer.h>

#define TFT_BLK 45
#define TFT_RES 21
#define TFT_CS 1
#define TFT_MOSI 2
#define TFT_MISO -1
#define TFT_SCLK 42
#define TFT_DC 46

#define LED_PIN 4

const char* ssid     = "Makerfabs"; // Change this to your WiFi SSID
const char* password = "20160704"; // Change this to your WiFi password

Arduino_ESP32SPI *bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
Arduino_GFX *gfx = new Arduino_GC9A01(bus, TFT_RES, 0 /* rotation */, true /* IPS */);

WebServer server(80);

void setup()
{
  // put your setup code here, to run once:
    Serial.begin(115200);

    pinMode(LED_PIN, OUTPUT);

    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500); Serial.print(".");
    }
    Serial.println("\nWiFi connected, IP address: " + WiFi.localIP().toString());

    server.on("/", handleRoot);
    server.on("/led/on", handleLedOn);
    server.on("/led/off", handleLedOff);
    server.begin();

    // Initialize screen
    pinMode(TFT_BLK, OUTPUT);
    digitalWrite(TFT_BLK, HIGH);
    
    gfx->begin();
    gfx->fillScreen(WHITE);
    gfx->setTextSize(2);
    gfx->setTextColor(BLACK);
    gfx->setCursor(45, 50);
    gfx->println(F("WebServer Demo"));
    gfx->setCursor(20, 95);
    gfx->println(F("IP Address:"));
    gfx->setCursor(20, 125);
    gfx->println(WiFi.localIP().toString());
    gfx->setCursor(45, 180);
    gfx->println(F("LED is OFF"));
}

void loop()
{
  server.handleClient();  // Process incoming client requests, handle HTTP requests
}

void handleRoot()
{
  bool ledState = digitalRead(LED_PIN);
  // Set button color based on LED state: red (#f44336) when ON, green (#4CAF50) when OFF
  String buttonColor = ledState ? "#f44336" : "#4CAF50";
  // Set button text based on LED state: "Turn OFF" when ON, "Turn ON" when OFF
  String buttonText = ledState ? "Turn OFF" : "Turn ON";
  // Set button link based on LED state: link to OFF path when ON, link to ON path when OFF
  String buttonLink = ledState ? "/led/off" : "/led/on";
  
  // Build HTML page content
  String html = "<!DOCTYPE html>\n"
  "<html>\n"
  "<head>\n"
  "  <meta charset=\"utf-8\">\n" // Set UTF-8 character encoding
  "  <title>WebServer Demo LED Control</title>\n" // Set page title
  "  <style>\n" // Begin CSS style definition
  "    body { font-family: Arial; text-align: center; margin-top: 50px; }\n" // Set body style: Arial font, center alignment, 50px top margin
  "    a { display: inline-block; padding: 15px; margin: 10px; font-size: 20px; color: white; text-decoration: none; border-radius: 8px; }\n" // Set button style: inline-block display, 15px padding, 10px margin, 20px font size, white text, no underline, 8px rounded corners
  "  </style>\n" // End CSS style definition
  "</head>\n"
  "<body>\n"
  "  <h1>WebServer Demo LED Control</h1>\n" // Page main title
  "  <a href=\"" + buttonLink + "\" style=\"background: " + buttonColor + "\">" + buttonText + "</a>\n" // Dynamically generate control button: set link address (buttonLink), background color (buttonColor) and button text (buttonText) based on LED state, creating a clickable button element
  "</body>\n"
  "</html>";
  
  // Send HTTP response with status code 200, content type HTML, and the constructed html string
  server.send(200, "text/html", html);
}

//Handle LED ON/OFF request, control LED ON/OFF.
void handleLedOn()
{
  digitalWrite(LED_PIN, HIGH);
  
  // Display LED status on screen
  gfx->fillRect(0, 180, 240, 30, WHITE);
  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->setCursor(60, 180);
  gfx->println(F("LED is ON"));
  
  // Redirect back to the main page
  server.sendHeader("Location", "/", true);  // Set HTTP header for redirection to root path, with replace=true to override any existing Location header
  server.send(302, "text/plain", "");  // Send HTTP 302 (Found) status code to perform the redirection, with empty content
}

void handleLedOff()
{
  digitalWrite(LED_PIN, LOW);
  
  // Display LED status on screen
  gfx->fillRect(0, 180, 240, 30, WHITE);
  gfx->setTextSize(2);
  gfx->setTextColor(BLACK);
  gfx->setCursor(60, 180);
  gfx->println(F("LED is OFF"));
  
  // Redirect back to the main page
  server.sendHeader("Location", "/", true);  // Set HTTP header for redirection to root path, with replace=true to override any existing Location header
  server.send(302, "text/plain", "");  // Send HTTP 302 (Found) status code to perform the redirection, with empty content
}
