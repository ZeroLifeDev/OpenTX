#ifndef HARDWARE_CONFIG_RX_H
#define HARDWARE_CONFIG_RX_H

// --- PINS ---

// I2C (MPU6050)
#define PIN_I2C_SDA 21
#define PIN_I2C_SCL 22

// Motor (L298N Channel A)
#define PIN_MOTOR_ENA 32 // PWM Speed
#define PIN_MOTOR_IN1 33 // Direction 1
#define PIN_MOTOR_IN2 25 // Direction 2

// Servos
#define PIN_SERVO_STEER 26
#define PIN_SERVO_SUSP_FL 27 
#define PIN_SERVO_SUSP_FR 14
#define PIN_SERVO_SUSP_RL 12
#define PIN_SERVO_SUSP_RR 13

// Sensors
#define PIN_BATTERY_ADC 35

// --- SETTINGS ---
#define PWM_FREQ 20000
#define PWM_RES 8
#define PWM_CH_MOTOR 0

#endif
