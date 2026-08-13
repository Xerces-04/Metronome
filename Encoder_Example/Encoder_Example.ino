#include <Encoder.h>

// Change these pins if using a board other than an Uno/Nano
#define ENCODER_PIN_A  11
#define ENCODER_PIN_B  4
#define BUTTON_PIN     9

// Initialize the encoder on hardware interrupt pins
Encoder myEnc(ENCODER_PIN_A, ENCODER_PIN_B);

long oldPosition  = -999;
bool lastButtonState = HIGH;

void setup() {
  Serial.begin(9600);
  Serial.println("EC11 Encoder Test Initialized:");
  
  // Configure the button pin with the internal pull-up resistor
  pinMode(BUTTON_PIN, INPUT_PULLUP);
}

void loop() {
  // 1. Read the encoder position
  // The library updates this automatically in the background using interrupts
  long newPosition = myEnc.read();
  
  // Most EC11 encoders count 4 states per physical "click" (detent)
  // Dividing by 4 gives you 1 count per physical click
  long actualClickPosition = newPosition / 4;
  
  if (actualClickPosition != oldPosition) {
    oldPosition = actualClickPosition;
    Serial.print("Position: ");
    Serial.println(actualClickPosition);
  }

  // 2. Read the push-button (Active LOW because of INPUT_PULLUP)
  bool currentButtonState = digitalRead(BUTTON_PIN);
  if (currentButtonState == LOW && lastButtonState == HIGH) {
    delay(50); // Simple debounce delay
    if (digitalRead(BUTTON_PIN) == LOW) {
      Serial.println("Button Pressed!");
    }
  }
  lastButtonState = currentButtonState;
}