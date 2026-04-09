#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

// ─── Налаштування Wi-Fi та Пінів ──────────────────────────────
const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASSWORD = "";

// Піни для LED (використовуємо вбудований діод на ESP32)
const int LED_PIN = 2;
bool ledState = false;

// ─── HTTP-сервер на порту 80 ──────────────────────────────────
WebServer server(80);

// ─── Прототипи ────────────────────────────────────────────────
void connectWiFi();
void initLittleFS();
bool handleFileRead(String path);
void handleApiStatus();
void handleApiControl();
void handleNotFound();

void setup() {
  Serial.begin(115200);
  delay(500);

  // Налаштування піна LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.println("ESP32 Async REST API Server");
  Serial.println("Лабораторна робота — CSDA");

  // Крок 1 — Ініціалізація файлової системи LittleFS
  initLittleFS();

  // Крок 2 — Підключення до Wi-Fi
  connectWiFi();

  // Крок 3 — Реєстрація REST API ендпоінтів
  server.on("/api/status", HTTP_GET, handleApiStatus);
  server.on("/api/control", HTTP_POST, handleApiControl);

  // Обробник для всіх інших запитів (Static Files)
  server.onNotFound(handleNotFound);

  // Крок 4 — Запуск сервера
  server.begin();
  Serial.println("[HTTP] Сервер запущено на порту 80");
  Serial.print("[HTTP] Відкрийте у браузері: http://");
  Serial.println(WiFi.localIP());
  Serial.println("─────────────────────────────────────────────");
}

void loop() { server.handleClient(); }
/**
 * GET /api/status
 * Повертає поточний стан LED у форматі JSON.
 */
void handleApiStatus() {
  StaticJsonDocument<200> doc;
  doc["led_on"] = ledState;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);

  Serial.println("[API] GET /api/status -> " + response);
}

/**
 * POST /api/control
 * Приймає JSON команди, наприклад: {"command": "toggle"}
 */
void handleApiControl() {
  if (server.hasArg("plain") == false) {
    server.send(400, "application/json", "{\"error\": \"Body is empty\"}");
    return;
  }

  String body = server.arg("plain");
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    Serial.print("[API] JSON Parse Error: ");
    Serial.println(error.c_str());
    server.send(400, "application/json", "{\"error\": \"Invalid JSON\"}");
    return;
  }

  String command = doc["command"];
  if (command == "toggle") {
    ledState = !ledState;
  } else if (command == "on") {
    ledState = true;
  } else if (command == "off") {
    ledState = false;
  } else {
    server.send(400, "application/json", "{\"error\": \"Unknown command\"}");
    return;
  }

  digitalWrite(LED_PIN, ledState ? HIGH : LOW);
  Serial.print("[API] POST /api/control -> Command: ");
  Serial.println(command);

  server.send(200, "application/json", "{\"result\": \"ok\"}");
}

/**
 * handleFileRead(String path)
 * Читає файл з LittleFS і відправляє його клієнту.
 * Якщо файл існує, повертає true, інакше false.
 */
bool handleFileRead(String path) {
  Serial.println("[FS] Читання: " + path);
  if (path.endsWith("/"))
    path += "index.html";

  String contentType = "text/plain";
  if (path.endsWith(".html"))
    contentType = "text/html";
  else if (path.endsWith(".css"))
    contentType = "text/css";
  else if (path.endsWith(".js"))
    contentType = "application/javascript";
  else if (path.endsWith(".json"))
    contentType = "application/json";

  if (LittleFS.exists(path)) {
    File file = LittleFS.open(path, "r");
    server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

/**
 * handleNotFound()
 * Використовується для роздачі статики (HTML, CSS), або повертає 404.
 */
void handleNotFound() {
  if (handleFileRead(server.uri())) {
    // Якщо файл роздано успішно – виходимо
    return;
  }
  server.send(404, "text/plain", "404: File Not Found");
}

void connectWiFi() {
  Serial.print("[WiFi] Підключення до ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  uint8_t attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] ✓ Підключено успішно!");
    Serial.print("[WiFi] IP-адреса: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] ✗ Помилка підключення.");
  }
}

void initLittleFS() {
  Serial.println("[FS] Ініціалізація LittleFS...");
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] ✗ Помилка монтування LittleFS!");
    return;
  }
  Serial.println("[FS] ✓ LittleFS змонтована успішно.");
}
