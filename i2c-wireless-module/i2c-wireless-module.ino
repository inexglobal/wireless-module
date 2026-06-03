#include "Wire.h"
#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define I2C_DEV_ADDR 0x55  // กำหนด Address ประจำอุปกรณ์
#define LED_PIN 8

// Time
uint32_t time_loop = 0;
uint32_t time_send = 0;
bool st_led = true;

uint8_t broadcastAddress[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };  // แก้ไขเป็น MAC ตัวรับ หรือ ส่งแบบกระจาย
uint8_t i2cBuffer[64];
int dataLength = 0;  // เก็บความยาวข้อมูล (ไม่ใช่ volatile)

// ฟังก์ชันรับข้อมูล I2C แบบเรียบง่าย
void onReceive(int len) {
  dataLength = 0;
  while (Wire.available() && dataLength < sizeof(i2cBuffer)) {
    i2cBuffer[dataLength++] = Wire.read();
  }
}

// แสดงข้อมูลทั้งหมดใน i2cBuffer ด้วย Serial.printf
void printI2CBuffer(uint8_t *buffer, int length) {
  Serial.printf("Buffer length = %d bytes\n", length);
  for (int i = 0; i < length; i++) {
    Serial.printf("Byte[%02d] = %3d (0x%02X)\n", i, buffer[i], buffer[i]);
  }
}

// Callback เมื่อส่งข้อมูลเสร็จ
void onDataSent(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  Serial.print("Send Status: ");
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Success");
  } else {
    Serial.println("Fail");
  }
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  pinMode(LED_PIN, OUTPUT);
  if (esp_now_init() != ESP_OK) return;

  //ตั้งกำลังส่ง
  int8_t power = 44;
  esp_err_t my_error = esp_wifi_set_max_tx_power(power);

  if (my_error == ESP_OK) {
    Serial.println("Set TX Power Successfully!");
  }

  esp_now_register_send_cb(onDataSent);
  esp_now_peer_info_t peerInfo;
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  // ตั้งค่า I2C Slave
  Wire.onReceive(onReceive);
  //
  // 20/sda/RX , 21/sdl/TX
  Wire.setPins(20, 21);
  Wire.begin((uint8_t)I2C_DEV_ADDR);
  digitalWrite(LED_PIN, HIGH);
}
void loop() {
  if (dataLength) {
    // แสดงผลออก Serial
    printI2CBuffer(i2cBuffer, dataLength);
    // Send message via ESP-NOW
    if (millis() - time_send > 5) {
      esp_err_t result = esp_now_send(broadcastAddress, i2cBuffer, dataLength);
      if (result == ESP_OK) {
        Serial.println("Sent with success");
        st_led = !st_led;
        digitalWrite(LED_PIN, st_led);
        time_send = millis();
        dataLength = 0;  // เคลียร์ความยาวกลับเป็น 0 เพื่อรอรับชุดถัดไป
      } else {
        Serial.println("Error sending the data");
      }
    }
    time_loop = millis();
  }

  if (millis() - time_loop > 500) {
    digitalWrite(LED_PIN, HIGH);
  }
}