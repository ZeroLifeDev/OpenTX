
# OpenTX & OpenRX Firmware

Built for ESP32 with ILI9163 / ST7735 Displays.

## 🚀 Features

*   **ESP-NOW Connectivity**: Ultra-low latency control with bi-directional telemetry.
*   **Adaptive UI**: High-framerate dashboard with "expensive" aesthetics.
*   **Bi-Directional**: TX knows when RX is connected. (LED Status: OFF = Connected).
*   **Safe**: Brownout protection and auto-failsafe.

## 🛠️ Setup Instructions

### 1. Transmitter (TX)
*   Flash the code in the `TX` folder.
*   **Required Libraries**: 
    *   `TFT_eSPI` (configured for your screen).
    *   **Edit `User_Setup.h`** in your library folder to select `ILI9163_DRIVER` and set pins: `13, 14, 5, 25, 4`.
*   **Controls**:
    *   **Menu**: Cycles screens.
    *   **Set**: Reset Trim / Select.
    *   **Trim Buttons**: Adjust steering center.
    *   **Gyro Switch**: Toggles Gyro (with cool animation).

### 2. Receiver (RX)
*   Flash code in `RX` folder.
*   **No Pairing Needed**: Uses Broadcast pairing initially.
*   **LED Status**:
    *   **Blinking**: Searching for signal.
    *   **OFF**: Connected and receiving data.

## 🔧 Hardware Config
*   **Display**: 1.8" or 1.44" TFT (ST7735 / ILI9163).
*   **Inputs**: Calibrated for standard joysticks (Calibration offsets in `HardwareConfig.h`).
