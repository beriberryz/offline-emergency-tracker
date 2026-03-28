<h1>Emergency GPS Tracker System Using ESP32 + A9G GSM/GPS Module</h1>

<p><b>Emergency GPS Tracker System</b> is a standalone embedded safety device built using <b>ESP32 touchscreen board</b> and <b>A9G GSM + GPS module</b>. The system allows a user to send their real-time location through SMS during emergencies by pressing a hardware button.</p>

<p>This project works completely <b>without internet</b>, using only GPS satellites and GSM mobile network. It is designed as a beginner-friendly embedded systems project that demonstrates real-world applications of IoT, GPS tracking, and microcontroller communication.</p>


<h2>1. Project Overview</h2>

<p>This system is designed to function as a portable emergency alert device. When triggered, it automatically obtains the user's GPS location and sends it via SMS to a predefined emergency contact.</p>

<ul>
  <li>ESP32 handles user interface and system logic</li>
  <li>A9G module handles GPS and SMS communication</li>
  <li>Touchscreen is used for setup and status display</li>
  <li>Hardware button triggers emergency mode</li>
</ul>


<h2>2. System Features</h2>

<ul>
  <li>Touchscreen UI built with LVGL</li>
  <li>QWERTY on-screen keyboard</li>
  <li>One-time user setup (name and age)</li>
  <li>Real-time GPS location tracking</li>
  <li>SMS emergency alert with Google Maps link</li>
  <li>Hardware emergency button (instant trigger)</li>
  <li>Buzzer alert system</li>
  <li>Flash memory storage (no reconfiguration needed)</li>
</ul>


<h2>3. How the System Works</h2>

<h3>3.1 Normal Startup</h3>
<p>When the device is powered on, it automatically loads the system and enters either setup mode or main screen depending on whether user data exists.</p>

<h3>3.2 First-Time Setup</h3>

<ul>
  <li>User enters name: [[NAME INPUT]]</li>
  <li>User enters age: [[AGE INPUT]]</li>
  <li>Data is saved in ESP32 flash memory</li>
</ul>

<p>After setup, this screen will no longer appear unless reset.</p>

<h3>3.3 Emergency Trigger</h3>

<p>When the hardware button is pressed:</p>

<ol>
  <li>Buzzer activates</li>
  <li>ESP32 sends command to A9G module</li>
  <li>A9G retrieves GPS coordinates</li>
  <li>Coordinates are formatted into Google Maps link</li>
  <li>SMS is sent to emergency contact</li>
  <li>Status is shown on touchscreen UI</li>
</ol>


<h2>4. Hardware Requirements</h2>

<table>
  <tr>
    <th>Component</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>ESP32 Touchscreen Board</td>
    <td>Main controller + UI system</td>
  </tr>
  <tr>
    <td>A9G Module</td>
    <td>GPS + GSM communication</td>
  </tr>
  <tr>
    <td>Push Button</td>
    <td>Emergency trigger input</td>
  </tr>
  <tr>
    <td>Buzzer</td>
    <td>Audio alert system</td>
  </tr>
  <tr>
    <td>Power Supply</td>
    <td>Battery or external charger</td>
  </tr>
</table>
