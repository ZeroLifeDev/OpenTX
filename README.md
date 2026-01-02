# OpenTX Professional - Firmware Guide

Welcome to the new OpenTX Professional Firmware. This release focuses on a high-contrast, automotive-grade user interface designed for clarity and performance in the field.

## Key Features

### 1. Professional UX Design
*   **Theme**: "Midnight Modern" – Utilizes a pure black background (0x0000) with Slate Grey panels and Amber/Blue accents for maximum readability and reduced backlight bleed.
*   **Navigation**: Intuitive Menu-Driven interface.
    *   **Dashboard**: Primary driving view with Analog-Style Throttle Gauge and Digital Speedometer.
    *   **Menu**: Clean list access to System Info, Trim Settings, and Telemetry.

### 2. Core Functionality
*   **Stealth Mode**: The Blue LED on both the Remote and Receiver is explicitly **OFF** when a connection is established, blinking only when searching.
*   **Input Correction**: Integrated +20 offset to center inputs correctly.
*   **Telemetry**: Real-time signal and battery monitoring (where supported).

### 3. Audio Feedback
*   **System Boot**: Quick diagnostic sweep.
*   **Link Established**: Rising chime confirming connection.
*   **Critical Alarm**: Distinct siren pattern for signal loss.

## Controls

| Button | Function |
| :--- | :--- |
| **MENU** | Toggle between Dashboard and Main Menu. Back button in sub-menus. |
| **SET** | Select highlighted item / Confirm action / Reset Trim (on Dashboard). |
| **TRIM +** | Scroll Up / Increase Steering Trim. |
| **TRIM -** | Scroll Down / Decrease Steering Trim. |

## Installation
1.  **Flash TX**: Upload `TX/TX.ino` to your Transmitter ESP32.
2.  **Flash RX**: Upload `RX/RX.ino` to your Receiver ESP32.
3.  **Power Cycle**: Turn on the Receiver, then the Transmitter. Wait for the "Link" chime.

## Troubleshooting
*   **Center Drift**: If steering/throttle is not centered, verify the "NORM_VAL" in the Debug screen is near 0.
*   **Display**: Optimized for ILI9163/ST7735 displays. If colors are inverted, check `TFT_INVERSION_ON` in your library settings, though the firmware attempts to handle this.

Designed for performance.
