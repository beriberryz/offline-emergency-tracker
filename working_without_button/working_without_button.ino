#include <SoftwareSerial.h>

SoftwareSerial A9G(7, 8);

// ================= CLEAN READ LINE =================
String readLine(unsigned long timeout) {
  String line = "";
  unsigned long t = millis();

  while (millis() - t < timeout) {
    while (A9G.available()) {
      char c = A9G.read();

      if (c == '\r') continue;

      if (c == '\n') {
        if (line.length() > 0) {
          return line;
        }
      } else {
        if (c == '/') c = '.';
        line += c;
      }
    }
  }

  return line;
}

// ================= FLUSH =================
void flushA9G() {
  while (A9G.available()) A9G.read();
  delay(100);
}

// ================= SEND COMMAND =================
void sendCommand(String cmd) {
  flushA9G();
  A9G.println(cmd);
}

// ================= SINGLE GPS READ =================
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

// ================= GPS (3X CHECK VERSION) =================
String getLocation() {
  Serial.println("Requesting GPS (3x check)...");

  String results[3];

  for (int i = 0; i < 3; i++) {
    Serial.println("GPS attempt " + String(i + 1));

    results[i] = getSingleLocation();

    Serial.println("Result " + String(i + 1) + ": " + results[i]);

    delay(1000);
  }

  // pick best valid result
  String best = "";

  for (int i = 0; i < 3; i++) {
    if (results[i].length() > 10 && results[i].indexOf(',') != -1) {
      best = results[i];
      break;
    }
  }

  if (best == "") {
    Serial.println("No valid GPS fix");
    return "";
  }

  Serial.println("FINAL GPS: " + best);
  return best;
}

// ================= SMS =================
void sendSMS(String number, String message) {
  Serial.println("Sending SMS...");

  flushA9G();

  A9G.print("AT+CMGS=\"");
  A9G.print(number);
  A9G.print("\"\r");

  unsigned long t = millis();
  bool gotPrompt = false;

  while (millis() - t < 5000) {
    if (A9G.available()) {
      if (A9G.read() == '>') {
        gotPrompt = true;
        break;
      }
    }
  }

  if (!gotPrompt) {
    Serial.println("FAILED: No > prompt");
    return;
  }

  Serial.println("Prompt OK");
  delay(200);

  A9G.print(message);
  delay(200);
  A9G.write(26);

  Serial.println("SMS SENT!");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  A9G.begin(115200);

  delay(3000);

  sendCommand("AT");
  sendCommand("AT+CMGF=1");
  sendCommand("AT+CSCS=\"GSM\"");
  sendCommand("AT+GPS=1");

  delay(3000);

  String location = getLocation();

  String message;
  if (location.length() > 0) {
    message = "TULONG! " + location;
  } else {
    message = "TULONG! Location unavailable.";
  }

  Serial.println("Final message: " + message);

  sendSMS("09560253860", message);
}

void loop() {}