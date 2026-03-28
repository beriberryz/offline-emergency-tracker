# Offline Emergency Tracker
## Project Overview
The Emergency GPS Tracker System is a standalone safety device built using ESP32 + A9G GSM/GPS module + touchscreen display. It allows a user to send their live location via SMS during an emergency by pressing a hardware button. This project works without internet, relying only on GPS and mobile signal.

## How It Works
### Normal Operation
1. Device powers on using external switch
2. System boots into main UI
3. Displays status: “Ready”
4. Waits for emergency trigger

### First-Time Setup
When the device is powered on for the first time:
- User is prompted to enter:
  Name: [[USER NAME INPUT FIELD]]
  Age: [[USER AGE INPUT FIELD]]
  Data is saved permanently in ESP32 memory
  Setup screen will not appear again unless reset

### Emergency Mode
When the hardware button is pressed:
1. Buzzer activates 🔊
2. A9G module requests GPS coordinates 📡
3. Coordinates are converted to Google Maps link 🗺️
4. SMS is sent to emergency contact 📩
5. UI updates with status message

## Hardware Requirements
