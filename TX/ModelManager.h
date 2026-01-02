#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// ==========================================
//          PROFESSIONAL MODEL MEMORY
// ==========================================
// Manages 10 Independent Model Slots
// Stores EPA, Expo, Trim, Reverse, ABS, etc.

#define MAX_MODELS 10
#define MODEL_NAME_LEN 12

// --- Data Structures ---

struct ChannelSettings {
    int epaMin;     // End Point Adjust Min (0-120%)
    int epaMax;     // End Point Adjust Max (0-120%)
    int subTrim;    // Mechanical offset (-50 to 50)
    int expo;       // Exponential Curve (-100 to 100)
    bool reverse;   // Servo Reverse
    int speed;      // Servo Speed (0=Fastest, 100=Slow)
};

struct ModelData {
    char name[MODEL_NAME_LEN + 1];
    int id;
    
    // Channels
    ChannelSettings steering;
    ChannelSettings throttle;
    
    // Aux Settings
    int absLevel;   // 0=Off, 1-100 Pulsing
    bool gMode;     // Gyro Mode (Normal/AVCS)
    int gyroGain;   // 0-100
};

class ModelManager {
private:
    Preferences prefs;
    ModelData currentModel;
    int currentSlot = 0;

public:
    void init() {
        prefs.begin("tx_models", false);
        currentSlot = prefs.getInt("curr_slot", 0);
        loadModel(currentSlot);
    }
    
    void loadModel(int slot) {
        if (slot < 0 || slot >= MAX_MODELS) slot = 0;
        currentSlot = slot;
        
        char key[16];
        sprintf(key, "m_%d", slot);
        
        // Read Bytes directly into struct
        if (prefs.isKey(key)) {
            prefs.getBytes(key, &currentModel, sizeof(ModelData));
        } else {
            // Initialize Default if empty
            setDefault(slot);
            saveModel();
        }
        
        // Save current slot pointer
        prefs.putInt("curr_slot", currentSlot);
    }
    
    void saveModel() {
        char key[16];
        sprintf(key, "m_%d", currentSlot);
        prefs.putBytes(key, &currentModel, sizeof(ModelData));
    }
    
    void setDefault(int slot) {
        currentModel.id = slot;
        sprintf(currentModel.name, "MODEL %02d", slot + 1);
        
        // Steering Defaults
        currentModel.steering.epaMin = 100;
        currentModel.steering.epaMax = 100;
        currentModel.steering.subTrim = 0;
        currentModel.steering.expo = 0;
        currentModel.steering.reverse = false;
        currentModel.steering.speed = 0;
        
        // Throttle Defaults
        currentModel.throttle.epaMin = 100;
        currentModel.throttle.epaMax = 100;
        currentModel.throttle.subTrim = 0;
        currentModel.throttle.expo = 0;
        currentModel.throttle.reverse = false;
        currentModel.throttle.speed = 0; // Instant
        
        // Aux
        currentModel.absLevel = 0;
        currentModel.gyroGain = 50;
        currentModel.gMode = false;
    }
    
    // --- Accessors ---
    ModelData* getModel() { return &currentModel; }
    
    int getSlot() { return currentSlot; }
    
    void setSlot(int slot) {
        saveModel(); // Save current before switch
        loadModel(slot);
    }
    
    void setName(const char* newName) {
        strncpy(currentModel.name, newName, MODEL_NAME_LEN);
        currentModel.name[MODEL_NAME_LEN] = '\0'; // Ensure null term
        saveModel();
    }
};

ModelManager modelManager;

#endif // MODEL_MANAGER_H
