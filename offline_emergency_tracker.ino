#include <TFT_eSPI.h>
#include <Preferences.h>

TFT_eSPI tft = TFT_eSPI();
Preferences prefs;

// ============================================================
//  PIN CONFIGURATION — change these to match your wiring
// ============================================================
#define TFT_BL        21
#define PIN_BUTTON    35   // GPIO for hardware emergency button
#define PIN_BUZZER    25   // GPIO for buzzer
#define A9G_TX        17   // ESP32 TX → A9G RX
#define A9G_RX        16   // ESP32 RX → A9G TX
#define A9G_BAUD      115200
// ============================================================

#define SCREEN_W 480
#define SCREEN_H 320

#define TOUCH_X_MIN 1
#define TOUCH_X_MAX 479
#define TOUCH_Y_MIN 17
#define TOUCH_Y_MAX 302

// ---------------- SYSTEM STATES ----------------
enum SystemState {
  STATE_START,
  STATE_REGISTER,
  STATE_MAIN,
  STATE_CHANGE,
  STATE_EMERGENCY
};
SystemState sysState = STATE_START;

// ---------------- DATA ----------------
String userName  = "";
String nameInput = "";
bool   firstBoot = true;

// ---------------- GPS / GSM STATUS ----------------
String gpsStatus = "No Fix";
String gsmStatus = "No Signal";
String sysStatus = "Ready";
float  gpsLat    = 0.0;
float  gpsLon    = 0.0;

// ---------------- EMERGENCY ----------------
volatile bool emergencyTriggered = false;
unsigned long lastButtonPress    = 0;
bool          buzzerActive       = false;
unsigned long lastBuzzerToggle   = 0;
bool          buzzerState        = false;
bool          stopRequested      = false;

// ---------------- UI ----------------
bool          needsRedraw   = true;
bool          cursorVisible = true;
unsigned long lastTouch     = 0;
unsigned long lastBlink     = 0;

// ---------------- BUTTON STRUCT ----------------
struct Button {
  int x, y, w, h;
  String label;
};

Button keys[40];
int    keyCount  = 0;
Button enterBtn;
Button delBtn;
Button spaceBtn;
Button signUpBtn;
Button changeBtn;
Button retryBtn;
Button stopBtn;
Button backBtn;

// ============================================================
//  TOUCH MAPPING
// ============================================================
int fixX(int rawX) {
  return constrain(map(rawX, TOUCH_X_MAX, TOUCH_X_MIN, 0, SCREEN_W), 0, SCREEN_W);
}
int fixY(int rawY) {
  return constrain(map(rawY, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H), 0, SCREEN_H);
}
bool hit(Button b, int x, int y) {
  return (x >= b.x && x <= b.x + b.w &&
          y >= b.y && y <= b.y + b.h);
}

// ============================================================
//  KEYBOARD
// ============================================================
void addKey(int x, int y, int w, int h, String label) {
  if (keyCount < 40) keys[keyCount++] = {x, y, w, h, label};
}

void buildKeyboard() {
  keyCount = 0;

  int keyW    = 42;
  int keyH    = 40;
  int gap     = 5;
  int startY  = 95;
  int startX  = 10;
  int kbWidth = SCREEN_W - 20;

  auto rowStartX = [&](int count) {
    int rowWidth = count * keyW + (count - 1) * gap;
    return startX + (kbWidth - rowWidth) / 2;
  };

  const char* r1 = "QWERTYUIOP";
  int x1 = rowStartX(10);
  for (int i = 0; i < 10; i++)
    addKey(x1 + i * (keyW + gap), startY, keyW, keyH, String(r1[i]));

  const char* r2 = "ASDFGHJKL";
  int x2 = rowStartX(9);
  for (int i = 0; i < 9; i++)
    addKey(x2 + i * (keyW + gap), startY + keyH + gap, keyW, keyH, String(r2[i]));

  const char* r3 = "ZXCVBNM";
  int x3 = rowStartX(7);
  for (int i = 0; i < 7; i++)
    addKey(x3 + i * (keyW + gap), startY + 2 * (keyH + gap), keyW, keyH, String(r3[i]));

  int row4Y     = startY + 3 * (keyH + gap) + 4;
  int smallBtnW = 80;
  int spaceW    = kbWidth - 2 * smallBtnW - 2 * gap;

  delBtn   = {startX,                                  row4Y, smallBtnW, 38, "DEL"};
  spaceBtn = {startX + smallBtnW + gap,                row4Y, spaceW,    38, "SPACE"};
  enterBtn = {startX + smallBtnW + gap + spaceW + gap, row4Y, smallBtnW, 38, "OK"};
}

void drawKey(Button b, uint16_t bgColor = TFT_DARKGREY) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 6, bgColor);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 6, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setTextSize(1);
  tft.drawString(b.label, b.x + b.w / 2, b.y + b.h / 2);
}

// ============================================================
//  TEXT FIELD
// ============================================================
void drawCursorOnly(bool visible) {
  int boxX        = 30;
  int boxY        = 30;
  int boxH        = 50;
  int textPadding = 10;
  tft.setTextSize(2);
  int cursorX = (nameInput.length() == 0)
    ? boxX + textPadding
    : boxX + textPadding + tft.textWidth(nameInput) + 2;
  tft.fillRect(cursorX, boxY + 10, 2, boxH - 20, visible ? TFT_WHITE : TFT_BLACK);
}

void drawTextField() {
  int boxX        = 30;
  int boxY        = 30;
  int boxW        = SCREEN_W - 60;
  int boxH        = 50;
  int textPadding = 10;

  tft.fillRect(boxX + 2, boxY + 2, boxW - 4, boxH - 4, TFT_BLACK);
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 6, TFT_WHITE);
  tft.setTextDatum(ML_DATUM);
  tft.setTextSize(2);

  if (nameInput.length() == 0) {
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.drawString("Type your name here...", boxX + textPadding, boxY + boxH / 2);
  } else {
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(nameInput, boxX + textPadding, boxY + boxH / 2);
  }
  drawCursorOnly(cursorVisible);
}

// ============================================================
//  SCREENS
// ============================================================

void drawStartScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(4);
  tft.drawString("Hello!", SCREEN_W / 2, SCREEN_H / 2 - 50);

  int btnW = 160, btnH = 50;
  int btnX = (SCREEN_W - btnW) / 2;
  int btnY = SCREEN_H / 2 + 20;
  signUpBtn = {btnX, btnY, btnW, btnH, "Sign Up"};
  tft.fillRoundRect(btnX, btnY, btnW, btnH, 10, TFT_CYAN);
  tft.setTextColor(TFT_BLACK, TFT_CYAN);
  tft.setTextSize(2);
  tft.drawString("Sign Up", btnX + btnW / 2, btnY + btnH / 2);
}

void drawInputScreen() {
  tft.fillScreen(TFT_BLACK);
  drawTextField();
  buildKeyboard();
  tft.setTextSize(1);
  for (int i = 0; i < keyCount; i++) drawKey(keys[i]);
  drawKey(spaceBtn, TFT_NAVY);
  drawKey(delBtn,   0x8000);
  drawKey(enterBtn, 0x03E0);
}

void drawMainScreen() {
  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, SCREEN_W, 45, TFT_NAVY);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(2);
  tft.drawString("Hi, " + userName, 15, 22);

  tft.drawRoundRect(10, 55, SCREEN_W - 20, 55, 8, TFT_DARKGREY);
  tft.setTextSize(1);

  uint16_t sysColor = TFT_GREEN;
  if (sysStatus == "Sending...")  sysColor = TFT_YELLOW;
  if (sysStatus == "Failed")      sysColor = TFT_RED;
  if (sysStatus == "Sent!")       sysColor = TFT_GREEN;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Status:", 20, 70);
  tft.setTextColor(sysColor, TFT_BLACK);
  tft.drawString(sysStatus, 90, 70);

  uint16_t gpsColor = (gpsStatus == "Fix") ? TFT_GREEN : TFT_RED;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("GPS:", 20, 90);
  tft.setTextColor(gpsColor, TFT_BLACK);
  tft.drawString(gpsStatus, 90, 90);

  uint16_t gsmColor = (gsmStatus == "Signal") ? TFT_GREEN : TFT_RED;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("GSM:", 200, 70);
  tft.setTextColor(gsmColor, TFT_BLACK);
  tft.drawString(gsmStatus, 260, 70);

  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Press hardware button for emergency", SCREEN_W / 2, 130);

  int btnW = 180, btnH = 40;
  int btnX = (SCREEN_W - btnW) / 2;
  int btnY = SCREEN_H - 60;
  changeBtn = {btnX, btnY, btnW, btnH, "Change Details"};
  tft.fillRoundRect(btnX, btnY, btnW, btnH, 8, TFT_DARKGREY);
  tft.drawRoundRect(btnX, btnY, btnW, btnH, 8, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(2);
  tft.drawString("Change Details", btnX + btnW / 2, btnY + btnH / 2);
}

void drawEmergencyScreen() {
  tft.fillScreen(TFT_BLACK);
  tft.fillRect(0, 0, SCREEN_W, 50, TFT_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setTextSize(2);
  tft.drawString("!! EMERGENCY !!", SCREEN_W / 2, 25);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Name: " + userName, SCREEN_W / 2, 80);
  tft.drawString("GPS: " + gpsStatus, SCREEN_W / 2, 105);

  if (gpsLat != 0.0 && gpsLon != 0.0) {
    String link = "maps.google.com/?q=" + String(gpsLat, 5) + "," + String(gpsLon, 5);
    tft.drawString(link, SCREEN_W / 2, 130);
  }

  tft.setTextSize(2);

  if (sysStatus == "Sending...") {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("Sending SMS...", SCREEN_W / 2, 175);

    int btnW = 180, btnH = 44;
    int btnX = (SCREEN_W - btnW) / 2;
    int btnY = 230;
    stopBtn = {btnX, btnY, btnW, btnH, "Stop"};
    tft.fillRoundRect(btnX, btnY, btnW, btnH, 8, 0x8000);
    tft.drawRoundRect(btnX, btnY, btnW, btnH, 8, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, 0x8000);
    tft.drawString("Stop Sending", btnX + btnW / 2, btnY + btnH / 2);

  } else if (sysStatus == "Sent!") {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("SMS Sent!", SCREEN_W / 2, 175);

    int btnW = 140, btnH = 44;
    int btnX = (SCREEN_W - btnW) / 2;
    int btnY = 230;
    backBtn = {btnX, btnY, btnW, btnH, "Back"};
    tft.fillRoundRect(btnX, btnY, btnW, btnH, 8, TFT_DARKGREY);
    tft.drawRoundRect(btnX, btnY, btnW, btnH, 8, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawString("Back", btnX + btnW / 2, btnY + btnH / 2);

  } else if (sysStatus == "Failed") {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawString("SMS Failed", SCREEN_W / 2, 175);

    int btnW   = 140, btnH = 44;
    int gap    = 20;
    int totalW = btnW * 2 + gap;
    int startX = (SCREEN_W - totalW) / 2;
    int btnY   = 230;

    retryBtn = {startX,              btnY, btnW, btnH, "Retry"};
    backBtn  = {startX + btnW + gap, btnY, btnW, btnH, "Back"};

    tft.fillRoundRect(retryBtn.x, btnY, btnW, btnH, 8, TFT_RED);
    tft.drawRoundRect(retryBtn.x, btnY, btnW, btnH, 8, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawString("Retry", retryBtn.x + btnW / 2, btnY + btnH / 2);

    tft.fillRoundRect(backBtn.x, btnY, btnW, btnH, 8, TFT_DARKGREY);
    tft.drawRoundRect(backBtn.x, btnY, btnW, btnH, 8, TFT_WHITE);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
    tft.drawString("Back", backBtn.x + btnW / 2, btnY + btnH / 2);
  }
}

// ============================================================
//  A9G — GPS
// ============================================================
void requestGPS() {
  Serial2.println("AT+LOCATION=2");

  unsigned long t = millis();
  while (millis() - t < 2000) {
    if (stopRequested) { gpsStatus = "No Fix"; return; }
    delay(10);
  }

  String response = "";
  t = millis();
  while (millis() - t < 3000) {
    if (stopRequested) { gpsStatus = "No Fix"; return; }
    if (Serial2.available()) response += (char)Serial2.read();
    delay(10);
  }

  int idx = response.indexOf("+LOCATION:");
  if (idx != -1) {
    String coords = response.substring(idx + 10);
    coords.trim();
    int comma = coords.indexOf(',');
    if (comma != -1) {
      gpsLat    = coords.substring(0, comma).toFloat();
      gpsLon    = coords.substring(comma + 1).toFloat();
      gpsStatus = "Fix";
      return;
    }
  }
  gpsStatus = "No Fix";
}

// ============================================================
//  A9G — GSM
// ============================================================
void checkGSM() {
  Serial2.println("AT+CSQ");

  unsigned long t = millis();
  while (millis() - t < 1000) {
    if (stopRequested) return;
    delay(10);
  }

  String response = "";
  t = millis();
  while (millis() - t < 2000) {
    if (stopRequested) return;
    if (Serial2.available()) response += (char)Serial2.read();
    delay(10);
  }

  if (response.indexOf("+CSQ: 99") != -1 || response.indexOf("+CSQ:99") != -1) {
    gsmStatus = "No Signal";
  } else if (response.indexOf("+CSQ") != -1) {
    gsmStatus = "Signal";
  } else {
    gsmStatus = "No Signal";
  }
}

// ============================================================
//  A9G — SMS
// ============================================================
void sendSMS() {
  // ============================================================
  //  EMERGENCY CONTACT — change this to your recipient number
  // ============================================================
  String contactNumber = "+639XXXXXXXXX";  // <-- put number here
  // ============================================================

  if (stopRequested) { sysStatus = "Failed"; return; }

  String mapsLink = "https://maps.google.com/?q="
                    + String(gpsLat, 5) + "," + String(gpsLon, 5);

  String message  = "EMERGENCY!\n";
         message += "Name: " + userName + "\n";
         message += "Location: " + mapsLink;

  Serial2.println("AT+CMGF=1");

  unsigned long t = millis();
  while (millis() - t < 500) {
    if (stopRequested) { sysStatus = "Failed"; return; }
    delay(10);
  }

  Serial2.println("AT+CMGS=\"" + contactNumber + "\"");

  t = millis();
  while (millis() - t < 500) {
    if (stopRequested) { sysStatus = "Failed"; return; }
    delay(10);
  }

  Serial2.print(message);
  Serial2.write(26);

  String response = "";
  t = millis();
  while (millis() - t < 5000) {
    if (stopRequested) { sysStatus = "Failed"; return; }
    if (Serial2.available()) response += (char)Serial2.read();
    delay(10);
  }

  if (response.indexOf("OK") != -1 || response.indexOf("+CMGS") != -1) {
    sysStatus = "Sent!";
  } else {
    sysStatus = "Failed";
  }
}

// ============================================================
//  BUZZER
// ============================================================
void updateBuzzer() {
  if (!buzzerActive) {
    digitalWrite(PIN_BUZZER, LOW);
    return;
  }
  if (millis() - lastBuzzerToggle > 200) {
    lastBuzzerToggle = millis();
    buzzerState      = !buzzerState;
    digitalWrite(PIN_BUZZER, buzzerState ? HIGH : LOW);
  }
}

// ============================================================
//  EMERGENCY BUTTON INTERRUPT
// ============================================================
void IRAM_ATTR onEmergencyButton() {
  if (millis() - lastButtonPress > 500) {
    lastButtonPress    = millis();
    emergencyTriggered = true;
  }
}

// ============================================================
//  PREFERENCES
// ============================================================
void saveData() {
  prefs.begin("userdata", false);
  prefs.putString("name", userName);
  prefs.putBool("registered", true);
  prefs.end();
}

void loadData() {
  prefs.begin("userdata", true);
  firstBoot = !prefs.getBool("registered", false);
  userName  = prefs.getString("name", "");
  prefs.end();
}

// ============================================================
//  EMERGENCY FLOW
// ============================================================
void runEmergency() {
  stopRequested = false;
  buzzerActive  = true;
  sysStatus     = "Sending...";
  sysState      = STATE_EMERGENCY;
  needsRedraw   = true;
  drawEmergencyScreen();

  requestGPS();
  if (!stopRequested) checkGSM();
  if (!stopRequested) sendSMS();

  // If stop was pressed at any point go straight back to main
  if (stopRequested) {
    buzzerActive = false;
    digitalWrite(PIN_BUZZER, LOW);
    sysStatus    = "Ready";
    sysState     = STATE_MAIN;
    needsRedraw  = true;
    return;
  }

  buzzerActive = false;
  digitalWrite(PIN_BUZZER, LOW);
  needsRedraw  = true;
}

// ============================================================
//  TOUCH HANDLER
// ============================================================
void handleTouch(int x, int y) {
  if (millis() - lastTouch < 180) return;
  lastTouch = millis();

  // START SCREEN
  if (sysState == STATE_START) {
    if (hit(signUpBtn, x, y)) {
      nameInput   = "";
      sysState    = STATE_REGISTER;
      needsRedraw = true;
    }
    return;
  }

  // REGISTER or CHANGE SCREEN
  if (sysState == STATE_REGISTER || sysState == STATE_CHANGE) {
    if (hit(enterBtn, x, y)) {
      if (nameInput.length() > 0) {
        userName    = nameInput;
        saveData();
        nameInput   = "";
        sysState    = STATE_MAIN;
        needsRedraw = true;
      }
      return;
    }
    if (hit(delBtn, x, y)) {
      if (nameInput.length() > 0) {
        nameInput.remove(nameInput.length() - 1);
        needsRedraw = true;
      }
      return;
    }
    if (hit(spaceBtn, x, y)) {
      if (nameInput.length() < 20) {
        nameInput  += " ";
        needsRedraw = true;
      }
      return;
    }
    for (int i = 0; i < keyCount; i++) {
      if (hit(keys[i], x, y)) {
        if (nameInput.length() < 20) {
          nameInput  += keys[i].label;
          needsRedraw = true;
        }
        return;
      }
    }
    return;
  }

  // MAIN SCREEN
  if (sysState == STATE_MAIN) {
    if (hit(changeBtn, x, y)) {
      nameInput   = userName;
      sysState    = STATE_CHANGE;
      needsRedraw = true;
    }
    return;
  }

  // EMERGENCY SCREEN
  if (sysState == STATE_EMERGENCY) {
    if (sysStatus == "Failed") {
      if (hit(retryBtn, x, y)) {
        runEmergency();
      }
      if (hit(backBtn, x, y)) {
        buzzerActive = false;
        digitalWrite(PIN_BUZZER, LOW);
        sysStatus    = "Ready";
        sysState     = STATE_MAIN;
        needsRedraw  = true;
      }
    } else if (sysStatus == "Sent!") {
      if (hit(backBtn, x, y)) {
        buzzerActive = false;
        digitalWrite(PIN_BUZZER, LOW);
        sysStatus    = "Ready";
        sysState     = STATE_MAIN;
        needsRedraw  = true;
      }
    }
    return;
  }
}

// ============================================================
//  RENDER
// ============================================================
void render() {
  if (!needsRedraw) return;
  needsRedraw = false;

  switch (sysState) {
    case STATE_START:     drawStartScreen();     break;
    case STATE_REGISTER:  drawInputScreen();     break;
    case STATE_CHANGE:    drawInputScreen();     break;
    case STATE_MAIN:      drawMainScreen();      break;
    case STATE_EMERGENCY: drawEmergencyScreen(); break;
  }
}

// ============================================================
//  CURSOR BLINK
// ============================================================
void updateCursor() {
  if (sysState != STATE_REGISTER && sysState != STATE_CHANGE) return;
  if (needsRedraw) return;
  if (millis() - lastBlink > 500) {
    lastBlink     = millis();
    cursorVisible = !cursorVisible;
    drawCursorOnly(cursorVisible);
  }
}

// ============================================================
//  SETUP
// ============================================================
void setup() {
  Serial.begin(115200);

  Serial2.begin(A9G_BAUD, SERIAL_8N1, A9G_RX, A9G_TX);

  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);

  pinMode(PIN_BUTTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), onEmergencyButton, FALLING);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);

  loadData();
  buildKeyboard();

  if (!firstBoot) {
    sysState = STATE_MAIN;
  }

  needsRedraw = true;
}

// ============================================================
//  LOOP
// ============================================================
void loop() {
  if (emergencyTriggered) {
    emergencyTriggered = false;
    if (sysState != STATE_EMERGENCY) {
      runEmergency();
    }
  }

  uint16_t rawX, rawY;
  if (tft.getTouch(&rawX, &rawY)) {
    int x = fixX(rawX);
    int y = fixY(rawY);

    // During emergency sending, check stop button directly
    // so it can interrupt the blocking flow
    if (sysState == STATE_EMERGENCY && sysStatus == "Sending...") {
      if (hit(stopBtn, x, y)) {
        stopRequested = true;
      }
    } else {
      handleTouch(x, y);
    }
  }

  updateBuzzer();
  render();
  updateCursor();
}