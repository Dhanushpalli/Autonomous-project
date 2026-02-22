// ESP32 Web Interface - Optimized for smaller program size
#include <WiFi.h>
#include <WebServer.h>

// ==== Wi-Fi credentials ====
const char* ssid = "Dhanush's M34 5G";     // <-- Put your SSID here
const char* password = "7569576743"; // <-- Put your password here

// ==== Motor pins ====
#define IN1 32
#define IN2 33
#define EN_A 25 // Connect to ENA on motor driver
#define IN3 18
#define IN4 19
#define EN_B 26 // Connect to ENB on motor driver

// ==== Ultrasonic Sensors ====
#define TRIG_LEFT 12
#define ECHO_LEFT 13
#define TRIG_RIGHT 4
#define ECHO_RIGHT 5

// ==== PWM Configuration ====
int currentSpeed = 150;       // Default speed (0-255)

WebServer server(80);

bool movementActive = false;
unsigned long movementEndMs = 0;
char movementCmd = 'S';

const unsigned long MOVE_FORWARD_MS = 500;
const unsigned long MOVE_TURN_MS = 400;
const unsigned long MOVE_BACK_MS = 500;

// ==== Motor functions (using analogWrite) ====
void motorStop() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW); digitalWrite(IN4, LOW);
  analogWrite(EN_A, 0);
  analogWrite(EN_B, 0);
}

void motorForward() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  // Motor A Fwd
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);  // Motor B Fwd
  analogWrite(EN_A, currentSpeed);
  analogWrite(EN_B, currentSpeed);
}

void motorBackward() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);  // Motor A Rev
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  // Motor B Rev ✅
  analogWrite(EN_A, currentSpeed);
  analogWrite(EN_B, currentSpeed);
}

void motorTurnLeft() {
  digitalWrite(IN1, LOW); digitalWrite(IN2, HIGH);  // Motor A Rev
  digitalWrite(IN3, LOW); digitalWrite(IN4, HIGH);  // Motor B Fwd ✅
  analogWrite(EN_A, currentSpeed);
  analogWrite(EN_B, currentSpeed);
}

void motorTurnRight() {
  digitalWrite(IN1, HIGH); digitalWrite(IN2, LOW);  // Motor A Fwd
  digitalWrite(IN3, HIGH); digitalWrite(IN4, LOW);  // Motor B Rev ✅
  analogWrite(EN_A, currentSpeed);
  analogWrite(EN_B, currentSpeed);
}

void startMovement(char cmd) {
  movementCmd = cmd;
  if (cmd == 'F') {
    motorForward();
    movementEndMs = millis() + MOVE_FORWARD_MS;
    movementActive = true;
  } else if (cmd == 'B') {
    motorBackward();
    movementEndMs = millis() + MOVE_BACK_MS;
    movementActive = true;
  } else if (cmd == 'L') {
    motorTurnLeft();
    movementEndMs = millis() + MOVE_TURN_MS;
    movementActive = true;
  } else if (cmd == 'R') {
    motorTurnRight();
    movementEndMs = millis() + MOVE_TURN_MS;
    movementActive = true;
  } else {
    motorStop();
    movementActive = false;
    movementCmd = 'S';
  }
}

float getDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, 20000);
  float distance = duration * 0.0343 / 2;
  if (distance == 0 || distance > 400) distance = 400;
  return distance;
}

// ==== Compact HTML ====
const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html><html><head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>Robot Control</title><style>*{margin:0;padding:0;box-sizing:border-box}body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;padding:10px}.container{max-width:600px;margin:0 auto;background:#fff;border-radius:15px;box-shadow:0 10px 30px rgba(0,0,0,.3);overflow:hidden}.header{background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);color:#fff;padding:20px;text-align:center}.header h1{font-size:24px}.content{padding:20px}.section{margin-bottom:20px;padding:15px;background:#f8f9fa;border-radius:10px}.section-title{font-size:16px;font-weight:600;color:#495057;margin-bottom:10px}.sensor-grid{display:grid;grid-template-columns:1fr 1fr;gap:10px}.sensor-card{background:#fff;padding:15px;border-radius:8px;text-align:center;box-shadow:0 2px 5px rgba(0,0,0,.1)}.sensor-label{font-size:12px;color:#6c757d;margin-bottom:5px}.sensor-value{font-size:28px;font-weight:700;color:#667eea}.info-row{display:flex;justify-content:space-between;padding:10px;background:#fff;border-radius:5px;margin-bottom:8px;font-size:14px}.status-indicator{display:inline-block;width:10px;height:10px;border-radius:50%;margin-right:5px}.status-active{background:#28a745}.status-inactive{background:#dc3545}.controls{display:grid;grid-template-columns:repeat(3,1fr);gap:8px;margin-top:10px}.btn{padding:12px;border:none;border-radius:8px;font-weight:600;cursor:pointer;transition:.2s;color:#fff}.btn-control{background:#667eea}.btn-control:active{transform:scale(.95)}#forward{grid-column:2}#left{grid-row:2}#stop{grid-column:2;grid-row:2;background:#dc3545}#right{grid-column:3;grid-row:2}#backward{grid-column:2;grid-row:3}.btn-emergency{background:#dc3545;width:100%;margin-top:10px}.speed-control{display:flex;align-items:center;gap:10px}#speed-val{font-size:18px;font-weight:600;width:40px;text-align:center}.slider{flex-grow:1;height:10px;accent-color:#667eea}</style></head><body>
<div class="container"><div class="header"><h1>🤖 Robot Control</h1></div><div class="content">
<div class="section"><div class="section-title">📡 Sensors</div><div class="sensor-grid">
<div class="sensor-card"><div class="sensor-label">LEFT</div><div class="sensor-value" id="left">--</div></div>
<div class="sensor-card"><div class="sensor-label">RIGHT</div><div class="sensor-value" id="right">--</div></div>
</div></div>
<div class="section"><div class="section-title">📍 Status</div>
<div class="info-row"><span>Status</span><span><span class="status-indicator status-inactive" id="ind"></span><span id="stat">Ready</span></span></div>
<div class="info-row"><span>Command</span><span id="cmd">STOP</span></div>
</div>
<div class="section"><div class="section-title">⚡ Speed</div><div class="speed-control"><span id="speed-val">200</span><input type="range" min="0" max="255" value="200" class="slider" id="speedSlider"></div></div>
<div class="section"><div class="section-title">🎮 Controls</div>
<div class="controls">
<button class="btn btn-control" id="forward" onclick="send('F')">&uarr;</button>
<button class="btn btn-control" id="left" onclick="send('L')">&larr;</button>
<button class="btn btn-control" id="stop" onclick="send('S')">&blacksquare;</button>
<button class="btn btn-control" id="right" onclick="send('R')">&rarr;</button>
<button class="btn btn-control" id="backward" onclick="send('B')">&darr;</button>
</div><button class="btn btn-emergency" onclick="stop()">EMERGENCY STOP</button>
</div></div></div>
<script>
setInterval(updateSensors,500);setInterval(updateStatus,1000);
async function updateSensors(){try{const r=await fetch('/ultrasonic');const d=await r.json();
document.getElementById('left').textContent=d.left.toFixed(1);
document.getElementById('right').textContent=d.right.toFixed(1);
document.getElementById('left').style.color=d.left<25?'#dc3545':d.left<50?'#ffc107':'#28a745';
document.getElementById('right').style.color=d.right<25?'#dc3545':d.right<50?'#ffc107':'#28a745';
}catch(e){}}
async function updateStatus(){try{const r=await fetch('/status');const s=await r.text();
document.getElementById('ind').className=s==='BUSY'?'status-indicator status-active':'status-indicator status-inactive';
}catch(e){}}
async function send(c){try{await fetch('/move?dir='+c);
document.getElementById('cmd').textContent=c==='F'?'FORWARD':c==='B'?'BACKWARD':c==='L'?'LEFT':c==='R'?'RIGHT':'STOP';
}catch(e){alert('Failed!')}}
async function stop(){try{await fetch('/stop');document.getElementById('cmd').textContent='STOPPED';}catch(e){}}
const speedSlider=document.getElementById('speedSlider');const speedValEl=document.getElementById('speed-val');speedSlider.oninput=()=>{speedValEl.textContent=speedSlider.value;};speedSlider.onchange=async()=>{try{await fetch('/speed?val='+speedSlider.value);}catch(e){alert('Speed set failed!');}};
updateSensors();updateStatus();
</script></body></html>
)HTML";

// ==== Handlers ====
void handleRoot() {
  server.send(200, "text/html", INDEX_HTML);
}

void handleMove() {
  String dir = server.arg("dir");
  if (dir.length() == 0) {
    server.send(400, "text/plain", "Missing dir");
    return;
  }
  char d = dir.charAt(0);
  if (d == 'F' || d == 'B' || d == 'L' || d == 'R' || d == 'S') {
    startMovement(d);
    server.send(200, "text/plain", "OK");
  } else {
    server.send(400, "text/plain", "Bad dir");
  }
}

void handleSpeed() {
  String val = server.arg("val");
  if (val.length() == 0) {
    server.send(400, "text/plain", "Missing val");
    return;
  }
  int speed = val.toInt();
  if (speed < 0) speed = 0;
  if (speed > 255) speed = 255;
  currentSpeed = speed;

  // Update speed immediately if a movement is active
  if (movementActive) {
    analogWrite(EN_A, currentSpeed);
    analogWrite(EN_B, currentSpeed);
  }

  server.send(200, "text/plain", "Speed set to " + String(currentSpeed));
}

void handleUltrasonic() {
  float left = getDistance(TRIG_LEFT, ECHO_LEFT);
  float right = getDistance(TRIG_RIGHT, ECHO_RIGHT);
  String json = "{\"left\":" + String(left, 1) + ",\"right\":" + String(right, 1) + "}";
  server.send(200, "application/json", json);
}

void handleStatus() {
  server.send(200, "text/plain", movementActive ? "BUSY" : "DONE");
}

void handleStop() {
  motorStop();
  movementActive = false;
  movementCmd = 'S';
  movementEndMs = 0;
  server.send(200, "text/plain", "STOPPED");
}

// ==== Setup ====
void setup() {
  Serial.begin(115200);
  delay(100); // Small delay for stability

  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(EN_A, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(EN_B, OUTPUT);

  pinMode(TRIG_LEFT, OUTPUT);
  pinMode(ECHO_LEFT, INPUT);
  pinMode(TRIG_RIGHT, OUTPUT);
  pinMode(ECHO_RIGHT, INPUT);

  motorStop(); // This will now set speed to 0 using analogWrite

  Serial.println("\nConnecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    Serial.println("Open browser: http://" + WiFi.localIP().toString());
  } else {
    Serial.println("\nWiFi failed!");
  }

  server.on("/", handleRoot);
  server.on("/move", handleMove);
  server.on("/speed", handleSpeed);
  server.on("/ultrasonic", handleUltrasonic);
  server.on("/status", handleStatus);
  server.on("/stop", handleStop);

  server.begin();
  Serial.println("Ready!");
}

// ==== Loop ====
void loop() {
  server.handleClient();

  if (movementActive && (long)(millis() - movementEndMs) >= 0) {
    motorStop();
    movementActive = false;
    movementCmd = 'S';
  }

  delay(1);
}