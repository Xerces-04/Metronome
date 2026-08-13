/*
 * Created by ArduinoGetStarted.com
 *
 * This example code is in the public domain
 *
 * Tutorial page: https://arduinogetstarted.com/tutorials/arduino-ssd1309-oled-display
 */

/*
 * DIYables OLED SSD1309 – Display Text Demo
 * Demonstrates text display features on 2.42" SSD1309 128x64 I2C OLED.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <DIYables_OLED_SSD1309.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

DIYables_OLED_SSD1309 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(9600);

  if (!display.begin(SSD1309_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1309 allocation failed"));
    for (;;);
  }

  delay(2000);          // wait for initializing
  display.clearDisplay(); // clear display

  display.setTextSize(1);                // text size = 1
  display.setTextColor(SSD1309_PIXEL_ON);  // set text color
  display.setCursor(0, 10);              // set position to display
  display.println(F("Text size = 1"));   // display text
  display.display();                     // update display
  delay(2000);

  display.setTextSize(2);                // text size = 2
  display.setCursor(0, 30);              // set position to display
  display.println(F("Size = 2"));        // display text
  display.display();                     // update display
  delay(2000);

  display.clearDisplay(); // clear display
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.println(F("Display integer:"));
  display.println(12345);
  display.println();
  display.println(F("Display float:"));
  display.println(1.2345);
  display.println();
  display.println(F("Display HEX:"));
  display.println(0xABCD, HEX);
  display.display();                     // update display
}

void loop() {
}
