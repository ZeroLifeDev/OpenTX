#ifndef COMMS_MANAGER_H
#define COMMS_MANAGER_H

#include <WiFi.h>
#include <esp_now.h>
#include "HardwareConfig.h"

// Data Packet Structure (Must match RX)
typedef struct {
    int16_t steering;      // -100 to 100
    int16_t throttle;      // -100 to 100
    int16_t knob1;         // 0-100 (Suspension)
    int16_t trim;          // -20 to 20
    bool gyroEnabled;
    uint32_t timestamp;
} ControlPacket;

class CommsManager {
private:
    ControlPacket packet;
    esp_now_peer_info_t peerInfo;
    uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // blinking logic
    unsigned long lastBlink = 0;
    bool ledState = false;

public:
    void init() {
        // Init WiFi Mode
        WiFi.mode(WIFI_STA);
        
        // Init ESP-NOW
        if (esp_now_init() != ESP_OK) {
            Serial.println("Error initializing ESP-NOW");
            return;
        }

        // Register Peer (Broadcast)
        memset(&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, broadcastAddress, 6);
        peerInfo.channel = 0;  
        peerInfo.encrypt = false;
        
        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("Failed to add peer");
            return;
        }

        // Init LED
        pinMode(PIN_LED_BUILTIN, OUTPUT);
    }

    void sendData(int steer, int thr, int susp, int trim, bool gyro) {
        packet.steering = (int16_t)steer;
        packet.throttle = (int16_t)thr;
        packet.knob1 = (int16_t)map(susp, 0, 4095, 0, 100);
        packet.trim = (int16_t)trim;
        packet.gyroEnabled = gyro;
        packet.timestamp = millis();

        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &packet, sizeof(packet));
        
        if (result == ESP_OK) {
            // Success
        } else {
            // Error
        }
    }

    void update() {
        // Blink LED to indicate transmission active
        if (millis() - lastBlink > 200) { // Fast blink for TX
            lastBlink = millis();
            ledState = !ledState;
            digitalWrite(PIN_LED_BUILTIN, ledState);
        }
    }
};

// Global Instance
CommsManager commsManager;

#endif // COMMS_MANAGER_H
