#include "HX711.h"
#include "SoftwareSerial.h"

const int SOFT_SERIAL_TX = D3;
const int SOFT_SERIAL_RX = D4;
const int FLEX_PIN = A0;
const int ULTRASONIC_TRIG_PIN = D6;
const int ULTRASONIC_ECHO_PIN = D5;
const int LOADCELL_DOUT_PIN = D1;
const int LOADCELL_SCK_PIN = D0;
const int FLEX_THRESHOLD = 100;

const long LOADCELL_OFFSET = -500;
const long LOADCELL_DIVIDER = 442;

HX711 loadcell;
SoftwareSerial softSerial(SOFT_SERIAL_RX, SOFT_SERIAL_TX, false);
uint16_t previousWeight, previousUtil;

void setup() {
  Serial.begin(9600);
  softSerial.begin(4800);

  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  //loadcell.set_scale();
  loadcell.set_scale(LOADCELL_DIVIDER);
  loadcell.set_offset(LOADCELL_OFFSET);
  //loadcell.tare();

  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);

  pinMode(FLEX_PIN, INPUT);

  delay(1000);
  Serial.println("Init OK");

  previousWeight = get_weight();
  previousUtil = get_utilization();
}

void loop() {
  int flexVal = analogRead(FLEX_PIN);
  Serial.printf("Flex value: %d\n", flexVal);

  if (flexVal < FLEX_THRESHOLD) {
    //Serial.println("Lid is closed");
    delay(1000);
    return;
  }

  Serial.print("Lid is open. Waiting...");
  while (flexVal >= FLEX_THRESHOLD) {
    Serial.print(".");
    delay(100);
    flexVal = analogRead(FLEX_PIN);
  }

  delay(500);
  Serial.println("Lid is closed. Collect sensor values");

  uint16_t weight = get_weight();
  uint16_t util = get_utilization();

  // Calibrate utilization
  if (weight > previousWeight && util < previousUtil)
      util = previousUtil;
  if (weight < 10 && util < 10)
      util = 0;

  previousWeight = weight;
  previousUtil = util;

  // Prints the distance on the Serial Monitor
  Serial.printf("Util: %hu%%\n", util);
  Serial.printf("Weight: %hu\n", weight);

  softSerial.write(0xFF);
  softSerial.write(0xFE);
  softSerial.write((uint8_t*)&weight, sizeof(weight));
  softSerial.write((uint8_t*)&util, sizeof(util));

  delay(1000);
}

uint16_t safe_cast_float_to_u16(float val, uint16_t limit) {
  if (val >= limit)
    return limit;
  else if (val < 0)
    return 0;
  else
    return (uint16_t)val;
}

uint16_t get_weight() {
  // Read weight in gram, max = 5kg
  float weight = loadcell.get_units(10);
  return safe_cast_float_to_u16(weight, 5000);
}

uint16_t get_utilization() {
  // Clears the trigPin
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  float duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH);

  // Calculate the distance in cm
  float distance = duration * 0.034 / 2;

  // 24cm -> 0%, 4cm -> 100%
  float util = min(max(24 - distance, 0.0) / 20, 1.0);

  return safe_cast_float_to_u16(util * 100, 100);
}
