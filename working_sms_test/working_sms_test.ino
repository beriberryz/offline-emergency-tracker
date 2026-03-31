#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define TFT_BL 21
#define MY_NUMBER "+639189902216"

// A9G Serial (IMPORTANT FIX)
HardwareSerial A9G(2); // Use UART2

int lineY = 10;

// 📺 Display function
void screenPrint(String msg, uint16_t color = TFT_WHITE) {
  if (lineY > 300) {
    tft.fillScreen(TFT_BLACK);
    lineY = 10;
  }
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(msg, 5, lineY);
  lineY += 14;
}

// 📡 Send AT command
String sendAT(String cmd, int waitMs) {
  while (A9G.available()) A9G.read();

  screenPrint(">> " + cmd, TFT_CYAN);
  A9G.println(cmd);

  String response = "";
  unsigned long t = millis();

  while (millis() - t < waitMs) {
    while (A9G.available()) {
      response += (char)A9G.read();
    }
  }

  response.trim();

  if (response.length() > 0) {
    screenPrint(response, TFT_GREEN);
  } else {
    screenPrint("(no response)", TFT_RED);
  }

  return response;
}

void setup() {
  // 🔆 TFT Setup
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // 🔍 Debug Serial (USB)
  Serial.begin(115200);

  // 📡 A9G Serial (IMPORTANT)
  A9G.begin(115200, SERIAL_8N1, 16, 17); 
  // RX = GPIO16, TX = GPIO17

  screenPrint("Powering A9G...", TFT_YELLOW);

  // ⏱️ Give A9G enough time
  delay(20000);

  screenPrint("=== SMS Test ===", TFT_YELLOW);

  // 🧪 Basic checks
  sendAT("AT", 2000);
  sendAT("AT+CSQ", 2000);
  sendAT("AT+CREG?", 2000);

  // 📩 SMS setup
  sendAT("AT+CMGF=1", 2000);

  // 📤 Send SMS
  sendAT("AT+CMGS=\"" + String(MY_NUMBER) + "\"", 3000);

  screenPrint("Sending body...", TFT_YELLOW);

  A9G.print("TEST FROM ESP32");
  delay(500);
  A9G.write(26); // CTRL+Z

  // ⏳ Wait for response
  String resp = "";
  unsigned long t = millis();

  while (millis() - t < 10000) {
    while (A9G.available()) {
      resp += (char)A9G.read();
    }
  }

  resp.trim();

  if (resp.indexOf("OK") != -1 || resp.indexOf("+CMGS") != -1) {
    screenPrint("SMS SENT!", TFT_GREEN);
  } else {
    screenPrint("SMS FAILED", TFT_RED);
    screenPrint(resp.length() > 0 ? resp : "(no response)", TFT_RED);
  }
}

void loop() {
  // nothing
}