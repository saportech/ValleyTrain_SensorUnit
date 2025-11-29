// ==== Sender side (per station board) ====
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// MAC of the receiver (brain / UI ESP32)
uint8_t receiverMac[] = {0x34, 0x5F, 0x45, 0x33, 0x5C, 0xD0};

const int STATION_PIN = 4;           // the sensor pin on this board
const unsigned long debounceDelay = 30; // ms

bool stationStableLow = false;       // equivalent to stationStableStates[i]
unsigned long stationDebounceTime = 0;

void printMacAddress();
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

void setup() {
    Serial.begin(115200);
    delay(4000);

    printMacAddress();

    pinMode(STATION_PIN, INPUT_PULLUP); // or INPUT depending on wiring

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }

    esp_now_register_send_cb(onDataSent);

    esp_now_peer_info_t peer = {};
    memcpy(peer.peer_addr, receiverMac, 6);
    peer.channel = 0;
    peer.encrypt = false;

    if (esp_now_add_peer(&peer) != ESP_OK) {
        Serial.println("Failed to add ESP-NOW peer");
    }
}

void loop() {
    int stationState = digitalRead(STATION_PIN); // HIGH or LOW
    unsigned long now = millis();

    if (stationState == LOW) {
        // same logic as in your UI::sampleStations()
        if ((now - stationDebounceTime) > debounceDelay) {
            if (!stationStableLow) { // new stable LOW
                stationStableLow = true;

                // send "1" once when the stable LOW is detected
                uint8_t value = 1;
                esp_err_t result = esp_now_send(receiverMac, &value, 1);
                if (result != ESP_OK) {
                    Serial.print("Error sending ESP-NOW: ");
                    Serial.println(result);
                } else {
                    Serial.println("Sent station LOW event");
                }
            }
        }
    } else {
        // if station is HIGH – reset debounce timer and stable state
        stationStableLow = false;
        stationDebounceTime = now;
    }

    // small loop delay – not part of debounce, just to avoid hammering
    delay(5);
}

void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Send callback: ");
    for (int i = 0; i < 6; i++) {
        if (mac_addr[i] < 16) Serial.print("0");
        Serial.print(mac_addr[i], HEX);
        if (i < 5) Serial.print(":");
    }

    Serial.print(" -> status = ");
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("SUCCESS");
    } else {
        Serial.println("FAIL");
    }
}

void printMacAddress() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    Serial.print("Receiver MAC Address: ");
    for (int i = 0; i < 6; i++) {
        if (mac[i] < 16) Serial.print("0"); // leading zero
        Serial.print(mac[i], HEX);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
}