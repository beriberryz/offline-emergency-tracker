#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define TFT_BL 21

#define SCREEN_W 480
#define SCREEN_H 320

// Calibration from your panel
#define TOUCH_X_MIN 1
#define TOUCH_X_MAX 479
#define TOUCH_Y_MIN 17
#define TOUCH_Y_MAX 302

enum ScreenState { SCREEN_INPUT, SCREEN_HOME };
ScreenState screen = SCREEN_INPUT;

String nameInput = "";
bool needsRedraw = true;
bool cursorVisible = true;
unsigned long lastTouch = 0;
unsigned long lastBlink = 0;

struct Button {
  int x, y, w, h;
  String label;
};

Button keys[40];
int keyCount = 0;
Button enterBtn;
Button delBtn;
Button spaceBtn;

int fixX(int rawX) {
  int x = map(rawX, TOUCH_X_MAX, TOUCH_X_MIN, 0, SCREEN_W);
  return constrain(x, 0, SCREEN_W);
}

int fixY(int rawY) {
  int y = map(rawY, TOUCH_Y_MIN, TOUCH_Y_MAX, 0, SCREEN_H);
  return constrain(y, 0, SCREEN_H);
}

bool hit(Button b, int x, int y) {
  return (x >= b.x && x <= b.x + b.w &&
          y >= b.y && y <= b.y + b.h);
}

void addKey(int x, int y, int w, int h, String label) {
  if (keyCount < 40) {
    keys[keyCount++] = {x, y, w, h, label};
  }
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

  int row4Y      = startY + 3 * (keyH + gap) + 4;
  int smallBtnW  = 80;
  int spaceW     = kbWidth - 2 * smallBtnW - 2 * gap;

  delBtn   = {startX,                                       row4Y, smallBtnW, 38, "DEL"};
  spaceBtn = {startX + smallBtnW + gap,                     row4Y, spaceW,    38, "SPACE"};
  enterBtn = {startX + smallBtnW + gap + spaceW + gap,      row4Y, smallBtnW, 38, "OK"};
}

void drawKey(Button b, uint16_t bgColor = TFT_DARKGREY) {
  tft.fillRoundRect(b.x, b.y, b.w, b.h, 6, bgColor);
  tft.drawRoundRect(b.x, b.y, b.w, b.h, 6, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, bgColor);
  tft.setTextSize(1);
  tft.drawString(b.label, b.x + b.w / 2, b.y + b.h / 2);
}

void drawCursorOnly(bool visible) {
  int boxX        = 30;
  int boxY        = 30;
  int boxH        = 50;
  int textPadding = 10;

  tft.setTextSize(2);

  int cursorX;
  if (nameInput.length() == 0) {
    cursorX = boxX + textPadding;
  } else {
    cursorX = boxX + textPadding + tft.textWidth(nameInput) + 2;
  }

  int cursorY = boxY + 10;
  int cursorH = boxH - 20;

  tft.fillRect(cursorX, cursorY, 2, cursorH, visible ? TFT_WHITE : TFT_BLACK);
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

void drawHome() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(3);
  tft.drawString("Hello,", SCREEN_W / 2, SCREEN_H / 2 - 30);
  tft.drawString(nameInput, SCREEN_W / 2, SCREEN_H / 2 + 30);
}

void handleTouch(int x, int y) {
  if (millis() - lastTouch < 180) return;
  lastTouch = millis();

  if (screen != SCREEN_INPUT) return;

  if (hit(enterBtn, x, y)) {
    if (nameInput.length() > 0) {
      screen = SCREEN_HOME;
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
      nameInput += " ";
      needsRedraw = true;
    }
    return;
  }

  for (int i = 0; i < keyCount; i++) {
    if (hit(keys[i], x, y)) {
      if (nameInput.length() < 20) {
        nameInput += keys[i].label;
        needsRedraw = true;
      }
      return;
    }
  }
}

void render() {
  if (!needsRedraw) return;
  needsRedraw = false;

  if (screen == SCREEN_INPUT) {
    drawInputScreen();
  } else {
    drawHome();
  }
}

void updateCursor() {
  if (screen != SCREEN_INPUT) return;
  if (needsRedraw) return;

  if (millis() - lastBlink > 500) {
    lastBlink = millis();
    cursorVisible = !cursorVisible;
    drawCursorOnly(cursorVisible);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  tft.init();
  tft.setRotation(1);
  needsRedraw = true;
}

void loop() {
  uint16_t rawX, rawY;
  if (tft.getTouch(&rawX, &rawY)) {
    int x = fixX(rawX);
    int y = fixY(rawY);
    // Serial.printf("Raw: %d,%d -> Screen: %d,%d\n", rawX, rawY, x, y);
    handleTouch(x, y);
  }

  render();
  updateCursor();
}