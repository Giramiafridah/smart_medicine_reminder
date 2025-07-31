#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WebServer.h>

// WiFi credentials
const char* ssid = "EDDIE 2919";
const char* password = "123456789"; // Fixed password syntax

// Login credentials
const char* www_username = "admin";
const char* www_password = "medicine123";

// Define LED and buzzer pins
const int ledPin = 23;
const int buzzerPin = 22;

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Web server on port 80
WebServer server(80);

// Medication schedule (24-hour format)
int medTimes[][2] = {
  {8, 0},   // 8:00 AM
  {13, 30}, // 1:30 PM
  {20, 0}   // 8:00 PM
};
const int maxMedTimes = 10;
int numMedTimes = sizeof(medTimes) / sizeof(medTimes[0]);

// Variables for alert state
bool isAlertActive = false;
bool alarmsEnabled = true;
unsigned long alertStartTime = 0;
const unsigned long alertDuration = 300000; // 5 minutes in milliseconds
String lastTaken = "Never";
bool isAuthenticated = false;

// Function prototypes
void acknowledgeAlert();
String getLoginHtml();
String getDashboardHtml();
void handleRoot();
void handleLogin();
void handleLogout();
void handleAck();
void handleUpdate();
void handleToggle();
void handleGetSchedule();
bool checkAuth();

void setup() {
  Serial.begin(115200);
  
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  // Initialize NTP client
  timeClient.begin();
  timeClient.setTimeOffset(10800); // UTC+3 for Kampala, Uganda (3*3600 seconds)  // Initialize pins

  // Set up web server routes
  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/logout", handleLogout);
  server.on("/ack", handleAck);
  server.on("/update", handleUpdate);
  server.on("/toggle", handleToggle);
  server.on("/getschedule", handleGetSchedule);
  
  server.begin();
  Serial.println("HTTP server started");
}



void loop() {
  server.handleClient();
  timeClient.update();
  
  if (!alarmsEnabled) {
    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
    delay(100);
    return;
  }
  
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  String currentTime = String(currentHour) + ":" + (currentMinute < 10 ? "0" : "") + String(currentMinute);
  
  // Check if it's medication time
  bool isMedTime = false;
  for (int i = 0; i < numMedTimes; i++) {
    if (currentHour == medTimes[i][0] && currentMinute == medTimes[i][1]) {
      isMedTime = true;
      break;
    }
  }
  
  // If it's medication time and alert isn't already active
  if (isMedTime && !isAlertActive) {
    isAlertActive = true;
    alertStartTime = millis();
    lastTaken = "Pending (" + currentTime + ")";
    Serial.println("Time to take your medicine!");
  }
  
  // Handle active alert
  if (isAlertActive) {
    // Blink LED
    digitalWrite(ledPin, !digitalRead(ledPin));
    
    // Sound buzzer (beep pattern)
    if (millis() % 1000 < 500) {
      digitalWrite(buzzerPin, HIGH);
    } else {
      digitalWrite(buzzerPin, LOW);
    }
    
    // Check if alert duration has elapsed
    if (millis() - alertStartTime >= alertDuration) {
      acknowledgeAlert();
    }
  } else {
    // No active alert - ensure outputs are off
    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }
  
  delay(100);
}

void acknowledgeAlert() {
  isAlertActive = false;
  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);
  timeClient.update();
  int currentHour = timeClient.getHours();
  int currentMinute = timeClient.getMinutes();
  lastTaken = String(currentHour) + ":" + (currentMinute < 10 ? "0" : "") + String(currentMinute);
  Serial.println("Alert acknowledged at " + lastTaken);
}

bool checkAuth() {
  if (isAuthenticated) {
    return true;
  }
  
  if (!server.authenticate(www_username, www_password)) {
    server.requestAuthentication();
    return false;
  }
  isAuthenticated = true;
  return true;
}

void handleRoot() {
  if (!checkAuth()) return;
  
  server.send(200, "text/html", getDashboardHtml());
}

void handleLogin() {
  if (server.authenticate(www_username, www_password)) {
    isAuthenticated = true;
    server.sendHeader("Location", "/");
    server.send(302);
  } else {
    server.requestAuthentication();
  }
}

void handleLogout() {
  isAuthenticated = false;
  server.send(200, "text/html", getLoginHtml());
}

String getLoginHtml() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Medicine Reminder - Login</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: Arial, sans-serif;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
      margin: 0;
      background-color: #f5f5f5;
    }
    .login-container {
      background-color: white;
      padding: 30px;
      border-radius: 8px;
      box-shadow: 0 4px 8px rgba(0,0,0,0.1);
      width: 300px;
      text-align: center;
    }
    h1 {
      color: #2c3e50;
      margin-bottom: 30px;
    }
    input {
      width: 100%;
      padding: 10px;
      margin: 10px 0;
      border: 1px solid #ddd;
      border-radius: 4px;
      box-sizing: border-box;
    }
    button {
      width: 100%;
      padding: 10px;
      background-color: #3498db;
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 16px;
      margin-top: 10px;
    }
    button:hover {
      background-color: #2980b9;
    }
    .logo {
      font-size: 24px;
      font-weight: bold;
      color: #3498db;
      margin-bottom: 10px;
    }
    .team-name {
      font-size: 14px;
      color: #7f8c8d;
      margin-bottom: 20px;
      font-style: italic;
    }
  </style>
</head>
<body>
  <div class="login-container">
    <div class="logo">MEDICINE REMINDER</div>
    <div class="team-name">Developed by Team MediCare</div>
    <h1>Login</h1>
    <form action="/login" method="POST">
      <input type="text" name="username" placeholder="Username" required>
      <input type="password" name="password" placeholder="Password" required>
      <button type="submit">Login</button>
    </form>
  </div>
</body>
</html>
)rawliteral";
  return html;
}

String getDashboardHtml() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Medicine Reminder</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      margin: 0;
      padding: 0;
      background-color: #f9f9f9;
      color: #333;
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
    }
    header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 30px;
      padding-bottom: 15px;
      border-bottom: 1px solid #eee;
    }
    h1 {
      color: #2c3e50;
      margin: 0;
    }
    .logout-btn {
      background-color: #e74c3c;
      color: white;
      border: none;
      padding: 8px 15px;
      border-radius: 4px;
      cursor: pointer;
      text-decoration: none;
    }
    .card {
      background-color: white;
      border-radius: 8px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.05);
      padding: 20px;
      margin-bottom: 20px;
    }
    .alert-card {
      border-left: 5px solid )rawliteral";
  
  html += isAlertActive ? "#e74c3c" : "#2ecc71";
  html += R"rawliteral(;
    }
    .alert-status {
      font-weight: bold;
      color: )rawliteral";
  
  html += isAlertActive ? "#e74c3c" : "#2ecc71";
  html += R"rawliteral(;
    }
    .btn {
      padding: 10px 15px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      font-size: 14px;
      margin-right: 10px;
      text-decoration: none;
      display: inline-block;
    }
    .btn-primary {
      background-color: #3498db;
      color: white;
    }
    .btn-danger {
      background-color: #e74c3c;
      color: white;
    }
    .btn-success {
      background-color: #2ecc71;
      color: white;
    }
    .btn-toggle {
      background-color: )rawliteral";
  
  html += alarmsEnabled ? "#e74c3c" : "#2ecc71";
  html += R"rawliteral(;
      color: white;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      margin: 15px 0;
    }
    th, td {
      border: 1px solid #ddd;
      padding: 12px;
      text-align: left;
    }
    th {
      background-color: #f2f2f2;
    }
    tr:nth-child(even) {
      background-color: #f9f9f9;
    }
    .form-group {
      margin-bottom: 15px;
    }
    label {
      display: block;
      margin-bottom: 5px;
      font-weight: bold;
    }
    input[type="number"] {
      padding: 8px;
      width: 80px;
      border: 1px solid #ddd;
      border-radius: 4px;
    }
    .system-info {
      display: grid;
      grid-template-columns: repeat(auto-fill, minmax(200px, 1fr));
      gap: 15px;
      margin-top: 20px;
    }
    .info-item {
      background-color: white;
      padding: 15px;
      border-radius: 8px;
      box-shadow: 0 2px 5px rgba(0,0,0,0.05);
    }
    .info-label {
      font-size: 12px;
      color: #7f8c8d;
      margin-bottom: 5px;
    }
    .info-value {
      font-size: 18px;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <div class="container">
    <header>
      <h1>Medicine Reminder Dashboard</h1>
      <a href="/logout" class="logout-btn">Logout</a>
    </header>
    
    <div class="card alert-card">
      <h2>Alert Status</h2>
      <p>Alarms are currently: <span class="alert-status">)rawliteral";
  
  html += alarmsEnabled ? "ENABLED" : "DISABLED";
  html += R"rawliteral(</span></p>
      <button class="btn btn-toggle" onclick="toggleAlarms()">)rawliteral";
  
  html += alarmsEnabled ? "Disable Alarms" : "Enable Alarms";
  html += R"rawliteral(</button>
      
      <div style="margin-top: 20px;">
        <p>Current alert status: <span class="alert-status">)rawliteral";
  
  html += isAlertActive ? "ACTIVE - Medicine Time!" : "No active alerts";
  html += R"rawliteral(</span></p>)rawliteral";
  
  if (isAlertActive) {
    html += R"rawliteral(
        <button class="btn btn-primary" onclick="acknowledgeAlert()">Acknowledge Alert</button>)rawliteral";
  }
  
  html += R"rawliteral(
      </div>
    </div>
    
    <div class="card">
      <h2>Medication Schedule</h2>
      <table>
        <thead>
          <tr>
            <th>Time</th>
            <th>Action</th>
          </tr>
        </thead>
        <tbody>)rawliteral";
  
  for (int i = 0; i < numMedTimes; i++) {
    html += "<tr><td>" + String(medTimes[i][0]) + ":" + (medTimes[i][1] < 10 ? "0" : "") + String(medTimes[i][1]) + "</td>";
    html += "<td><button class=\"btn btn-danger\" onclick=\"deleteTime(" + String(i) + ")\">Delete</button></td></tr>";
  }
  
  html += R"rawliteral(
        </tbody>
      </table>
      
      <h3>Add New Reminder</h3>
      <form onsubmit="event.preventDefault(); addTime()">
        <div class="form-group">
          <label for="hour">Hour (0-23):</label>
          <input type="number" id="hour" min="0" max="23" required>
        </div>
        <div class="form-group">
          <label for="minute">Minute (0-59):</label>
          <input type="number" id="minute" min="0" max="59" required>
        </div>
        <button type="submit" class="btn btn-success">Add Reminder</button>
      </form>
    </div>
    
    <div class="card">
      <h2>System Information</h2>
      <div class="system-info">
        <div class="info-item">
          <div class="info-label">Last dose taken</div>
          <div class="info-value">)rawliteral";
  
  html += lastTaken;
  html += R"rawliteral(</div>
        </div>
        <div class="info-item">
          <div class="info-label">Current time</div>
          <div class="info-value">)rawliteral";
  
  html += timeClient.getFormattedTime();
  html += R"rawliteral(</div>
        </div>
        <div class="info-item">
          <div class="info-label">Device IP</div>
          <div class="info-value">)rawliteral";
  
  html += WiFi.localIP().toString();
  html += R"rawliteral(</div>
        </div>
        <div class="info-item">
          <div class="info-label">WiFi Status</div>
          <div class="info-value">)rawliteral";
  
  html += (WiFi.status() == WL_CONNECTED) ? "Connected" : "Disconnected";
  html += R"rawliteral(</div>
        </div>
      </div>
    </div>
  </div>
  
  <script>
    function addTime() {
      const hour = document.getElementById('hour').value;
      const minute = document.getElementById('minute').value;
      fetch('/update?add=' + hour + ',' + minute).then(() => location.reload());
    }
    
    function deleteTime(index) {
      if (confirm('Are you sure you want to delete this reminder?')) {
        fetch('/update?delete=' + index).then(() => location.reload());
      }
    }
    
    function acknowledgeAlert() {
      fetch('/ack').then(() => location.reload());
    }
    
    function toggleAlarms() {
      fetch('/toggle').then(() => location.reload());
    }
  </script>
</body>
</html>
)rawliteral";
  return html;
}

void handleAck() {
  if (!checkAuth()) return;
  acknowledgeAlert();
  server.send(200, "text/plain", "Alert acknowledged");
}

void handleToggle() {
  if (!checkAuth()) return;
  alarmsEnabled = !alarmsEnabled;
  if (!alarmsEnabled) {
    // Ensure alerts are turned off when disabling
    isAlertActive = false;
    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
  }
  server.send(200, "text/plain", alarmsEnabled ? "Alarms enabled" : "Alarms disabled");
}

void handleUpdate() {
  if (!checkAuth()) return;
  
  if (server.hasArg("add")) {
    String newTime = server.arg("add");
    int commaPos = newTime.indexOf(',');
    if (commaPos != -1 && numMedTimes < maxMedTimes) {
      int hour = newTime.substring(0, commaPos).toInt();
      int minute = newTime.substring(commaPos + 1).toInt();
      
      // Validate time
      if (hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59) {
        medTimes[numMedTimes][0] = hour;
        medTimes[numMedTimes][1] = minute;
        numMedTimes++;
        server.send(200, "text/plain", "Time added successfully");
        return;
      }
    }
    server.send(400, "text/plain", "Invalid time format");
  } 
  else if (server.hasArg("delete")) {
    int index = server.arg("delete").toInt();
    if (index >= 0 && index < numMedTimes) {
      for (int i = index; i < numMedTimes - 1; i++) {
        medTimes[i][0] = medTimes[i + 1][0];
        medTimes[i][1] = medTimes[i + 1][1];
      }
      numMedTimes--;
      server.send(200, "text/plain", "Time deleted successfully");
      return;
    }
    server.send(400, "text/plain", "Invalid index");
  } 
  else {
    server.send(400, "text/plain", "Invalid request");
  }
}

void handleGetSchedule() {
  if (!checkAuth()) return;
  
  String json = "[";
  for (int i = 0; i < numMedTimes; i++) {
    if (i != 0) json += ",";
    json += "{\"hour\":" + String(medTimes[i][0]) + ",\"minute\":" + String(medTimes[i][1]) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}j