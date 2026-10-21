// Wird in Bruce einkopiert. Startet Doom (ota_1) per Neustart.
#include <Arduino.h>
#include <esp_ota_ops.h>
#include <esp_partition.h>
#include <esp_system.h>

void launchDoom() {
    const esp_partition_t *p =
        esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_1, NULL);
    if (!p) { Serial.println("[doom] Partition ota_1 nicht gefunden"); return; }
    if (esp_ota_set_boot_partition(p) != ESP_OK) {
        Serial.println("[doom] Doom-Image fehlt/ungueltig (Gesamtpaket neu flashen)");
        return;
    }
    delay(100);
    esp_restart();
}
