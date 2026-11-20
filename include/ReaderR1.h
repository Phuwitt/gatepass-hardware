#pragma once
// PN532 on the ORIGINAL R1 J5; separate SPI bus from the LCD.
#if 1
#include <SPI.h>
#include <Adafruit_PN532.h>


SPIClass readerSpi(HSPI);
Adafruit_PN532 reader(R1::nfcCs, &readerSpi);
bool readerReady = false;
uint32_t lastRead = 0;
String lastUid;

bool cardReaderReady() { return readerReady; }

void startReader() {
  readerSpi.begin(R1::nfcSck, R1::nfcMiso, R1::nfcMosi, R1::nfcCs);
  reader.begin();
  if (!reader.getFirmwareVersion()) {
    Serial.println("PN532 not detected. Check SPI mode, pins and power. NO unlock.");
    return;
  }
  readerReady = reader.SAMConfig();
  reader.setPassiveActivationRetries(0x01);
  Serial.printf("PN532 ready: %s\r\n", readerReady ? "yes" : "no");
}

bool sendReaderEvent(const char* path, const char* field, const char* value, bool capture) {
  cJSON* root = cJSON_CreateObject();
  if (!root) return false;
  cJSON_AddStringToObject(root, field, value);
  if (capture) cJSON_AddStringToObject(root, "session", cardSession.c_str());
  char* json = cJSON_PrintUnformatted(root);
  cJSON_Delete(root);
  if (!json) return false;
  String out;
  const String rid = capture ? "" : requestId();
  int status = post(path, (uint8_t*)json, strlen(json), "application/json", out, rid);
  if (!capture && (status < 0 || status == 202)) {
    delay(200);
    status = post(path, (uint8_t*)json, strlen(json), "application/json", out, rid);
  }
  memset(json, 0, strlen(json));
  cJSON_free(json);
  Serial.printf("Reader HTTP %d\r\n", status);
  if (!capture && status == 200) handleDecision(out);
  if (capture && status == 200) screenText("CARD CAPTURED", "Confirm enrollment on the admin web.",teal,4000);
  if (status != 200) screenText("READER ERROR",String("HTTP ")+status+"\nNo access granted.",ILI9341_RED,4000);
  return status == 200 || status == 409;
}

bool handlePhoneTap() {
  // Adafruit_PN532 sets its target number for APDU exchange in inListPassiveTarget().
  if (!reader.inListPassiveTarget()) return false;
  uint8_t selectAid[] = {0x00, 0xA4, 0x04, 0x00, 0x07, 0xF0, 0x47, 0x41, 0x54, 0x45, 0x50, 0x41, 0x00};
  uint8_t reply[48] = {};
  uint8_t length = sizeof(reply);
  if (!reader.inDataExchange(selectAid, sizeof(selectAid), reply, &length) ||
      length != 2 || reply[0] != 0x90 || reply[1] != 0x00) {
    return false;
  }

  uint8_t getCredential[] = {0x80, 0xCA, 0x00, 0x00, 0x20};
  length = sizeof(reply);
  if (!reader.inDataExchange(getCredential, sizeof(getCredential), reply, &length) ||
      length != 34 || reply[32] != 0x90 || reply[33] != 0x00) {
    Serial.println("GATEPASS NFC detected. Open the app, prepare the pass and tap again.");
    return true;
  }

  char credential[65] = {};
  const char* hex = "0123456789abcdef";
  for (size_t i = 0; i < 32; ++i) {
    credential[2 * i] = hex[reply[i] >> 4];
    credential[2 * i + 1] = hex[reply[i] & 0x0F];
  }
  sendReaderEvent("/api/device/nfc", "credential", credential, false);
  return true;
}

void pollReader() {
  if (!readerReady || millis() - lastRead < 300) return;
  lastRead = millis();
  uint8_t uid[10] = {}, length = 0;
  if (!reader.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &length, 50)) {
    lastUid = "";
    return;
  }
  if (length != 4 && length != 7 && length != 10) return;
  String value;
  for (uint8_t i = 0; i < length; ++i) {
    char hex[3];
    snprintf(hex, sizeof(hex), "%02X", uid[i]);
    value += hex;
  }
  if (value == lastUid) return;
  activeQrToken="";
  lastUid = value;
  const bool capture = cardSession.length() && (int32_t)(millis() - cardSessionUntil) < 0;
  if (!capture && handlePhoneTap()) return;
  const char* path = capture ? "/api/device/card-capture" : "/api/device/card";
  if (sendReaderEvent(path, "uid", value.c_str(), capture) && capture) cardSession = "";
}
#else
bool cardReaderReady() { return false; }
void startReader() {}
void pollReader() {}
#endif
