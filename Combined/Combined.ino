#include <Wire.h>
#include <Adafruit_MCP4725.h>
#include <Adafruit_GFX.h>
#include <DIYables_OLED_SSD1309.h>
#include <Encoder.h>

// --- Display Configuration ---
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

// --- Hardware Pins ---
#define ENCODER_PIN_A  2
#define ENCODER_PIN_B  3
#define BUTTON_PIN     9

// --- Object Instantiations ---
Adafruit_MCP4725 dac;
DIYables_OLED_SSD1309 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);

// --- State Variables ---
long oldPosition  = -999;
bool lastButtonState = HIGH;
bool active = false;
long BPM = 120;

// --- Animation Variables ---
float ballX = 64.0;          // Float for smooth calculation
int ballDirection = 1;       // 1 = Right, -1 = Left
unsigned long lastMillis = 0; // Tracks the time of the last frame

// Toggle this to switch between tick (downbeat) and tock (upbeat)
bool isAccent = true;

// --- Function Declarations ---
void InitializeDisplay();
void UpdateDisplay();
void DrawDisplay();
void UpdateBPM();
void UpdateBall();
void CheckButtonPress();
void DrawBPM();
void DrawBall();
void playClick(bool accent);

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize I2C and DAC
  Wire.begin();
  Wire.setClock(400000);
  dac.begin(0x60);
  dac.setVoltage(2048, false);
  randomSeed(analogRead(A0)); // Seed for noise generation

  // Initialize OLED Display
  if (!display.begin(SSD1309_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1309 allocation failed"));
    for (;;);
  }

  InitializeDisplay();
  UpdateDisplay();
}

void loop() {
  CheckButtonPress();
  UpdateDisplay();
  DrawDisplay();

  Serial.print("ballX: ");
  Serial.println(ballX);
}

void UpdateDisplay() {
  if (!active) {
    UpdateBPM();
  }
  if (active) {
    UpdateBall();
  }
}

void DrawDisplay() {
  display.clearDisplay();
  DrawBPM();
  DrawBall();
  display.display(); 
}

void InitializeDisplay() {
  delay(1000); 
  display.clearDisplay();                
  display.setTextSize(4);                
  display.setTextColor(SSD1309_PIXEL_ON);  
  display.setCursor(0, 0);
  display.display();                     
  delay(1000);
}

void UpdateBPM() {
  long newPosition = myEnc.read() + 480;

  if (myEnc.read() < -320) {
    myEnc.write((-320));
  } else if (myEnc.read() > 400) {
    myEnc.write((400));
  }

  long actualClickPosition = newPosition / 4;
  if (actualClickPosition == 39) {
    actualClickPosition = 40;
  }
  
  if (actualClickPosition != oldPosition) {
    oldPosition = actualClickPosition;
    BPM = actualClickPosition;
  }
}

void UpdateBall() {
  unsigned long currentMillis = millis();
  float deltaTime = (currentMillis - lastMillis) / 1000.0;
  lastMillis = currentMillis;

  if (!active) {
    return; 
  }

  // Distance travel span is 122 pixels (from x=3 to x=125)
  float pixelsPerBeat = 122.0; 
  float ballSpeed = pixelsPerBeat * ((float)BPM / 60.0);

  // Update ball position
  ballX += ballDirection * ballSpeed * deltaTime;

  // Handle wall collisions and trigger audio click
  if (ballX >= 125.0) {
    ballX = 125.0;            // Snap to edge
    ballDirection = -1;       // Reverse direction
    playClick(isAccent);      // Trigger audio click on visual impact
    // isAccent = !isAccent;  // Uncomment to alternate audio accent style
  } 
  else if (ballX <= 3.0) {
    ballX = 3.0;              // Snap to edge
    ballDirection = 1;        // Reverse direction
    playClick(isAccent);      // Trigger audio click on visual impact
    // isAccent = !isAccent;  // Uncomment to alternate audio accent style
  }
}

void CheckButtonPress() {
  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button Pressed!");
      active = !active;
      if (active) {
        lastMillis = millis();
      } else {
        myEnc.write(BPM * 4 - 480);
      }
    }
  }
  lastButtonState = currentButtonState;
}

void DrawBPM() {
  if (BPM >= 100) {
    display.setCursor(30, 32);              
  } else {
    display.setCursor(42, 32);              
  }
  display.print(BPM);
}

void DrawBall() {
  display.drawCircle((int)ballX, display.height() / 5, 3, SSD1309_PIXEL_ON);              
}

// --- Audio Generation Function ---
void playClick(bool accent) {
  int duration = 100;
  float filtered = 2048.0;

  for (int i = 0; i < duration; i++) {
    float amplitude = exp(-0.05 * i);
    float noise = (random(0, 2048) - 1024) / 1024.0;
    float target = 2048.0 + (2047.0 * amplitude * noise);
    filtered = filtered + 0.25 * (target - filtered); 
    dac.setVoltage((uint16_t)constrain(filtered, 0, 4095), false);
    delayMicroseconds(40);
  }
  dac.setVoltage(2048, false); // Reset to baseline
}