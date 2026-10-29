#include <WiFi.h>
#include <WebServer.h>

const char* SSID = "RC-CAR";
const char* PASS = "12345678";

// Penggerak
const int IN1 = 4;
const int IN2 = 5;
// Steering
const int BIN1 = 6;
const int BIN2 = 7;

const unsigned long TIMEOUT_MS = 400;   // failsafe

bool berjalan = false;
bool membelok = false;
unsigned long lastDrive = 0;
unsigned long lastSteer = 0;

WebServer server(80);

const char HALAMAN[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no">
<title>RC Car</title>
<style>
  body { font-family: sans-serif; text-align: center; background: #222; color: #fff;
         -webkit-user-select: none; user-select: none; touch-action: none; }
  button { height: 90px; margin: 10px 0; font-size: 26px;
           border: none; border-radius: 16px; background: #3a7; color: #fff; }
  button:active { background: #285; }
  
  .baris { margin-top: 120px ; display: grid ; justify-content: center; gap: 12px; }
  .baris button { width: 200px ; background: #37a; }
  .baris button:active { background: #258; }
  .control { display:flex; justify-content: center; gap: 12px; }
  .control button { width: 38%; height: 200px;}
</style>
</head>
<body>

  <div class="control">
    <button id="mundur"> < </button>
    <button id="maju"> > </button>
  </div>
  <div class="baris">
    <button id="kiri">KIRI</button>
    <button id="kanan">KANAN</button>
  </div>

<script>
  function tahan(id, path, stopPath) {
    const el = document.getElementById(id);
    let timer = null;
    function mulai(e) {
      e.preventDefault();
      if (timer) return;
      fetch(path);
      timer = setInterval(() => fetch(path), 150);
    }
    function selesai(e) {
      e.preventDefault();
      clearInterval(timer);
      timer = null;
      fetch(stopPath);
    }
    el.addEventListener('pointerdown', mulai);
    el.addEventListener('pointerup', selesai);
    el.addEventListener('pointercancel', selesai);
    el.addEventListener('pointerleave', selesai);
  }
  tahan('maju',   '/maju',   '/stop');
  tahan('mundur', '/mundur', '/stop');
  tahan('kiri',   '/kiri',   '/lurus');
  tahan('kanan',  '/kanan',  '/lurus');
</script>
</body>
</html>
)rawliteral";

// ---------- PENGGERAK (tanpa PWM) ----------
void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  berjalan = false;
}

void maju() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  berjalan = true;
}

void mundur() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  berjalan = true;
}

// ---------- STEERING ----------
void kiri() {
  digitalWrite(BIN1, HIGH);
  digitalWrite(BIN2, LOW);
  membelok = true;
}

void kanan() {
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, HIGH);
  membelok = true;
}

void lurus() {
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
  membelok = false;
}

void setup() {
  Serial.begin(115200);
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  stopMotor();
  lurus();

  WiFi.softAP(SSID, PASS);
  Serial.print("Alamat: http://");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() { server.send_P(200, "text/html", HALAMAN); });

  server.on("/maju",   []() { lastDrive = millis(); maju();      server.send(200, "text/plain", "ok"); });
  server.on("/mundur", []() { lastDrive = millis(); mundur();    server.send(200, "text/plain", "ok"); });
  server.on("/stop",   []() { lastDrive = millis(); stopMotor(); server.send(200, "text/plain", "ok"); });

  server.on("/kiri",   []() { lastSteer = millis(); kiri();   server.send(200, "text/plain", "ok"); });
  server.on("/kanan",  []() { lastSteer = millis(); kanan();  server.send(200, "text/plain", "ok"); });
  server.on("/lurus",  []() { lastSteer = millis(); lurus();  server.send(200, "text/plain", "ok"); });

  server.begin();
}

void loop() {
  server.handleClient();

  // failsafe penggerak
  if (berjalan && millis() - lastDrive > TIMEOUT_MS) {
    stopMotor();
  }
  // failsafe steering
  if (membelok && millis() - lastSteer > TIMEOUT_MS) {
    lurus();
  }
}