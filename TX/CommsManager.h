#ifndef COMMS_MANAGER_H
#define COMMS_MANAGER_H

#include <WiFi.h>
#include <esp_now.h>
#include "HardwareConfig.h"
#include "SoundManager.h" // Include full header since we use it

// Data Packet Structure (Must match RX)
typedef struct {
    int16_t steering;      // -100 to 100
    int16_t throttle;      // -100 to 100
    int16_t knob1;         // 0-100 (Suspension)
    int16_t trim;          // -20 to 20
    bool gyroEnabled;
    uint32_t timestamp;
} ControlPacket;

// MUST MATCH RX EXACTLY
typedef struct {
    float rxVoltage; // Example telemetry
    bool active;
} TelemetryPacket;

class CommsManager {
private:
    ControlPacket packet;
    TelemetryPacket rxData;
    
    esp_now_peer_info_t peerInfo;
    uint8_t broadcastAddress[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    // Connection Logic
    bool connected = false;
    unsigned long lastHeartbeat = 0;
    
    // blinking logic
    unsigned long lastBlink = 0;
    bool ledState = false;
    
    // callback needs access to instance or valid static
public:
    static void OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len);

    void init() {
        // Init WiFi Mode
        WiFi.mode(WIFI_STA);
        
        // Init ESP-NOW
        if (esp_now_init() != ESP_OK) return;

        // Register Peer (Broadcast)
        memset(&peerInfo, 0, sizeof(peerInfo));
        memcpy(peerInfo.peer_addr, broadcastAddress, 6);
        peerInfo.channel = 0;  
        peerInfo.encrypt = false;
        if (esp_now_add_peer(&peerInfo) != ESP_OK) return;

        // Register Callback for Telemetry
        esp_now_register_recv_cb(CommsManager::OnDataRecv);

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

        esp_now_send(broadcastAddress, (uint8_t *) &packet, sizeof(packet));
    }

    void update() {
        // Check Timeout (1 second)
        if (millis() - lastHeartbeat > 1000) {
            if (connected) {
                 connected = false;
                 soundManager.playDisconnected(); 
            }
        }

        // LED Logic
        if (connected) {
            digitalWrite(PIN_LED_BUILTIN, LED_OFF_STATE); 
        } else {
            // Blink when searching
            if (millis() - lastBlink > 200) { 
                lastBlink = millis();
                ledState = !ledState;
                digitalWrite(PIN_LED_BUILTIN, ledState ? LED_ON_STATE : LED_OFF_STATE);
            }
        }
    }
    
    bool isConnected() { return connected; }
    
    // Internal use for static callback
    void markHeartbeat() {
        if (!connected) {
            connected = true;
            soundManager.playConnected();
        }
        lastHeartbeat = millis();
    }
};

// Global Instance
CommsManager commsManager;

// Static Callback Implementation
void CommsManager::OnDataRecv(const esp_now_recv_info_t * info, const uint8_t *incomingData, int len) {
    if (len == sizeof(TelemetryPacket)) {
        commsManager.markHeartbeat();
    }
}

#endif // COMMS_MANAGER_H
