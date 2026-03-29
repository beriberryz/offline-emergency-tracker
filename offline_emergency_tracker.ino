#include <TFT_eSPI.h>
#include <Preferences.h>

TFT_eSPI tft = TFT_eSPI();
Preferences prefs;

#define TFT_BL        21
#define PIN_BUTTON    35
#define PIN_BUZZER    25
#define A9G_TX        17
#define A9G_RX        16
#define A9G_BAUD      115200

#define SCREEN_W 480
#define SCREEN_H 320

#define TOUCH_X_MIN 1
#define TOUCH_X_MAX 479
#define TOUCH_Y_MIN 17
#define TOUCH_Y_MAX 302

enum SystemState {
  STATE_START,
  STATE_REGISTER,
  STATE_MAIN,
  STATE_CHANGE,
  STATE_EMERGENCY
};
SystemState sysState = STATE_START;

String userName  = "";
String nameInput = "";
bool   firstBoot = true;

String gpsStatus = "No Fix";
String gsmStatus = "No Signal";
String sysStatus = "Ready";
float  gpsLat    = 0.0;
float  gpsLon    = 0.0;

volatile bool emergencyTriggered = false;
unsigned long lastButtonPress    = 0;
bool          buzzerActive       = false;
unsigned long lastBuzzerToggle   = 0;
bool          buzzerState        = false;
bool          stopRequested      = false;

bool          needsRedraw   = true;
bool          cursorVisible = true;
unsigned long lastTouch     = 0;
unsigned long lastBlink     = 0;

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
Button sosBtn;
Button resetBtn;
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

  // ── Header ──────────────────────────────────────────────
  tft.fillRect(0, 0, SCREEN_W, 45, TFT_NAVY);
  tft.setTextDatum(ML_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.setTextSize(2);
  tft.drawString("Hi, " + userName, 15, 22);

  // Reset button — top-right inside header
  int resetW = 70, resetH = 31;
  int resetX = SCREEN_W - resetW - 8;
  int resetY = 7;
  resetBtn = {resetX, resetY, resetW, resetH, "Reset"};
  tft.fillRoundRect(resetX, resetY, resetW, resetH, 6, TFT_RED);
  tft.drawRoundRect(resetX, resetY, resetW, resetH, 6, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setTextSize(1);
  tft.drawString("Reset", resetX + resetW / 2, resetY + resetH / 2);

  // ── Status box ──────────────────────────────────────────
  // Box starts at y=52, height=80 — plenty of room for 2 rows
  int boxX = 10, boxY = 52, boxW = SCREEN_W - 20, boxH = 80;
  tft.fillRoundRect(boxX + 1, boxY + 1, boxW - 2, boxH - 2, 7, TFT_BLACK); // clear inside
  tft.drawRoundRect(boxX, boxY, boxW, boxH, 8, TFT_DARKGREY);

  // Use textSize(1) — each char is 6px wide, 8px tall
  tft.setTextSize(1);
  tft.setTextDatum(ML_DATUM);

  // Row 1  y=72  — Status value left, GSM value right
  int row1Y = 72;
  uint16_t sysColor = TFT_GREEN;
  if (sysStatus == "Sending...")  sysColor = TFT_YELLOW;
  if (sysStatus == "Failed")      sysColor = TFT_RED;

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Status:", 25, row1Y);        // label  ~50px wide
  tft.setTextColor(sysColor, TFT_BLACK);
  tft.drawString(sysStatus,  90, row1Y);       // value starts at 90

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("GSM:", 260, row1Y);          // label
  uint16_t gsmColor = (gsmStatus == "Signal") ? TFT_GREEN : TFT_RED;
  tft.setTextColor(gsmColor, TFT_BLACK);
  tft.drawString(gsmStatus, 295, row1Y);       // value

  // Row 2  y=96  — GPS
  int row2Y = 96;
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("GPS:", 25, row2Y);
  uint16_t gpsColor = (gpsStatus == "Fix") ? TFT_GREEN : TFT_RED;
  tft.setTextColor(gpsColor, TFT_BLACK);
  tft.drawString(gpsStatus, 60, row2Y);

  // ── Hint label ──────────────────────────────────────────
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextSize(1);
  tft.drawString("Press hardware button or SOS for emergency", SCREEN_W / 2, 148);

  // ── SOS button — pushed well down ───────────────────────
  int sosBtnW = 160, sosBtnH = 55;
  int sosBtnX = (SCREEN_W - sosBtnW) / 2;
  int sosBtnY = 185;
  sosBtn = {sosBtnX, sosBtnY, sosBtnW, sosBtnH, "SOS"};
  tft.fillRoundRect(sosBtnX, sosBtnY, sosBtnW, sosBtnH, 10, TFT_RED);
  tft.drawRoundRect(sosBtnX, sosBtnY, sosBtnW, sosBtnH, 10, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setTextSize(3);
  tft.drawString("SOS", sosBtnX + sosBtnW / 2, sosBtnY + sosBtnH / 2);

  // ── Change Details button ────────────────────────────────
  int btnW = 180, btnH = 38;
  int btnX = (SCREEN_W - btnW) / 2;
  int btnY = SCREEN_H - 42;
  changeBtn = {btnX, btnY, btnW, btnH, "Change Details"};
  tft.fillRoundRect(btnX, btnY, btnW, btnH, 8, TFT_DARKGREY);
  tft.drawRoundRect(btnX, btnY, btnW, btnH, 8, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
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
//  STOP BUTTON CHECK — called inside blocking loops
// ============================================================
void checkStopTouch() {
  uint16_t rawX, rawY;
  if (tft.getTouch(&rawX, &rawY)) {
    int x = fixX(rawX);
    int y = fixY(rawY);
    if (hit(stopBtn, x, y)) {
      stopRequested = true;
    }
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
    checkStopTouch();
    delay(10);
  }

  String response = "";
  t = millis();
  while (millis() - t < 3000) {
    if (stopRequested) { gpsStatus = "No Fix"; return; }
    checkStopTouch();
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
    checkStopTouch();
    delay(10);
  }

  String response = "";
  t = millis();
  while (millis() - t < 2000) {
    if (stopRequested) return;
    checkStopTouch();
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
  String contactNumber = "+639XXXXXXXXX";  // <-- put number here

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
    checkStopTouch();
    delay(10);
  }

  Serial2.println("AT+CMGS=\"" + contactNumber + "\"");

  t = millis();
  while (millis() - t < 500) {
    if (stopRequested) { sysStatus = "Failed"; return; }
    checkStopTouch();
    delay(10);
  }

  Serial2.print(message);
  Serial2.write(26);

  String response = "";
  t = millis();
  while (millis() - t < 5000) {
    if (stopRequested) { sysStatus = "Failed"; return; }
    checkStopTouch();
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

void resetData() {
  prefs.begin("userdata", false);
  prefs.clear();
  prefs.end();
  userName  = "";
  nameInput = "";
  firstBoot = true;
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

  if (sysState == STATE_START) {
    if (hit(signUpBtn, x, y)) {
      nameInput   = "";
      sysState    = STATE_REGISTER;
      needsRedraw = true;
    }
    return;
  }

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

  if (sysState == STATE_MAIN) {
    if (hit(sosBtn, x, y)) {
      runEmergency();
      return;
    }
    if (hit(changeBtn, x, y)) {
      nameInput   = userName;
      sysState    = STATE_CHANGE;
      needsRedraw = true;
      return;
    }
    if (hit(resetBtn, x, y)) {
      resetData();
      sysState    = STATE_START;
      needsRedraw = true;
      return;
    }
    return;
  }

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
    handleTouch(x, y);
  }

  updateBuzzer();
  render();
  updateCursor();
}