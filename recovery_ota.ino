// ============================================================================
//  Marauder C5 — RECOVERY OTA (firmware minimo, gira dalla partizione FACTORY)
//  Flasha l'app principale (partizione ota_0) via WiFi quando la USB fa i
//  capricci. Usa il WebServer SINCRONO del core (niente AsyncTCP -> niente bug
//  "Required to lock TCPIP core functionality" su ESP-IDF 5.x). Headless.
//
//  Uso: connettiti al WiFi  "Marauder-OTA"  (pass: marauder1234),
//       apri  http://192.168.4.1 , carica il .bin, Flash.
//  Scrive ota_0, imposta il boot su ota_0, riavvia nell'app.
//  Bottone "Boot app" per tornare all'app senza flashare.
// ============================================================================
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <esp_ota_ops.h>

static WebServer server(80);

static const char PAGE[] PROGMEM = R"HTML(<!doctype html><html><head>
<meta name=viewport content="width=device-width,initial-scale=1"><title>Marauder OTA</title>
<style>body{font-family:sans-serif;background:#0f1115;color:#e8e8e8;text-align:center;padding:26px}
h2{color:#39c6ff;letter-spacing:2px}p{color:#aab}input,button{font-size:18px;padding:12px;margin:8px}
button{background:#c0282a;color:#fff;border:0;border-radius:8px;min-width:150px}
.b2{background:#2a7fc0}#s{margin-top:16px;font-size:16px;color:#8fd}</style></head><body>
<h2>MARAUDER&nbsp;OTA</h2><p>Recovery flasher &mdash; carica il firmware .bin dell'app.</p>
<form id=f method=POST action=/update enctype=multipart/form-data>
<input type=file name=f accept=.bin required><br>
<button type=submit>FLASH</button></form>
<form method=POST action=/boot><button class=b2 type=submit>Boot app (esci)</button></form>
<p id=s></p>
<script>f.onsubmit=()=>{s.innerText='Flashing... NON staccare, si riavvia da solo.';};</script>
</body></html>)HTML";

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Marauder-OTA", "marauder1234");   // AP fisso e documentato
  delay(300);

  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", PAGE); });

  // ritorna all'app principale senza flashare
  server.on("/boot", HTTP_POST, []() {
    const esp_partition_t* app0 = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (app0) esp_ota_set_boot_partition(app0);
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", "Boot app... riavvio");
    delay(600); ESP.restart();
  });

  // upload firmware -> scrive nella prossima partizione OTA (ota_0) + boot + reboot
  server.on("/update", HTTP_POST,
    []() {
      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", Update.hasError() ? "FLASH FALLITO" : "OK - reboot nell'app");
      delay(800);
      ESP.restart();
    },
    []() {
      HTTPUpload& up = server.upload();
      if (up.status == UPLOAD_FILE_START) {
        Serial.printf("[OTA] start %s\n", up.filename.c_str());
        Update.begin(UPDATE_SIZE_UNKNOWN);          // target = ota_0
      } else if (up.status == UPLOAD_FILE_WRITE) {
        Update.write(up.buf, up.currentSize);
      } else if (up.status == UPLOAD_FILE_END) {
        if (Update.end(true)) Serial.printf("[OTA] done %u bytes\n", up.totalSize);
        else Update.printError(Serial);
      }
    });

  server.begin();
  Serial.println("RECOVERY OTA pronto: AP Marauder-OTA / http://192.168.4.1");
}

void loop() { server.handleClient(); }
