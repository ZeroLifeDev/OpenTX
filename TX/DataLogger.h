#ifndef DATA_LOGGER_H
#define DATA_LOGGER_H

#include <Arduino.h>

// ==========================================
//          COMMERCIAL GRADE LOGGER
// ==========================================
// Circular buffer storage with statistical analysis caps.
// Buffer Size: 1000 points (approx 10-20 sec history @ 50hz)

#define LOG_SIZE 256 // Reduced for RAM safety on ESP32 if heavy UI exists

class DataLogger {
private:
    int throttleBuf[LOG_SIZE];
    int steeringBuf[LOG_SIZE];
    int signalBuf[LOG_SIZE];
    int head; // Write pointer
    bool wrapped;
    
public:
    void init() {
        head = 0;
        wrapped = false;
        // Zero out
        for(int i=0; i<LOG_SIZE; i++) {
            throttleBuf[i] = 0;
            steeringBuf[i] = 0;
            signalBuf[i] = 0;
        }
    }
    
    void log(int thr, int str, int rssi) {
        throttleBuf[head] = thr;
        steeringBuf[head] = str;
        signalBuf[head] = rssi;
        
        head++;
        if (head >= LOG_SIZE) {
            head = 0;
            wrapped = true;
        }
    }
    
    // --- STATISTICAL ANALYSIS ---
    
    float getAvgThrottle() {
        long sum = 0;
        int count = wrapped ? LOG_SIZE : head;
        if (count == 0) return 0;
        
        for(int i=0; i<count; i++) sum += abs(throttleBuf[i]);
        return (float)sum / count;
    }
    
    int getMaxThrottle() {
        int m = 0;
        int count = wrapped ? LOG_SIZE : head;
        for(int i=0; i<count; i++) {
            if (abs(throttleBuf[i]) > m) m = abs(throttleBuf[i]);
        }
        return m;
    }
    
    float getAvgSignal() {
        long sum = 0;
        int count = wrapped ? LOG_SIZE : head;
        if (count == 0) return 0;
        for(int i=0; i<count; i++) sum += signalBuf[i];
        return (float)sum / count;
    }
    
    // Retrieve historic point (0 = newest, SIZE-1 = oldest)
    int getThrottleHistory(int stepsBack) {
        int idx = head - 1 - stepsBack;
        if (idx < 0) idx += LOG_SIZE;
        return throttleBuf[idx];
    }
    
    int getSteeringHistory(int stepsBack) {
        int idx = head - 1 - stepsBack;
        if (idx < 0) idx += LOG_SIZE;
        return steeringBuf[idx];
    }
};

// Global Instance
DataLogger dataLogger;

#endif // DATA_LOGGER_H
