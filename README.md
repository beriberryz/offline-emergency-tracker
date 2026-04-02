<h1>Emergency GPS Tracker System Using Arduino Nano + A9G GSM/GPS Module</h1>

<p><b>Emergency GPS Tracker System</b> is a standalone offline safety device built using an <b>Arduino Nano</b> and the <b>A9G GSM + GPS module</b>. It is designed to send the user’s real-time location via SMS during emergencies using a hardware push button.</p>

<p>This system works completely <b>without internet</b>, relying only on GPS satellites for location and GSM network for SMS communication.</p>

<p><b>⚠️ Note:</b> This document focuses on how to operate and understand the hardware behavior of the device.</p>

---

<h2>1. How to Use the Device</h2>

<h3>1.1 Normal Operation</h3>

<ul>
  <li>Power on the device using battery or external supply</li>
  <li>Wait a few seconds for GSM and GPS initialization</li>
  <li>The system automatically starts background GPS tracking</li>
</ul>

<h3>1.2 Emergency Trigger</h3>

<p>Press the hardware button to activate emergency mode:</p>

<ol>
  <li>Buzzer immediately turns ON</li>
  <li>Arduino checks latest stored GPS data</li>
  <li>If GPS is available → coordinates are used</li>
  <li>If GPS is not available → fallback message is used</li>
  <li>SMS is sent to the predefined contact number</li>
  <li>Buzzer provides confirmation beeps after sending</li>
</ol>

---

<h2>2. System Behavior (Important Technical Details)</h2>

<h3>2.1 GPS Update System</h3>

<ul>
  <li>The system updates GPS data every <b>15 seconds</b></li>
  <li>GPS runs in the background without blocking button input</li>
  <li>The latest valid location is stored in memory</li>
  <li>If GPS fails, the last known location is retained</li>
</ul>

<h3>2.2 GPS Accuracy Notes</h3>

<ul>
  <li>GPS readings may sometimes contain small digit errors due to signal noise</li>
  <li>Example: a digit may temporarily change (e.g. 14.6482 → 24.6482)</li>
  <li>This happens due to weak satellite lock or signal reflection</li>
  <li>The system mitigates this by averaging multiple readings</li>
</ul>

---

<h2>3. LED Indicator System</h2>

<p>The system uses LED blinking patterns to indicate device status:</p>

<table>
  <tr>
    <th>LED Behavior</th>
    <th>Meaning</th>
  </tr>
  <tr>
    <td>Fast blinking</td>
    <td>Searching for GPS/GSM signal</td>
  </tr>
  <tr>
    <td>Slow blinking</td>
    <td>GPS/GSM locked and stable</td>
  </tr>
  <tr>
    <td>OFF</td>
    <td>System idle or no power</td>
  </tr>
</table>

---

<h2>4. Hardware Requirements</h2>

<table>
  <tr>
    <th>Component</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>Arduino Nano</td>
    <td>Main microcontroller handling logic and control</td>
  </tr>
  <tr>
    <td>A9G Module</td>
    <td>Handles GPS positioning and GSM SMS sending</td>
  </tr>
  <tr>
    <td>Push Button</td>
    <td>Triggers emergency SMS system</td>
  </tr>
  <tr>
    <td>Buzzer</td>
    <td>Audio alert for emergency activation and confirmation</td>
  </tr>
  <tr>
</table>

---

<h2>5. Pinout Configuration</h2>

<p><b>Insert pinout diagram below:</b></p>

<pre>
[ PLACE YOUR PINOUT IMAGE HERE ]
</pre>

<ul>
  <li>A9G TX → Arduino Pin 7</li>
  <li>A9G RX → Arduino Pin 8</li>
  <li>Push Button → Arduino Pin 3</li>
  <li>Buzzer → Arduino Pin 5</li>
  <li>GND → Common Ground</li>
  <li>VCC → 5V / regulated supply (depending on module requirements)</li>
</ul>

---

<h2>6. How the System Works Internally</h2>

<h3>6.1 Background GPS System</h3>

<ul>
  <li>GPS is continuously updated every <b>15 seconds</b></li>
  <li>Each update attempts multiple readings for stability</li>
  <li>Only valid coordinates are stored</li>
</ul>

<h3>6.2 Emergency Flow</h3>

<ol>
  <li>User presses button</li>
  <li>Buzzer activates immediately</li>
  <li>System checks stored GPS data</li>
  <li>Message is built (Google Maps link if available)</li>
  <li>SMS is sent via GSM network</li>
  <li>Buzzer confirms completion</li>
</ol>

---

<h2>7. Limitations</h2>

<ul>
  <li>GPS may fail indoors or in weak signal areas</li>
  <li>SMS delivery depends on mobile network availability</li>
  <li>Coordinates may slightly vary due to GPS noise</li>
</ul>

---

<h2>8. Software Requirements</h2>

<ul>
  <li>Arduino IDE</li>
  <li>SoftwareSerial library (built-in)</li>
</ul>
