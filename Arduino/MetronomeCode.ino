#include <Wire.h>
#include <Adafruit_GFX.h>
#include <DIYables_OLED_SSD1309.h>
#include <Encoder.h>

/////////////
#include "DFRobotDFPlayerMini.h"
#include <SoftwareSerial.h>
/////////////////

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
#define ENCODER_PIN_A  2
#define ENCODER_PIN_B  3
#define BUTTON_PIN     9
#define BATTERY_PIN    A1
#define VOLUME_POT_PIN A2


////////////
#define Rx 11
#define Tx 12
#define BUSY_PIN A0
/////////////

DIYables_OLED_SSD1309 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);

long oldPosition  = -999;
bool lastButtonState = HIGH;
bool active = false;
long BPM = 120;
long ballSpeed = 0;
float ballX = 64.0;          // Changed to float for smooth calculation
int ballDirection = 1;       // 1 = Right, -1 = Left
unsigned long lastMillis = 0; // Tracks the time of the last frame
int lowBatteryThreshold = 6.75;
unsigned long lastBatteryCheck = 0;
const unsigned long batteryCheckInterval = 30000; // Check every 5000ms (5 seconds)
int currentBatteryPercent = 100;
int lastVolume = -1; // Tracks previous volume to avoid spamming the DFPlayer
int currentVolume = 15; // Global variable to hold current volume (0-30)
//void CheckButtonPress();


/////////////
SoftwareSerial mySoftwareSerial(11, 12); // RX (11), TX (12)
DFRobotDFPlayerMini myDFPlayer;
/////////////

void setup() {
  delay(200);
  Serial.begin(9600);
  mySoftwareSerial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1309_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1309 allocation failed"));
    for (;;);
  }

  InitializeDisplay();
  UpdateDisplay();

  while (!myDFPlayer.begin(mySoftwareSerial)) {
    Serial.println("DFPlayer error - check wiring/SD card.");
    delay(500);
  }
  myDFPlayer.volume(30); // Volume 0 to 30

  // --- FIX: Pre-warm the SD card reader ---
  myDFPlayer.playFolder(1, 1); 
  delay(50);                  // Give it a split second to initiate communication
  myDFPlayer.stop();          // Stop it immediately

}

void loop() 
{


  CheckButtonPress();

  UpdateDisplay();
  DrawDisplay();
  if(!active)
  {
    CheckVolumeKnob(); // <-- Add this here
  }
  CheckBatteryLevel();

  //Serial.print("ballX: ");
  //Serial.println(ballX);
}



void UpdateDisplay()
{
  if (!active)
  {
    UpdateBPM();
  }
  if (active)
  {
    UpdateBall();
  }
}

void DrawDisplay()
{
  display.clearDisplay();

  DrawBPM();
  DrawBall();
  DrawBatteryIcon(112, 38, currentBatteryPercent);
  DrawVolumeIndicator(2, 54);

  display.display();      // update display
}

void InitializeDisplay()
{
  //delay(2000);          // wait for initializing
  display.clearDisplay();                // clear display
  display.setTextSize(4);                // text size = 2
  display.setTextColor(SSD1309_PIXEL_ON);  // set text color
  display.setCursor(0, 0);
  display.display();                     // update display
  //delay(2000);

}

void UpdateBPM()
{
  // 1. Read the encoder position
  // The library updates this automatically in the background using interrupts
  
  long newPosition = myEnc.read() + 480;

  if (myEnc.read() < -320)
  {
    //actualClickPosition = 40;
    myEnc.write((-320));
  } else if (myEnc.read() > 720)
  {
    //actualClickPosition = 220;
    myEnc.write((720));
  }

  // Most EC11 encoders count 4 states per physical "click" (detent)
  // Dividing by 4 gives you 1 count per physical click
  long actualClickPosition = newPosition / 4;
  if (actualClickPosition < 40)
  {
    actualClickPosition = 40;
  } else if (actualClickPosition > 300)
  {
    actualClickPosition = 300;
  }
  
  if (actualClickPosition != oldPosition) {
    oldPosition = actualClickPosition;
    BPM = actualClickPosition;
    //Serial.print("Position: ");
    //Serial.println(actualClickPosition);
  }
}

void UpdateBall()
{
  // 1. Calculate delta time (time passed since last frame in seconds)
  unsigned long currentMillis = millis();
  float deltaTime = (currentMillis - lastMillis) / 1000.0;
  lastMillis = currentMillis;

  // 2. Only move the ball if the metronome is active
  if (!active) {
    return; 
  }

  // 3. Calculate Speed: 
  // Total travel span is 122 pixels (from x=3 to x=125).
  // Speed (pixels/sec) = Distance * Beats Per Second
  float pixelsPerBeat = 122.0; 
  float ballSpeed = pixelsPerBeat * ((float)BPM / 60.0);

  // 4. Update the ball position
  ballX += ballDirection * ballSpeed * deltaTime;

  // 5. Handle wall collisions and bounce back
  if (ballX >= 125.0) {
    ballX = 125.0;            // Snap to edge
    ballDirection = -1;       // Reverse direction
    PlayClick();
  } 
  else if (ballX <= 3.0) {
    ballX = 3.0;              // Snap to edge
    ballDirection = 1;        // Reverse direction
    PlayClick();
  }
}

void CheckButtonPress()
{
  // 2. Read the push-button (Active LOW because of INPUT_PULLUP)
  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    delay(50); // Simple debounce delay
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button Pressed!");
      //PlayClick();
      active = !active;
      if (active)
      {
        lastMillis = millis();
      }else 
      {
        myEnc.write(BPM * 4 - 480);
      }
    }
  }
  lastButtonState = currentButtonState;
}


void DrawBPM()
{
  if (BPM >= 100)
  {
    display.setCursor(34, 32);              // set position to display
  } else 
  {
    display.setCursor(42, 32);              // set position to display
  }
  display.print(BPM);
}

void DrawBall()
{
  display.drawCircle(ballX, display.height() / 5, 3, SSD1309_PIXEL_ON);              // set position to display
}

void PlayClick()
{
  myDFPlayer.play(1);
}


void CheckBatteryLevel()
{
  unsigned long currentMillis = millis();

  // Only run the code inside if 5 seconds have passed
  if (currentMillis - lastBatteryCheck >= batteryCheckInterval) {
    lastBatteryCheck = currentMillis; // Reset the timer

    // Read the raw ADC value (0 to 1023)
    int rawValue = analogRead(BATTERY_PIN);
    //Serial.println(rawValue);
    // Convert ADC value to the voltage at the Pin (0V - 5V)
    float pinVoltage = (rawValue * 5.0) / 1023.0;
    
    // Calculate original battery voltage (Multiply by 2 because of the 1:1 voltage divider)
    float batteryVoltage = pinVoltage * 2.0;
    
    // Print the voltage to the Serial Monitor
    Serial.print("Battery Voltage: ");
    Serial.print(batteryVoltage);
    Serial.println(" V");
    
    // Check if battery is low
    if (batteryVoltage < lowBatteryThreshold) {
      Serial.println("⚠️ WARNING: Low Battery!");
      // Trigger your low battery feature here (e.g., flash an LED, buzz, go to sleep)
    }

    currentBatteryPercent = GetBatteryPercentage(batteryVoltage);
  }
}

int GetBatteryPercentage(float voltage) {
  // Map 6.5V - 9.0V to 0% - 100%
  int percentage = map(voltage * 100, 650, 900, 0, 100);
  
  // Constrain between 0 and 100 so it doesn't break the drawing bounds
  return constrain(percentage, 0, 100);
}

void DrawBatteryIcon(int x, int y, int percentage) {
  // 1. Draw the little positive terminal tip on the top (Width: 4, Height: 2)
  display.drawRect(x + 3, y, 4, 2, SSD1309_PIXEL_ON); 
  
  // 2. Draw the main outer box (Width: 10, Height: 20)
  display.drawRect(x, y + 2, 10, 20, SSD1309_PIXEL_ON); 
  
  // 3. Calculate how tall the inner "fill" bar should be (Max 16 pixels tall)
  int fillHeight = map(percentage, 0, 100, 0, 16);
  
  // 4. Fill the battery based on percentage (leaving a 2-pixel border)
  // Since it fills from bottom up, we need to calculate the y-coordinate of the fill
  int fillY = y + 2 + 18 - fillHeight; // Outer box Y + top padding + inner height - fill height
  display.fillRect(x + 2, fillY, 6, fillHeight, SSD1309_PIXEL_ON); 
}

void CheckVolumeKnob() {
  int rawPot = analogRead(VOLUME_POT_PIN);
  int targetVolume = map(rawPot, 0, 1010, 0, 30);
  
  if (abs(targetVolume - lastVolume) >= 1) { 
    myDFPlayer.volume(targetVolume);
    lastVolume = targetVolume;
    currentVolume = targetVolume; // <-- Save to global variable for the display
    
    Serial.print("Volume changed to: ");
    Serial.println(targetVolume);
  }
}

void DrawSpeakerIcon(int x, int y) {
  // Speaker body back rectangle
  display.fillRect(x, y + 2, 3, 4, SSD1309_PIXEL_ON);
  
  // Speaker cone (triangular shape using lines)
  display.drawLine(x + 3, y + 2, x + 6, y,     SSD1309_PIXEL_ON);
  display.drawLine(x + 3, y + 5, x + 6, y + 7, SSD1309_PIXEL_ON);
  display.drawLine(x + 6, y,     x + 6, y + 7, SSD1309_PIXEL_ON);
  
  // Sound waves (only draw if volume is greater than 0)
  if (currentVolume > 0) {
    display.drawFastVLine(x + 8, y + 2, 4, SSD1309_PIXEL_ON); // Small wave
    if (currentVolume > 15) {
      display.drawFastVLine(x + 10, y, 8, SSD1309_PIXEL_ON); // Large wave
    }
  }
}

void DrawVolumeIndicator(int x, int y) {
  // 1. Draw the speaker icon
  DrawSpeakerIcon(x + 8, y - 11);
  
  // 2. Draw the outer frame for the volume bar (Width: 32, Height: 6)
  int barX = x ; // Position bar just to the right of the speaker
  int barY = y - 1;
  display.drawRect(barX, barY, 28, 6, SSD1309_PIXEL_ON);
  
  // 3. Calculate fill width based on 0-30 volume map to 0-28 pixels inside width
  int fillWidth = map(currentVolume, 0, 30, 0, 24);
  
  // 4. Fill the bar
  display.fillRect(barX + 2, barY + 2, fillWidth, 2, SSD1309_PIXEL_ON);
}