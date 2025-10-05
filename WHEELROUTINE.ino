#include <Servo.h>

Servo s;

const int SERVO_PIN = 9;
const int SWITCH_PIN = 2;   // signal input pin
const int RUN_US = 1100;    // slow rotation
const int STOP_US = 1500;   // neutral (stop) position
const int SKIP_AFTER_RESUME_MS = 200; // move ~0.3 rev

void setup() {
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  s.attach(SERVO_PIN);
  s.writeMicroseconds(RUN_US);
}

void loop() {
  if (digitalRead(SWITCH_PIN) == HIGH) {
    s.writeMicroseconds(STOP_US);
    delay(1500);  // adjust pause as needed
    s.writeMicroseconds(RUN_US);
    unsigned long start = millis();
    while (millis() - start < SKIP_AFTER_RESUME_MS) { }
  }
}