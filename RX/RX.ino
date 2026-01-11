#include <Arduino.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include "HardwareConfig.h"

// --- GLOBALS ---
Servo steerServo;
Servo suspFL, suspFR, suspRL, suspRR;

// MPU6050 Variables
int16_t ax, ay, az;
int16_t gx, gy, gz;

// Motor Control
void setMotor(int speed) {
    // speed: -255 to 255
    if (speed > 5) {
        digitalWrite(PIN_MOTOR_IN1, HIGH);
        digitalWrite(PIN_MOTOR_IN2, LOW);
        ledcWrite(PWM_CH_MOTOR, speed);
    } else if (speed < -5) {
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, HIGH);
        ledcWrite(PWM_CH_MOTOR, -speed);
    } else {
        digitalWrite(PIN_MOTOR_IN1, LOW);
        digitalWrite(PIN_MOTOR_IN2, LOW);
        ledcWrite(PWM_CH_MOTOR, 0);
    }
}

void setup() {
    Serial.begin(115200);
    
    // Motor Init
    pinMode(PIN_MOTOR_IN1, OUTPUT);
    pinMode(PIN_MOTOR_IN2, OUTPUT);
    ledcSetup(PWM_CH_MOTOR, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_MOTOR_ENA, PWM_CH_MOTOR);
    
    // Servo Init
    steerServo.attach(PIN_SERVO_STEER);
    suspFL.attach(PIN_SERVO_SUSP_FL);
    suspFR.attach(PIN_SERVO_SUSP_FR);
    suspRL.attach(PIN_SERVO_SUSP_RL);
    suspRR.attach(PIN_SERVO_SUSP_RR);
    
    // MPU Init
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    Wire.beginTransmission(0x68);
    Wire.write(0x6B); 
    Wire.write(0);    
    Wire.endTransmission(true);
    
    Serial.println("RX Ready");
}

void loop() {
    // 1. Read MPU
    Wire.beginTransmission(0x68);
    Wire.write(0x3B);
    Wire.endTransmission(false);
    Wire.requestFrom(0x68, 14, true);
    
    ax = Wire.read()<<8|Wire.read();
    ay = Wire.read()<<8|Wire.read();
    az = Wire.read()<<8|Wire.read();
    // Skip Temp
    Wire.read(); Wire.read();
    gx = Wire.read()<<8|Wire.read();
    gy = Wire.read()<<8|Wire.read();
    gz = Wire.read()<<8|Wire.read();
    
    // TODO: Add ESP-NOW Reception
    
    // Test Placeholder
    // Wiggle Steering
    float wiggle = sin(millis() / 500.0) * 30.0;
    steerServo.write(90 + wiggle);
    
    // Delay for stability
    delay(10);
}
