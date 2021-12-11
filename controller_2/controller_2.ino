#include <ESP8266WiFi.h>
#include "SoftwareSerial.h"
#include <Wire.h>
#include <ArduCAM.h>
#include <SPI.h>
#include "memorysaver.h"

// https://github.com/ArduCAM/Arduino/blob/master/ArduCAM/examples/mini/ArduCAM_Mini_2MP_Plus_VideoStreaming/ArduCAM_Mini_2MP_Plus_VideoStreaming.ino

const int LED_PIN = D8;

const int SOFT_SERIAL_TX = D3;
const int SOFT_SERIAL_RX = D4;

const int CS = D0;
ArduCAM myCAM(OV2640, CS);
const size_t BUFFER_SIZE = 4096;
uint8_t buffer[BUFFER_SIZE] = {};

SoftwareSerial softSerial(SOFT_SERIAL_RX, SOFT_SERIAL_TX, false);

const char *WIFI_SSID = "<REDACTED>";
const char *WIFI_PASSWORD = "<REDACTED>";

const char *SERVER_HOST = "192.168.31.245";
const int SERVER_PORT = 5000;

const uint32_t CHIP_ID = ESP.getChipId();

bool isFull = false;
bool ledActive = false;

void setup() {
  Serial.begin(9600);
  softSerial.begin(4800);
  delay(1000);
  Serial.println("Console OK");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("WiFi connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());

  Wire.begin();

  // set the CS as an output:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  // set the CS as an output:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);

  // initialize SPI
  SPI.begin();
  digitalWrite(SS, LOW);

  //Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);

  while (1) {
    uint8_t temp;

    //Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);

    if (temp != 0x55) {
      Serial.println("SPI Error!!!");
      delay(1000);
    } else {
      Serial.println("SPI OK");
      break;
    }
  }

  while (1) {
    uint8_t vid, pid;

    //Check if the camera module type is OV2640
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    
    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))) {
      Serial.println("Can't find OV2640 module!!!");
      delay(1000);
    }
    else {
      Serial.println("OV2640 detected");
      break;
    }
  }

  myCAM.set_format(JPEG);
  myCAM.InitCAM();
  //myCAM.OV2640_set_JPEG_size(OV2640_320x240);
  //myCAM.OV2640_set_JPEG_size(OV2640_640x480);
  myCAM.OV2640_set_JPEG_size(OV2640_1024x768);

  delay(1000);
  myCAM.clear_fifo_flag();

  pinMode(LED_PIN, OUTPUT);

  Serial.println("Setup OK");
}

void loop() {
  uint16_t weight = 0, util = 0;
  WiFiClient client;

  delay(500);

  ledActive = isFull ? (not ledActive) : false;
  digitalWrite(LED_PIN, ledActive ? HIGH : LOW);

  if (softSerial.available() <= 0)
    return;
  
  if (softSerial.read() == 0xFF && softSerial.read() == 0xFE) {
    softSerial.readBytes((uint8_t*)&weight, sizeof(weight));
    softSerial.readBytes((uint8_t*)&util, sizeof(util));
    Serial.printf("Weight = %hu, Util: %hu\n", weight, util);

    isFull = util > 80;
  } else {
    Serial.println("Broken data from controller 1");
    return;
  }

  if (!client.connect(SERVER_HOST, SERVER_PORT)) {
    Serial.println("Cannot connect to server");
    return;
  }

  Serial.println("Connected to server");

  uint8_t temp = 0xff, temp_last = 0;
  uint32_t length = 0;

  myCAM.flush_fifo();
  myCAM.clear_fifo_flag();

  Serial.println("Capturing...");
  myCAM.start_capture();
  while (!myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
    continue;
  length = myCAM.read_fifo_length();
  Serial.printf("JPEG size: %u bytes...\n", length);

  myCAM.CS_LOW();
  myCAM.set_fifo_burst();

  client.println("POST /update HTTP/1.1");
  client.println("Host: " + String(SERVER_HOST));
  client.println("Connection: close");
  client.println("Content-Type: application/octet-stream");
  client.println("Content-Length: " + String(8 + length));
  client.println();
  client.write((uint8_t*)&CHIP_ID, sizeof(CHIP_ID));
  client.write((uint8_t*)&weight, sizeof(weight));
  client.write((uint8_t*)&util, sizeof(util));

  while (length && client.connected()) {
    size_t will_copy = (length < BUFFER_SIZE) ? length : BUFFER_SIZE;
    SPI.transferBytes(buffer, buffer, will_copy);
    client.write(&buffer[0], will_copy);
    length -= will_copy;
  }

  int retVal = 0;
  while (client.connected())
    if (client.available()) {
      retVal <<= 8;
      retVal += client.read();
      if (retVal == 0x0d0a0d0a) break;
    }

  retVal = 0;
  while (client.connected())
    if (client.available()) {
      retVal = client.read();
      break;
    }

  Serial.printf("Server returned: %d\n", retVal);
  if (retVal == 0xFF)
    for (int i = 0; i < 20; ++i) {
      digitalWrite(LED_PIN, i % 2 ? LOW : HIGH);
      delay(100);
    }

  myCAM.CS_HIGH();
  client.stop();
  Serial.println("Connection closed");
}
