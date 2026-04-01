#include <SoftwareSerial.h>

SoftwareSerial A9G(7, 8);

#define BUTTON_PIN 3
#define BUZZER_PIN 5  

String latestLocation = "";
unsigned long lastGpsUpdate = 0;
const unsigned long gpsInterval = 5000;


String readLine(unsigned long timeout) {
  String line = "";
  unsigned long t = millis();

  while (millis() - t < timeout) {
    while (A9G.available()) {
      char c = A9G.read();

      if (c == '\r') continue;

      if (c == '\n') {
        if (line.length() > 0) return line;
      } else {
        if (c == '/') c = '.';
        line += c;
      }
    }
  }

  return line;
}


void flushA9G() {
  while (A9G.available()) A9G.read();
  delay(100);
}


void sendCommand(String cmd) {
  flushA9G();
  A9G.println(cmd);
}


String getSingleLocation() {
  flushA9G();
  A9G.println("AT+LOCATION=2");

  unsigned long t = millis();
  String line;

  while (millis() - t < 5000) {
    line = readLine(1000);
    line.trim();

    if (line.length() == 0) continue;
    if (line.startsWith("AT")) continue;
    if (line.startsWith("+LOCATION")) continue;
    if (line == "OK") continue;

    if (line.indexOf(',') != -1) {

      String clean = "";
      for (int i = 0; i < line.length(); i++) {
        char c = line[i];

        if ((c >= '0' && c <= '9') || c == '.' || c == ',' || c == '-') {
          clean += c;
        }
      }

      return clean;
    }
  }

  return "";
}


void updateGPS() {

  Serial.println("Background GPS update...");

  String best = "";

  for (int i = 0; i < 3; i++) {
    String r = getSingleLocation();
    delay(500);

    if (r.length() > 10) {
      best = r;
    }
  }

  if (best.length() > 10) {
    latestLocation = best;
    Serial.println("Stored GPS: " + latestLocation);
  }
}


void sendSMS(String number, String message) {
  Serial.println("Sending SMS...");

  flushA9G();

  A9G.print("AT+CMGS=\"");
  A9G.print(number);
  A9G.print("\"\r");

  unsigned long t = millis();
  while (millis() - t < 5000) {
    if (A9G.available()) {
      if (A9G.read() == '>') break;
    }
  }

  delay(200);

  A9G.print(message);
  delay(200);
  A9G.write(26);

  Serial.println("SMS SENT!");
}


void setup() {
  Serial.begin(115200);
  A9G.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);   // 🔊 ADDED

  digitalWrite(BUZZER_PIN, LOW);  // ensure OFF

  delay(3000);

  sendCommand("AT");
  sendCommand("AT+CMGF=1");
  sendCommand("AT+CSCS=\"GSM\"");
  sendCommand("AT+GPS=1");

  Serial.println("System ready, warming GPS...");

  updateGPS();
}

void loop() {

  if (millis() - lastGpsUpdate > gpsInterval) {
    lastGpsUpdate = millis();
    updateGPS();
  }

  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);

    if (digitalRead(BUTTON_PIN) == LOW) {

      Serial.println("BUTTON PRESSED!");

      // 🔊 START BUZZER IMMEDIATELY
      digitalWrite(BUZZER_PIN, HIGH);

      updateGPS();

      String message;

      if (latestLocation.length() > 0) {
        message = "TULONG! " + latestLocation;
      } else {
        message = "TULONG! Location unavailable.";
      }

      sendSMS("09560253860", message);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);
      digitalWrite(BUZZER_PIN, HIGH);
      delay(200);
      digitalWrite(BUZZER_PIN, LOW);
      delay(200);

      while (digitalRead(BUTTON_PIN) == LOW);
      delay(1000);
    }
  }
}