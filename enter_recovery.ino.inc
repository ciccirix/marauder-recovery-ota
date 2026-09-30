// Drop this into your MAIN app and call enterOtaRecovery() from a menu item
// ("Web Flash (OTA)"). It shows the recovery AP/URL, invalidates otadata so the
// bootloader falls back to the `factory` partition, and reboots into the
// recovery firmware.
//
// Requires the factory + ota_0 partition layout from partitions.csv.

#include <esp_ota_ops.h>
#include <esp_partition.h>

void enterOtaRecovery() {
  // --- Show instructions on your display (adapt to your UI) --------------
  // Example with TFT_eSPI-style API used by ESP32 Marauder (display_obj.tft):
  //
  // display_obj.tft.fillScreen(TFT_BLACK);
  // display_obj.tft.setTextColor(TFT_CYAN, TFT_BLACK);
  // display_obj.tft.drawString("WEB FLASH (OTA)", 6, 8, 2);
  // display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
  // display_obj.tft.drawString("WiFi: Marauder-OTA", 6, 50);
  // display_obj.tft.drawString("pass: marauder1234", 6, 70);
  // display_obj.tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  // display_obj.tft.drawString("http://192.168.4.1", 6, 100, 2);
  // delay(4500);

  // --- Invalidate otadata -> bootloader boots the `factory` recovery ----
  const esp_partition_t* otadata = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_OTA, NULL);
  if (otadata) esp_partition_erase_range(otadata, 0, otadata->size);

  ESP.restart();
}
