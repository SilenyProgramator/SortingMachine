#include <Wire.h>
#include <Servo.h>
#include "Adafruit_TCS34725.h"

// ---------- Servos ----------
Servo mixServo;  // Mixer servo
const int MIX_SERVO_PIN = 6;  // safe, independent

// ---------- Stepper pins ----------
const byte STEP_PIN = 3;   // PUL+
const byte DIR_PIN  = 4;   // DIR+
const byte EN_PIN   = 5;   // ENA+
const long FULL_STEPS_PER_REV = 200;
const int MICROSTEP = 8;
const long STEPS_PER_REV = FULL_STEPS_PER_REV * MICROSTEP;
const int POSITIONS = 5;
const long STEPS_PER_POS = 800; // adjust to real stepper distance
unsigned int US = 100;

int current_position = 0;
const char* colors[POSITIONS] = { "BROWN", "RED", "YELLOW", "GREEN", "BLUE" };

// ---------- Color sensor ----------
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_50MS,
  TCS34725_GAIN_4X
);

// ---------- Helper ----------
float clamp01(float x) { return (x < 0) ? 0 : (x > 1) ? 1 : x; }

const char* guessColor(uint8_t R, uint8_t G, uint8_t B) {
  struct ColorRef { const char* name; uint8_t r, g, b; };
  ColorRef refs[] = {
    {"BLUE", 107, 81, 82},
    {"GREEN", 124, 97, 46},
    {"YELLOW", 153, 73, 33},
    {"VIOLET", 167, 63, 43},
    {"ORANGE", 177, 55, 34},
    {"RED", 189, 49, 35},
    {"BROWN", 176, 61, 38}
  };

  float brightness = max(R, max(G, B)) / 255.0;
  if (brightness < 0.08) return "BLACK / DARK";
  if (brightness > 0.90) return "WHITE / GRAY";

  const char* bestName = "UNKNOWN";
  float bestDist = 1e9;
  for (auto &ref : refs) {
    float dR = R - ref.r, dG = G - ref.g, dB = B - ref.b;
    float dist = dR*dR + dG*dG + dB*dB;
    if (dist < bestDist) { bestDist = dist; bestName = ref.name; }
  }
  if (bestDist > 2000) return "UNKNOWN";
  return bestName;
}

// ---------- Stepper helpers ----------
inline void driverOn()  { digitalWrite(EN_PIN, HIGH); }
inline void driverOff() { digitalWrite(EN_PIN, LOW); }

inline void pulse() {
  digitalWrite(STEP_PIN, HIGH); delayMicroseconds(US);
  digitalWrite(STEP_PIN, LOW ); delayMicroseconds(US);
}

void moveSteps(long n, bool dirHigh) {
  digitalWrite(DIR_PIN, dirHigh ? HIGH : LOW);
  for (long i = 0; i < n; i++) pulse();
}

int colorToIndex(const String& cmd) {
  for (int i = 0; i < POSITIONS; i++)
    if (cmd.equalsIgnoreCase(colors[i])) return i;
  return -1;
}

// ---------- Setup ----------
void setup() {
  Serial.begin(115200);

  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN, OUTPUT);
  pinMode(EN_PIN, OUTPUT);
  driverOn();

  mixServo.attach(MIX_SERVO_PIN);
  mixServo.write(90); // neutral

  if (!tcs.begin()) {
    Serial.println("ERROR: TCS34725 not found!");
    while (1);
  }

  Serial.println("System ready!");
}

// ---------- Main loop ----------
void loop() {
  // Slowly move mixer servo
  mixServo.write(0);
  delay(500);
  mixServo.write(90);
  delay(500);

  // Read color
  uint16_t r, g, b, c;
  tcs.getRawData(&r, &g, &b, &c);
  if (c == 0) c = 1;
  int R8 = (int)(clamp01(r / (float)c) * 255.0);
  int G8 = (int)(clamp01(g / (float)c) * 255.0);
  int B8 = (int)(clamp01(b / (float)c) * 255.0);

  const char* name = guessColor(R8, G8, B8);
  Serial.print("Detected color: ");
  Serial.println(name);

  // Move stepper to corresponding color bin
  int target = colorToIndex(String(name));
  if (target >= 0) {
    int distance = target - current_position;
    if (distance > 0) moveSteps(STEPS_PER_POS * distance, true);
    else if (distance < 0) moveSteps(STEPS_PER_POS * -distance, false);
    current_position = target;
  }

  delay(500); // short pause before next reading
}
