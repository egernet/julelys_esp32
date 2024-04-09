#include <NeoPixelBus.h>
#include <WiFi.h>
#include <algorithm>
#include <vector>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define LED_ADDR_1 6
#define LED_ADDR_2 7
#define LED_ADDR_3 8

bool isReading = false;
bool imageHaveChange = false;

LiquidCrystal_I2C LCD = LiquidCrystal_I2C(0x27, 16, 2);

const char* ssid     = "*******";
const char* password = "*******";

WiFiServer server(80);

#define LED_TYPE NeoGrbwFeature
#define LED_SIGNAL_PIN 10
#define COLOR_ORDER RGBW
#define LED_ROWS 8
#define LED_COLUMNS 55

#define colorSaturation 128

#define I2C_SDA 1
#define I2C_SCL 2

RgbwColor red(colorSaturation, 0, 0, 0);
RgbwColor green(0, colorSaturation, 0, 0);
RgbwColor blue(0, 0, colorSaturation, 0);
RgbwColor white(colorSaturation);
RgbwColor black(0);

double ledBits = (double)(32 * LED_COLUMNS);
int updateInterval = (int)((ledBits / 800000) * 2000);


NeoPixelBus<LED_TYPE, Neo800KbpsMethod> strip(LED_COLUMNS, LED_SIGNAL_PIN);

std::vector<RgbwColor> selectColors = {red, green, blue, white, black};
RgbwColor colors[] = {red, green, blue, white, black};
RgbwColor image[LED_ROWS][LED_COLUMNS];


void sequence_task(void *pvParameter) {
  while (1) {
    if (isReading) {
      vTaskDelay(updateInterval / portTICK_PERIOD_MS);
    } else {
      runStrip();        
    }
  }
  vTaskDelete( NULL );
}

void rainbow_sequence_task(void *pvParameter) {
  while (1) {
    Serial.println("Start Rainbow");

    for(int j=0; j<256*5; j++) { 
      for(int c=0; c<LED_COLUMNS; c++) {
        uint8_t pos = ((c*256/LED_COLUMNS)+j) & 0xFF;
        RgbColor color = Wheel( pos );
        
        for(int r=0; r<LED_ROWS; r++) {
          setColor(color, r, c);
        }
      }

      imageHaveChange = true;
      do {
        delay(updateInterval);
      } while(isReading);

      spinner();
    }

    Serial.println("Rainbow is ended");
  }
  vTaskDelete( NULL );  
}

void spinner() {
  static int8_t counter = 0;
  const char* glyphs = "\xa1\xa5\xdb";
  LCD.setCursor(15, 1);
  LCD.print(glyphs[counter++]);
  if (counter == strlen(glyphs)) {
    counter = 0;
  }
}

void setup() {
  Serial.begin(115000);

  Wire.setPins(I2C_SDA, I2C_SCL);

  LCD.init();
  LCD.backlight();
  LCD.setCursor(0, 0);
  LCD.print("Connecting to ");
  LCD.setCursor(0, 1);
  LCD.print("WiFi ");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      spinner();
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  LCD.clear();
  LCD.setCursor(0, 0);
  LCD.print("IP address:");
  LCD.setCursor(0, 1);
  LCD.print(WiFi.localIP());
  
  Serial.println("Setup Multiplexer");
  
  setupLED();

  Serial.println("Startup Server");
  server.begin();

  xTaskCreate(
    &update_led_task, // task function
    "update_led_task", // task name
    2048, // stack size in words
    NULL, // pointer to parameters
    5, // priority
    NULL); // out pointer to task handle

  xTaskCreate(
    &rainbow_sequence_task,
    "sequence_task",
    2048,
    NULL,
    5,
    NULL);
}

void loop() {  
  loopWebServer();
}

void runStrip() {
  int length = selectColors.size();

  for(int i=0; i<length; i++) {
    RgbwColor color = selectColors[i];
    for(int r=0; r<LED_ROWS; r++) {
      for(int c=0; c<LED_COLUMNS; c++) {
        setColor(color, r, c);
      }
    }

    imageHaveChange = true;
    delay(1000);
  }

  Serial.println("End color loop");
}

void loopWebServer(){
  WiFiClient client = server.available();   // listen for incoming clients

  if(!client) {
    return;
  }

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // the content of the HTTP response follows the header:
            client.print("Click <a href=\"/R\">here</a> to turn the LEDs to red.<br>");
            client.print("Click <a href=\"/G\">here</a> to turn the LEDs to blue.<br>");
            client.print("Click <a href=\"/B\">here</a> to turn the LEDs to green.<br>");
            client.print("Click <a href=\"/W\">here</a> to turn the LEDs to white.<br>");
            client.print("Click <a href=\"/A\">here</a> to turn the LEDs to run colors.<br>");
            client.print("Click <a href=\"/O\">here</a> to LEDs off<br>");

            // The HTTP response ends with another blank line:
            client.println();
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (currentLine.endsWith("GET /R")) {
          selectColors = {red};
        }
        if (currentLine.endsWith("GET /G")) {
          selectColors = {green};
        }
        if (currentLine.endsWith("GET /B")) {
          selectColors = {blue};
        }
        if (currentLine.endsWith("GET /W")) {
          selectColors = {white};
        }
        if (currentLine.endsWith("GET /A")) {
          selectColors = {red, green, blue, white, black};
        }
        if (currentLine.endsWith("GET /O")) {
          selectColors = {black};
        }
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

RgbColor Wheel(uint8_t WheelPos) 
{
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) 
  {
    return RgbColor(255 - WheelPos * 3, 0, WheelPos * 3);
  } else if(WheelPos < 170) 
  {
    WheelPos -= 85;
    return RgbColor(0, WheelPos * 3, 255 - WheelPos * 3);
  } else 
  {
    WheelPos -= 170;
    return RgbColor(WheelPos * 3, 255 - WheelPos * 3, 0);
  }
}

void update_led_task(void *pvParameter) {
  strip.Begin();
  strip.Show();

  Serial.println("LED is setup");
  Serial.printf("Update interval will be: %d", updateInterval);
  Serial.println("");

  while (1) {
    if (imageHaveChange == false) {
      vTaskDelay(updateInterval / portTICK_PERIOD_MS);
    } else {
      isReading = true;
      updateStrings(); 
      isReading = false;   
      imageHaveChange = false;         
    }
  }
  vTaskDelete( NULL );
}

void setupLED() {
  pinMode(LED_ADDR_1, OUTPUT);
  pinMode(LED_ADDR_2, OUTPUT);
  pinMode(LED_ADDR_3, OUTPUT);

  digitalWrite(LED_ADDR_1, LOW);
  digitalWrite(LED_ADDR_2, LOW);
  digitalWrite(LED_ADDR_3, LOW);
}

void setColor(RgbwColor color, int r, int c) {
  image[r][c] = color;
}

void updateStrings() {
  for(int r=0; r<LED_ROWS; r++) {
    changeChannel(r);

    for(int c=0; c<LED_COLUMNS; c++) {
      RgbwColor color = image[r][c];
      strip.SetPixelColor(c, color);
    }

    strip.Show();
    delay(updateInterval);
  }
}

void changeChannel(int toChannel) {
  digitalWrite(LED_ADDR_1, (toChannel >> 0) & 1 ? HIGH : LOW);
  digitalWrite(LED_ADDR_2, (toChannel >> 1) & 1 ? HIGH : LOW);
  digitalWrite(LED_ADDR_3, (toChannel >> 2) & 1 ? HIGH : LOW);
}
