#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

// ─── Налаштування Wi-Fi ───────────────────────────────────────
// У Wokwi-симуляції SSID/пароль не важливі — підключення
// відбувається автоматично до віртуальної мережі Wokwi-GUEST.
const char *WIFI_SSID = "Wokwi-GUEST";
const char *WIFI_PASSWORD = "";

// ─── HTTP-сервер на порту 80 ──────────────────────────────────
WebServer server(80);

// ─── Прототипи ────────────────────────────────────────────────
void connectWiFi();
void initLittleFS();
void handleRoot();
void handleNotFound();

// =============================================================
//  ІНІЦІАЛІЗАЦІЯ
// =============================================================
void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("\n╔══════════════════════════════════════════╗");
  Serial.println("║   ESP32 Embedded HTTP Web Server         ║");
  Serial.println("║   Лабораторна робота — CSDA              ║");
  Serial.println("╚══════════════════════════════════════════╝\n");

  // Крок 1 — Ініціалізація файлової системи LittleFS
  initLittleFS();

  // Крок 2 — Підключення до Wi-Fi
  connectWiFi();

  // Крок 3 — Реєстрація HTTP-обробників
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  // Крок 4 — Запуск сервера
  server.begin();
  Serial.println("[HTTP] Сервер запущено на порту 80");
  Serial.print("[HTTP] Відкрийте у браузері: http://");
  Serial.println(WiFi.localIP());
  Serial.println("─────────────────────────────────────────────");
}

// =============================================================
//  ГОЛОВНИЙ ЦИКЛ
// =============================================================
void loop() {
  // Обробка вхідних HTTP-запитів (non-blocking)
  server.handleClient();
}

// =============================================================
//  ФУНКЦІЇ
// =============================================================

/**
 * connectWiFi()
 * Підключає ESP32 до Wi-Fi у режимі STA (Station).
 * Виводить IP-адресу у Serial-термінал після успішного з'єднання.
 */
void connectWiFi() {
  Serial.print("[WiFi] Підключення до ");
  Serial.println(WIFI_SSID);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Чекаємо підключення (до 30 секунд)
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
    Serial.print("[WiFi] Сила сигналу (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("\n[WiFi] ✗ Помилка підключення. Перевірте SSID/пароль.");
  }
}

/**
 * initLittleFS()
 * Монтує файлову систему LittleFS з Flash-пам'яті ESP32.
 * Виводить список наявних файлів у Serial.
 */
void initLittleFS() {
  Serial.println("[FS] Ініціалізація LittleFS...");

  if (!LittleFS.begin(true)) { // true = форматувати якщо не змонтовано
    Serial.println("[FS] ✗ Помилка монтування LittleFS!");
    return;
  }

  Serial.println("[FS] ✓ LittleFS змонтована успішно.");

  // Перевірка наявності index.html
  if (LittleFS.exists("/index.html")) {
    File f = LittleFS.open("/index.html", "r");
    Serial.print("[FS] index.html знайдено, розмір: ");
    Serial.print(f.size());
    Serial.println(" байт");
    f.close();
  } else {
    Serial.println("[FS] ⚠ index.html не знайдено у Flash.");
    Serial.println("[FS]   Буде використано вбудований HTML.");
  }
}

/**
 * handleRoot()
 * HTTP GET-обробник для маршруту "/".
 * Зчитує /index.html з LittleFS і відправляє клієнту.
 * Якщо файл відсутній — відправляє вбудовану fallback-сторінку.
 */
void handleRoot() {
  Serial.print("[HTTP] GET / від клієнта ");
  Serial.println(server.client().remoteIP());

  // ── Спроба прочитати файл з LittleFS ──────────────────────
  if (LittleFS.exists("/index.html")) {
    File htmlFile = LittleFS.open("/index.html", "r");

    if (htmlFile) {
      // Читаємо вміст файлу у рядок
      String htmlContent = "";
      htmlContent.reserve(htmlFile.size());

      while (htmlFile.available()) {
        htmlContent += (char)htmlFile.read();
      }
      htmlFile.close();

      // ── Відправляємо HTTP-відповідь ───────────────────────
      server.send(200, "text/html; charset=utf-8", htmlContent);

      Serial.print("[HTTP] ✓ Відправлено index.html (");
      Serial.print(htmlContent.length());
      Serial.println(" байт) → 200 OK");
      return;
    }
  }

  // ── Fallback: вбудований HTML (якщо файл у Flash не знайдено) ──
  // Це корисно при першому запуску без завантаженої ФС
  Serial.println("[HTTP] ⚠ Файл не знайдено, відправляємо fallback HTML");

  String fallback = R"rawhtml(
<!DOCTYPE html>
<html lang="uk">
<head>
  <meta charset="UTF-8">
  <title>ESP32 Web Server</title>
  <style>
    body { font-family: sans-serif; background:#1a1a2e; color:#e0e0e0;
           display:flex; align-items:center; justify-content:center;
           min-height:100vh; margin:0; }
    .box { background:rgba(255,255,255,0.05); border:1px solid rgba(255,255,255,0.1);
           border-radius:16px; padding:40px; max-width:500px; text-align:center; }
    h1   { color:#4f8ef7; margin-bottom:16px; }
    p    { color:#a0aec0; }
    code { background:rgba(255,255,255,0.1); padding:4px 8px;
           border-radius:4px; color:#f6ad55; }
  </style>
</head>
<body>
  <div class="box">
    <h1>&#128396; ESP32 HTTP Server</h1>
    <p>Сервер працює! LittleFS: файл <code>/index.html</code> не знайдено.</p>
    <p style="margin-top:16px;font-size:0.85rem;">
      Завантажте файлову систему через <code>ESP32 Sketch Data Upload</code>.
    </p>
  </div>
</body>
</html>
)rawhtml";

  server.send(200, "text/html; charset=utf-8", fallback);
}

/**
 * handleNotFound()
 * Обробник для всіх невідомих маршрутів → 404 Not Found.
 */
void handleNotFound() {
  String uri = server.uri();
  Serial.print("[HTTP] 404 Not Found: ");
  Serial.println(uri);

  server.send(404, "text/plain", "404: Ресурс не знайдено — " + uri);
}
