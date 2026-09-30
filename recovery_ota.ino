// ============================================================================
//  Marauder C5 — RECOVERY OTA (firmware minimo, gira dalla partizione FACTORY)
//  Scopo: flashare l'app principale (partizione ota_0) via WiFi, quando la USB
//  del DevKit fa i capricci. Headless (nessun display): l'utente sa gia' l'AP e
//  l'URL perche' li mostra l'app principale prima di riavviare qui.
//
//  Uso: connettiti al WiFi  "Marauder-OTA"  (pass: marauder1234),
//       apri  http://192.168.4.1  , carica il .bin, Flash.
//  Al termine scrive ota_0, imposta il boot su ota_0 e riavvia nell'app vera.
//  Bottone "Boot app" per tornare all'app senza flashare.
// ============================================================================
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <Update.h>
#include <esp_ota_ops.h>

static AsyncWebServer server(80);

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
  delay(200);

  server.on("/", HTTP_GET, [](AsyncWebServerRequest* r) {
    r->send_P(200, "text/html", PAGE);
  });

  // ritorna all'app principale senza flashare
  server.on("/boot", HTTP_POST, [](AsyncWebServerRequest* r) {
    const esp_partition_t* app0 = esp_partition_find_first(
        ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, NULL);
    if (app0) esp_ota_set_boot_partition(app0);
    r->send(200, "text/plain", "Boot app... riavvio");
    delay(600); ESP.restart();
  });

  // upload firmware -> scrive nella prossima partizione OTA (ota_0) + set boot + reboot
  server.on("/update", HTTP_POST,
    [](AsyncWebServerRequest* r) {
      bool ok = !Update.hasError();
      AsyncWebServerResponse* res = r->beginResponse(200, "text/plain",
                                     ok ? "OK - reboot nell'app" : "FLASH FALLITO");
      res->addHeader("Connection", "close");
      r->send(res);
      delay(800);
      ESP.restart();
    },
    [](AsyncWebServerRequest* r, String fn, size_t idx, uint8_t* data, size_t len, bool final) {
      if (idx == 0) {
        Serial.printf("[OTA] start %s\n", fn.c_str());
        Update.begin(UPDATE_SIZE_UNKNOWN);   // target = ota_0 (next OTA part)
      }
      if (len) Update.write(data, len);
      if (final) {
        if (Update.end(true)) Serial.printf("[OTA] done %u bytes\n", (unsigned)(idx + len));
        else Update.printError(Serial);
      }
    });

  server.begin();
  Serial.println("RECOVERY OTA pronto: AP Marauder-OTA / http://192.168.4.1");
}

void loop() { delay(1000); }
