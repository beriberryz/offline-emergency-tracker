#include <TFT_eSPI.h>
TFT_eSPI tft = TFT_eSPI();

#define TFT_BL     21
#define A9G_PWR    21   // your PWRKEY pin
#define PIN_BUTTON 35
#define MY_NUMBER  "+639189902216"

int  lineY   = 10;
bool isBusy  = false;

volatile bool btnFlag   = false;
unsigned long lastPress = 0;

void IRAM_ATTR onButton() {
  if (millis() - lastPress > 500) {
    lastPress = millis();
    btnFlag   = true;
  }
}

// ============================================================
void screenPrint(String msg, uint16_t color = TFT_WHITE) {
  if (lineY > 300) { tft.fillScreen(TFT_BLACK); lineY = 10; }
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(msg, 5, lineY);
  lineY += 14;
}

// Exact sendData from sample code — proven to work
String sendData(String command, int timeout) {
  String temp = "";
  Serial.println(command);
  long int t = millis();
  while ((t + timeout) > millis()) {
    while (Serial.available()) {
      temp += (char)Serial.read();
    }
  }
  screenPrint("> " + command, TFT_CYAN);
  screenPrint(temp.length() > 0 ? temp : "(no resp)", TFT_WHITE);
  return temp;
}

// ============================================================
//  GPS — turn on GPS, request location once
// ============================================================
String getLocation() {
  // Turn GPS on
  sendData("AT+GPS=1", 2000);

  // Request location
  screenPrint("Requesting GPS...", TFT_YELLOW);
  String res = sendData("AT+LOCATION=2", 5000);

  // Sample code checks for "GPS NOT" to detect no fix
  if (res.indexOf("GPS NOT") != -1 || res.indexOf("+LOCATION:") == -1) {
    screenPrint("No GPS fix", TFT_YELLOW);
    return "";  // empty = no fix
  }

  int idx = res.indexOf("+LOCATION:");
  if (idx == -1) return "";

  String coords = res.substring(idx + 10);
  coords.trim();
  int comma = coords.indexOf(',');
  if (comma == -1) return "";

  String lat = coords.substring(0, comma);
  String lon = coords.substring(comma + 1);
  lat.trim(); lon.trim();

  if (lat.toFloat() == 0.0 && lon.toFloat() == 0.0) return "";

  String link = "http://maps.google.com/maps?q=" + lat + "+" + lon;
  screenPrint("Fix: " + lat + "," + lon, TFT_GREEN);
  return link;
}

// ============================================================
//  SMS — exact pattern from sample code's Send_SMS()
// ============================================================
void sendSMS(String message) {
  screenPrint("Sending SMS...", TFT_YELLOW);
  screenPrint(message, TFT_WHITE);

  Serial.println("AT+CMGF=1");
  delay(1000);

  Serial.println("AT+CMGS=\"" + String(MY_NUMBER) + "\"\r");  // \r like sample code
  delay(1000);

  Serial.println(message);
  delay(1000);

  Serial.write(26);   // Ctrl-Z
  delay(1000);

  // Read result
  String resp = "";
  unsigned long t = millis();
  while (millis() - t < 10000) {
    if (Serial.available()) resp += (char)Serial.read();
  }
  resp.trim();

  screenPrint("Resp: " + resp, TFT_WHITE);

  if (resp.indexOf("+CMGS") != -1 || resp.indexOf("OK") != -1) {
    screenPrint("=== SMS SENT! ===", TFT_GREEN);
  } else {
    screenPrint("=== SMS FAILED ===", TFT_RED);
  }
}

// ============================================================
//  FULL SEQUENCE
// ============================================================
void runSequence() {
  isBusy = true;
  tft.fillScreen(TFT_BLACK);
  lineY = 10;
  screenPrint("Button held — starting...", TFT_CYAN);

  // Step 1 — GPS
  String location = getLocation();

  // Step 2 — build message
  String message;
  if (location.length() > 0) {
    message = "EMERGENCY! I'm here: " + location;
  } else {
    message = "EMERGENCY! Location could not be determined.";
  }

  // Step 3 — send SMS
  sendSMS(message);

  screenPrint("Done. Hold button to resend.", TFT_CYAN);
  isBusy = false;
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  pinMode(PIN_BUTTON, INPUT_PULLUP);

  // A9G PWRKEY — active LOW pulse
  pinMode(A9G_PWR, OUTPUT);
  digitalWrite(A9G_PWR, HIGH);
  delay(200);
  digitalWrite(A9G_PWR, LOW);
  delay(1000);
  digitalWrite(A9G_PWR, HIGH);

  Serial.begin(115200);
  screenPrint("Waiting 15s for A9G...", TFT_YELLOW);
  delay(15000);
  while (Serial.available()) Serial.read();

  // Confirm modem
  String r = sendData("AT", 2000);
  if (r.indexOf("OK") == -1) {
    screenPrint("A9G not responding!", TFT_RED);
  }

  // Turn on GPS at boot so it has time to get a fix
  sendData("AT+GPS=1", 2000);
  sendData("AT+CMGF=1", 2000);  // set SMS text mode once at boot

  // Attach interrupt — 5s hold like sample code
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), onButton, FALLING);
  btnFlag = false;

  screenPrint("Ready. Hold button 5s.", TFT_CYAN);
}

// ============================================================
//  LOOP — wait for 5s hold like sample code
// ============================================================
void loop() {
  if (!btnFlag || isBusy) return;
  btnFlag = false;

  // Count 5 seconds of hold — release early = cancel
  screenPrint("Hold for 5s to confirm...", TFT_YELLOW);
  int held = 0;
  for (int i = 0; i < 5; i++) {
    delay(1000);
    if (digitalRead(PIN_BUTTON) == HIGH) {
      // Released early — cancel
      screenPrint("Cancelled.", TFT_DARKGREY);
      return;
    }
    held++;
    screenPrint(String(5 - held) + "s...", TFT_YELLOW);
  }

  // Full 5s held — run sequence
  runSequence();
}