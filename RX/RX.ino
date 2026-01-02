#include <WiFi.h>
#include <esp_now.h>
// NO LEDC INCLUDES, NO EXTERNAL SERVO LIBRARIES

// --- Data Structure for ESP-NOW Communication ---
// This struct MUST be IDENTICAL to the one on your transmitter (TX)
// for proper data interpretation.
typedef struct struct_message {
    // Data sent from TX to RX (Control Data)
    int joystickX;      // Scaled joystick X value (0-1000, 500 is neutral). Represents throttle or steering.
    int joystickY;      // Scaled joystick Y value (0-1000, 500 is neutral). Represents throttle or steering.
    int potTrim;        // Scaled potentiometer 1 (Trim) value (0-1000). Could be used for fine-tuning.
    int airSuspension;  // Scaled potentiometer 2 (Air Suspension) value (0-1000).
    int buttonModeState; // State of the main mode button (HIGH/LOW). Used for screen mode changes.
    unsigned long timestamp; // Timestamp from TX for RX failsafe detection. Critical for signal loss detection.
    int rssi;           // RSSI value from TX (for RX to display/use). Indicates signal strength.
    bool gyroActive;    // State of the simulated Gyro feature (true/false).
    bool exhaustModeActive; // State of the simulated Exhaust mode (true/false).
    bool spoilerEngaged; // State of the simulated Spoiler action (momentary, true/false).
    int ledMode;        // Current mode of simulated LEDs (0: OFF, 1: LOW, 2: HIGH).
    int driveMode;      // Current mode (0-3).

    // Telemetry data received from RX (e.g., from MPU-6050, battery sensor, speed sensor)
    float rxBatteryVoltage; // Receiver battery voltage.
    float rxSpeedKmh;       // Actual speed from RX.
    float rxPacketLoss;     // Actual packet loss reported by RX (if RX tracks this).
    float gyroRoll;         // MPU-6050 Roll angle from RX.
    float gyroPitch;        // MPU-6050 Pitch angle from RX.
    float gyroYaw;          // MPU-6050 Yaw angle from RX.
} struct_message;

// Create a struct_message to hold received data
struct_message incomingData;

// --- Pin Definitions for Receiver Components ---
const int BLUE_LED_PIN = 2; // Built-in LED on many ESP32 Dev Kits

// Steering Servo
const int STEERING_SERVO_PIN = 18; // Example GPIO for steering servo
const int STEERING_MIN_ANGLE = 0;   // Servo angle min (degrees)
const int STEERING_MAX_ANGLE = 180; // Servo angle max (degrees)
const int STEERING_CENTER_ANGLE = 90; // Servo angle center (degrees)

// Air Suspension Servo
const int AIR_SUSPENSION_SERVO_PIN = 19; // Example GPIO for air suspension servo
const int SUSPENSION_MIN_ANGLE = 0;   // Suspension servo angle min (degrees)
const int SUSPENSION_MAX_ANGLE = 180; // Suspension servo angle max (degrees)

// L298N Motor Driver Pins
const int MOTOR_IN1_PIN = 26; // L298N IN1
const int MOTOR_IN2_PIN = 27; // L298N IN2
const int MOTOR_ENA_PIN = 14; // L298N ENA (PWM enabled pin)

// LED Pins (for low and high beam simulation)
const int LED_LOW_PIN = 25;  // Example GPIO for low beam LED
const int LED_HIGH_PIN = 33; // Example GPIO for high beam LED

// Exhaust Button Simulation Pin (for humidifier)
const int EXHAUST_BUTTON_PIN = 13; // Example GPIO for exhaust button simulation

// --- Failsafe Variables ---
unsigned long lastDataReceivedTime = 0; // Timestamp of the last successful data reception.
const unsigned long FAILSAFE_TIMEOUT_MS = 750; // If no data for this long, assume connection lost.
bool isConnected = false; // Flag to track connection status.

// --- Global State Variables for Control Logic ---
bool prevExhaustModeActive = false; // To detect changes in exhaust mode for button clicks
int currentSteeringAngle = 90; // To store the current steering angle (initialized to center)
int currentMotorSpeed = 0; // To store the current motor speed (0-255)
bool motorDirectionForward = true; // To store current motor direction

// --- Gyro Assist Parameters ---
const float GYRO_ASSIST_GAIN = 0.5; // Adjust this value to control the strength of gyro assist (e.g., 0.1 to 1.0)
const float GYRO_DEADZONE = 5.0; // Degrees of yaw change to ignore for assist

// --- Function Prototypes ---
void OnDataRecv(const esp_now_recv_info * mac_info, const uint8_t *incomingDataBuffer, int len);
void controlSteering(int joystickXValue, float gyroYawValue, bool gyroActiveState, int driveMode);
void controlMotor(int joystickYValue, int driveMode);
void controlLEDs(int ledMode);
void handleExhaust(bool exhaustActive);
void controlAirSuspension(int airSuspensionValue);
void applyFailsafe();
void simulateButtonClick(int pin, int duration_ms);
void runStartupSequence(); // New prototype for the startup sequence
void writeServoSoftwarePWM(int pin, int angle); // Software PWM function for servos

// --- Callback Function for Received ESP-NOW Data ---
// This function is executed automatically when an ESP-NOW packet is received.
void OnDataRecv(const esp_now_recv_info * mac_info, const uint8_t *incomingDataBuffer, int len) {
    // Check if the received data length matches our expected struct size.
    if (len == sizeof(struct_message)) {
        // Copy the incoming data into our 'incomingData' struct.
        memcpy(&incomingData, incomingDataBuffer, sizeof(struct_message));
        
        // Update the last data received time and set connection status to true.
        lastDataReceivedTime = millis();
        isConnected = true;

        // Turn on the blue LED to indicate active connection.
        digitalWrite(BLUE_LED_PIN, HIGH); 

        // --- Process incoming data and control actuators ---
        controlSteering(incomingData.joystickX, incomingData.gyroYaw, incomingData.gyroActive, incomingData.driveMode);
        controlMotor(incomingData.joystickY, incomingData.driveMode);
        controlLEDs(incomingData.ledMode);
        handleExhaust(incomingData.exhaustModeActive);
        controlAirSuspension(incomingData.airSuspension);

        // Optional: Print received data for debugging
        Serial.print("Received from TX: JX="); Serial.print(incomingData.joystickX);
        Serial.print(", JY="); Serial.print(incomingData.joystickY);
        Serial.print(", Trim="); Serial.print(incomingData.potTrim);
        Serial.print(", AirSusp="); Serial.print(incomingData.airSuspension);
        Serial.print(", ModeBtn="); Serial.print(incomingData.buttonModeState);
        Serial.print(", RSSI(TX)="); Serial.print(incomingData.rssi);
        Serial.print(", Gyro="); Serial.print(incomingData.gyroActive ? "ON" : "OFF");
        Serial.print(", Exhaust="); Serial.print(incomingData.exhaustModeActive ? "ON" : "OFF");
        Serial.print(", Spoiler="); Serial.print(incomingData.spoilerEngaged ? "ENGAGED" : "IDLE");
        Serial.print(", LED="); Serial.print(incomingData.ledMode);
        Serial.print(", Drive="); Serial.print(incomingData.driveMode);
        Serial.print(", RX Batt="); Serial.print(incomingData.rxBatteryVoltage, 2);
        Serial.print(", RX Speed="); Serial.print(incomingData.rxSpeedKmh, 1);
        Serial.print(", Gyro Roll="); Serial.print(incomingData.gyroRoll, 1);
        Serial.print(", Gyro Pitch="); Serial.print(incomingData.gyroPitch, 1);
        Serial.print(", Gyro Yaw="); Serial.println(incomingData.gyroYaw, 1);

    } else {
        Serial.print("Received unexpected data length: "); Serial.println(len);
    }
}

// --- Setup Function ---
void setup() {
    Serial.begin(115200); // Initialize serial communication for debugging.
    Serial.println("ESP32 Receiver Starting Up...");

    // Configure all pins first
    pinMode(BLUE_LED_PIN, OUTPUT);
    pinMode(STEERING_SERVO_PIN, OUTPUT); 
    pinMode(AIR_SUSPENSION_SERVO_PIN, OUTPUT);
    pinMode(MOTOR_IN1_PIN, OUTPUT);
    pinMode(MOTOR_IN2_PIN, OUTPUT);
    pinMode(MOTOR_ENA_PIN, OUTPUT);
    pinMode(LED_LOW_PIN, OUTPUT);
    pinMode(LED_HIGH_PIN, OUTPUT);
    pinMode(EXHAUST_BUTTON_PIN, OUTPUT);

    // Run the cool startup sequence
    runStartupSequence();

    // After startup sequence, ensure everything is in a safe initial state
    digitalWrite(BLUE_LED_PIN, LOW); // Start with LED off.
    Serial.print("Blue LED configured on GPIO "); Serial.println(BLUE_LED_PIN);

    // Initialize Servos to center/min on startup using software PWM
    writeServoSoftwarePWM(STEERING_SERVO_PIN, STEERING_CENTER_ANGLE);
    Serial.print("Steering Servo initialized on GPIO "); Serial.print(STEERING_SERVO_PIN); Serial.print(", centered at "); Serial.print(STEERING_CENTER_ANGLE); Serial.println(" degrees.");

    writeServoSoftwarePWM(AIR_SUSPENSION_SERVO_PIN, SUSPENSION_MIN_ANGLE);
    Serial.print("Air Suspension Servo initialized on GPIO "); Serial.print(AIR_SUSPENSION_SERVO_PIN); Serial.print(", set to min at "); Serial.print(SUSPENSION_MIN_ANGLE); Serial.println(" degrees.");

    // Configure Motor Driver Pins
    analogWrite(MOTOR_ENA_PIN, 0); // Set PWM to 0 (off)
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    Serial.println("Motor Driver (L298N) pins configured and PWM set up.");

    // Configure LED Pins
    digitalWrite(LED_LOW_PIN, LOW);
    digitalWrite(LED_HIGH_PIN, LOW);
    Serial.print("LED Low Beam configured on GPIO "); Serial.println(LED_LOW_PIN);
    Serial.print("LED High Beam configured on GPIO "); Serial.println(LED_HIGH_PIN);

    // Configure Exhaust Button Simulation Pin
    digitalWrite(EXHAUST_BUTTON_PIN, LOW); // Ensure it's off initially
    Serial.print("Exhaust Button Simulation configured on GPIO "); Serial.println(EXHAUST_BUTTON_PIN);


    // Set WiFi mode to Station. ESP-NOW works best in Station mode.
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(); // Disconnect from any previous WiFi connections.
    Serial.print("Receiver MAC Address: ");
    Serial.println(WiFi.macAddress());

    // Initialize ESP-NOW.
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW!");
        // If ESP-NOW fails to initialize, you might want to halt or indicate an error.
        // For example, blink the LED rapidly to show a critical error.
        while(true) {
            digitalWrite(BLUE_LED_PIN, HIGH);
            delay(100);
            digitalWrite(BLUE_LED_PIN, LOW); 
            delay(100);
        }
    }
    Serial.println("ESP-NOW initialized successfully.");

    // Register the callback function to handle incoming ESP-NOW data.
    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("ESP-NOW Receive Callback registered.");

    Serial.println("Receiver Setup Complete. Waiting for data...");
}

/**
 * @brief Runs a cool startup sequence for the receiver.
 * This sequence turns on LEDs, moves servos, and briefly activates the motor
 * to visually confirm system readiness and component functionality.
 */
void runStartupSequence() {
    Serial.println("Running startup sequence...");

    // 1. Blue LED blink
    digitalWrite(BLUE_LED_PIN, HIGH);
    delay(200);
    digitalWrite(BLUE_LED_PIN, LOW);
    delay(100);

    // 2. LED sequence (Low, High, Off)
    Serial.println("  - Cycling LEDs...");
    digitalWrite(LED_LOW_PIN, HIGH);
    delay(300);
    digitalWrite(LED_HIGH_PIN, HIGH); // Both low and high
    delay(300);
    digitalWrite(LED_LOW_PIN, LOW);
    digitalWrite(LED_HIGH_PIN, LOW);
    delay(200);

    // 3. Steering Servo sweep (using software PWM)
    Serial.println("  - Sweeping Steering Servo...");
    writeServoSoftwarePWM(STEERING_SERVO_PIN, 0); // Go to 0 degrees
    delay(500);
    writeServoSoftwarePWM(STEERING_SERVO_PIN, 180); // Go to 180 degrees
    delay(500);
    writeServoSoftwarePWM(STEERING_SERVO_PIN, 90); // Go to 90 degrees (center)
    delay(300);

    // 4. Air Suspension Servo sweep (using software PWM)
    Serial.println("  - Sweeping Air Suspension Servo...");
    writeServoSoftwarePWM(AIR_SUSPENSION_SERVO_PIN, 180); // Go to max height
    delay(500);
    writeServoSoftwarePWM(AIR_SUSPENSION_SERVO_PIN, 0); // Go to min height
    delay(300);

    // 5. Motor brief spin (forward then reverse)
    Serial.println("  - Briefly spinning motor...");
    digitalWrite(MOTOR_IN1_PIN, HIGH);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    analogWrite(MOTOR_ENA_PIN, 100); // Low speed forward (0-255)
    delay(500);
    analogWrite(MOTOR_ENA_PIN, 0); // Stop
    delay(100);
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, HIGH);
    analogWrite(MOTOR_ENA_PIN, 100); // Low speed reverse (0-255)
    delay(500);
    analogWrite(MOTOR_ENA_PIN, 0); // Stop
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    delay(200);

    // 6. Exhaust button clicks
    Serial.println("  - Toggling Exhaust button...");
    simulateButtonClick(EXHAUST_BUTTON_PIN, 50);
    delay(100);
    simulateButtonClick(EXHAUST_BUTTON_PIN, 50);
    delay(200);

    // Ensure all actuators are off and in a safe state after the sequence
    applyFailsafe();
    Serial.println("Startup sequence complete.");
    delay(500); // Small pause after sequence
}


// --- Main Loop Function ---
void loop() {
    // Check for failsafe: If no data has been received for FAILSAFE_TIMEOUT_MS,
    // assume connection is lost and turn off the blue LED and stop all actuators.
    if (millis() - lastDataReceivedTime > FAILSAFE_TIMEOUT_MS) {
        if (isConnected) { // Only print message and change state if it was previously connected.
            Serial.println("Failsafe triggered: No data received. Connection lost!");
            applyFailsafe(); // Call failsafe function to stop everything.
            isConnected = false;
        }
    }
    // Other non-time-critical receiver-side logic can go here.
    // For software PWM servos, you might need to continuously refresh the signal
    // if the servo doesn't hold its position. This loop is the place for that.
    // However, for basic control, sending on data reception is usually sufficient.
    yield(); // Yield to FreeRTOS to prevent watchdog timer resets.
}

/**
 * @brief Generates a software PWM pulse for a servo motor.
 * This method is CPU-intensive and can be affected by other code execution.
 * @param pin The GPIO pin connected to the servo's signal wire.
 * @param angle The desired angle for the servo (0-180 degrees).
 */
void writeServoSoftwarePWM(int pin, int angle) {
    // Map angle (0-180) to pulse width (500us to 2500us)
    int pulseWidthUs = map(angle, 0, 180, 500, 2500);

    // Generate the pulse
    digitalWrite(pin, HIGH);
    delayMicroseconds(pulseWidthUs);
    digitalWrite(pin, LOW);
    // Delay for the remainder of the 20ms (20000us) cycle
    // This delay is CRITICAL for the servo to interpret the pulse correctly
    // and for the overall PWM period to be maintained.
    delayMicroseconds(20000 - pulseWidthUs); 
}


/**
 * @brief Controls the steering servo based on joystick X input,
 * applying gyro assist and drive mode specific adjustments.
 * @param joystickXValue Scaled joystick X value (0-1000, 500 is neutral).
 * @param gyroYawValue Current yaw angle from gyro (degrees).
 * @param gyroActiveState Boolean indicating if gyro assist is active.
 * @param driveMode Current drive mode (0: Normal, 1: Sport, 2: Eco, 3: Custom/Drift).
 */
void controlSteering(int joystickXValue, float gyroYawValue, bool gyroActiveState, int driveMode) {
    int targetAngle = STEERING_CENTER_ANGLE; // Default to center
    int effectiveJoystickX = joystickXValue;

    // Apply drive mode specific steering sensitivity
    if (driveMode == 1) { // Sport Mode: Sharper steering
        effectiveJoystickX = map(joystickXValue, 0, 1000, -600, 1600); // Exaggerate input
        effectiveJoystickX = constrain(effectiveJoystickX, 0, 1000); // Constrain back
    } else if (driveMode == 2) { // Eco Mode: Softer steering
        effectiveJoystickX = map(joystickXValue, 0, 1000, 100, 900); // Reduce sensitivity
        effectiveJoystickX = constrain(effectiveJoystickX, 0, 1000);
    } else if (driveMode == 3) { // Custom/Drift Mode: More sensitive steering
        effectiveJoystickX = map(joystickXValue, 0, 1000, -800, 1800); // Even more exaggerated
        effectiveJoystickX = constrain(effectiveJoystickX, 0, 1000);
    }

    // Apply Gyro Assist if active and in drift mode
    if (gyroActiveState && driveMode == 3) {
        float steeringCorrection = 0;
        if (abs(gyroYawValue) > GYRO_DEADZONE) {
            steeringCorrection = -gyroYawValue * GYRO_ASSIST_GAIN;
        }
        effectiveJoystickX = constrain(effectiveJoystickX + (int)steeringCorrection, 0, 1000);
    }

    // Map the effective joystick X value (0-1000) to the servo angle range (0-180).
    targetAngle = map(effectiveJoystickX, 0, 1000, STEERING_MIN_ANGLE, STEERING_MAX_ANGLE);
    
    // Use software PWM function
    writeServoSoftwarePWM(STEERING_SERVO_PIN, targetAngle);
    currentSteeringAngle = targetAngle; // Update global state
}

/**
 * @brief Controls the motor speed and direction using L298N driver based on joystick Y input.
 * Implements different motor behaviors for various drive modes.
 * @param joystickYValue Scaled joystick Y value (0-1000, 500 is neutral).
 * @param driveMode Current drive mode (0: Normal, 1: Sport, 2: Eco, 3: Custom/Drift).
 */
void controlMotor(int joystickYValue, int driveMode) {
    int motorSpeedPWM = 0;
    bool forward = true;

    // Apply drive mode specific motor response
    if (driveMode == 0) { // Normal Mode
        if (joystickYValue > 500) { // Forward
            forward = true;
            motorSpeedPWM = map(joystickYValue, 500, 1000, 0, 255);
        } else if (joystickYValue < 500) { // Reverse
            forward = false;
            motorSpeedPWM = map(joystickYValue, 500, 0, 0, 255);
        } else { // Stop
            motorSpeedPWM = 0;
        }
    } else if (driveMode == 1) { // Sport Mode: More aggressive acceleration
        if (joystickYValue > 500) { // Forward
            forward = true;
            motorSpeedPWM = map(joystickYValue, 500, 1000, 50, 255); // Higher minimum speed
        } else if (joystickYValue < 500) { // Reverse
            forward = false;
            motorSpeedPWM = map(joystickYValue, 500, 0, 50, 255); // Higher minimum speed
        } else { // Stop
            motorSpeedPWM = 0;
        }
    } else if (driveMode == 2) { // Eco Mode: Less aggressive, power saving
        if (joystickYValue > 500) { // Forward
            forward = true;
            motorSpeedPWM = map(joystickYValue, 500, 1000, 0, 180); // Max speed limited
        } else if (joystickYValue < 500) { // Reverse
            forward = false;
            motorSpeedPWM = map(joystickYValue, 500, 0, 0, 180); // Max speed limited
        } else { // Stop
            motorSpeedPWM = 0;
        }
    } else if (driveMode == 3) { // Custom/Drift Mode: Adjusted for drift behavior
        // In drift mode, we want quick throttle response and potentially
        // a slight boost on sharp steering to initiate/maintain drift.
        // For simplicity, we'll make the throttle very responsive.
        if (joystickYValue > 500) { // Forward
            forward = true;
            motorSpeedPWM = map(joystickYValue, 500, 1000, 0, 255);
            // Optional: Add a temporary power boost if steering sharply
            // This would require more complex state tracking (e.g., if steering angle changes rapidly)
        } else if (joystickYValue < 500) { // Reverse
            forward = false;
            motorSpeedPWM = map(joystickYValue, 500, 0, 0, 255);
        } else { // Stop
            motorSpeedPWM = 0;
        }
    }

    // Set motor direction
    if (forward) {
        digitalWrite(MOTOR_IN1_PIN, HIGH);
        digitalWrite(MOTOR_IN2_PIN, LOW);
    } else {
        digitalWrite(MOTOR_IN1_PIN, LOW);
        digitalWrite(MOTOR_IN2_PIN, HIGH);
    }
    motorDirectionForward = forward; // Update global state

    // Apply motor speed (PWM)
    analogWrite(MOTOR_ENA_PIN, motorSpeedPWM); // Using analogWrite for motor PWM
    currentMotorSpeed = motorSpeedPWM; // Update global state
}

/**
 * @brief Controls the state of the low and high beam LEDs.
 * @param ledMode Integer representing the desired LED mode (0: OFF, 1: LOW, 2: HIGH).
 */
void controlLEDs(int ledMode) {
    switch (ledMode) {
        case 0: // OFF
            digitalWrite(LED_LOW_PIN, LOW);
            digitalWrite(LED_HIGH_PIN, LOW);
            break;
        case 1: // LOW Beam (GPIO X lights up)
            digitalWrite(LED_LOW_PIN, HIGH);
            digitalWrite(LED_HIGH_PIN, LOW);
            break;
        case 2: // HIGH Beam (GPIO X and Y light up)
            digitalWrite(LED_LOW_PIN, HIGH);
            digitalWrite(LED_HIGH_PIN, HIGH);
            break;
        default: // Fallback to OFF for unknown modes
            digitalWrite(LED_LOW_PIN, LOW);
            digitalWrite(LED_HIGH_PIN, LOW);
            break;
    }
}

/**
 * @brief Handles the exhaust (humidifier) control based on `exhaustModeActive`.
 * Simulates button clicks on a GPIO pin.
 * "Off does 2 clicks" logic is implemented here.
 * @param exhaustActive Boolean state from the transmitter.
 */
void handleExhaust(bool exhaustActive) {
    // If exhaust mode changes from active to inactive, simulate two clicks.
    if (prevExhaustModeActive && !exhaustActive) {
        Serial.println("Exhaust: Simulating 2 clicks for OFF.");
        simulateButtonClick(EXHAUST_BUTTON_PIN, 50); // First click
        delay(100); // Small delay between clicks
        simulateButtonClick(EXHAUST_BUTTON_PIN, 50); // Second click
    } 
    // If exhaust mode changes from inactive to active, simulate one click.
    else if (!prevExhaustModeActive && exhaustActive) {
        Serial.println("Exhaust: Simulating 1 click for ON.");
        simulateButtonClick(EXHAUST_BUTTON_PIN, 50); // Single click for ON
    }
    // Update previous state for next iteration
    prevExhaustModeActive = exhaustActive;
}

/**
 * @brief Simulates a momentary button click on a given GPIO pin.
 * Sets the pin HIGH, waits, then sets it LOW.
 * @param pin The GPIO pin to simulate the click on.
 * @param duration_ms The duration in milliseconds the button is "pressed" (HIGH).
 */
void simulateButtonClick(int pin, int duration_ms) {
    digitalWrite(pin, HIGH); // "Press" the button
    delay(duration_ms);      // Hold for duration
    digitalWrite(pin, LOW);  // "Release" the button
}

/**
 * @brief Controls the air suspension servo based on the incoming air suspension value.
 * @param airSuspensionValue Scaled air suspension value (0-1000).
 */
void controlAirSuspension(int airSuspensionValue) {
    // Map the incoming 0-1000 value to the servo's angle range.
    int targetAngle = map(airSuspensionValue, 0, 1000, SUSPENSION_MIN_ANGLE, SUSPENSION_MAX_ANGLE);
    // Use software PWM function
    writeServoSoftwarePWM(AIR_SUSPENSION_SERVO_PIN, targetAngle);
}

/**
 * @brief Applies failsafe measures when connection is lost.
 * Stops motors, sets servos to neutral, and turns off all LEDs.
 */
void applyFailsafe() {
    Serial.println("Applying Failsafe: Stopping all actuators.");
    // Turn off blue connection LED
    digitalWrite(BLUE_LED_PIN, LOW);

    // Stop motor
    digitalWrite(MOTOR_IN1_PIN, LOW);
    digitalWrite(MOTOR_IN2_PIN, LOW);
    analogWrite(MOTOR_ENA_PIN, 0); // Using analogWrite for motor PWM
    currentMotorSpeed = 0; // Update state

    // Set steering servo to neutral
    writeServoSoftwarePWM(STEERING_SERVO_PIN, STEERING_CENTER_ANGLE);
    currentSteeringAngle = STEERING_CENTER_ANGLE; // Update state

    // Set air suspension servo to a safe default (e.g., minimum height)
    writeServoSoftwarePWM(AIR_SUSPENSION_SERVO_PIN, SUSPENSION_MIN_ANGLE);

    // Turn off all LEDs
    digitalWrite(LED_LOW_PIN, LOW);
    digitalWrite(LED_HIGH_PIN, LOW);

    // Ensure exhaust button is not active
    digitalWrite(EXHAUST_BUTTON_PIN, LOW);
    prevExhaustModeActive = false; // Reset state
}
