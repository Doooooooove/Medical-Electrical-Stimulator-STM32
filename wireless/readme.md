# Wireless IoT Web Server (ESP32-C3 LuatOS)

## 📌 Overview
An asynchronous embedded Web server and AP controller running on the **AirM2M / LuatOS ESP32-C3** module.

## 🚀 Key Features
- **Wi-Fi SoftAP**: Broadcasts `ESP32-Stimulator` network with captive web dashboard.
- **RESTful HTTP API**:
  - `/api/status`: Real-time telemetry (operating mode, frequency, pulse width, current, impedance).
  - `/api/start` / `/api/stop`: Immediate stimulation start/stop commands.
  - `/api/preset`: Prescription switching (Preset 1: Dysphagia NMES, Preset 2: Pain Relief TENS, Preset 3: Muscle Re-education).
- **Serial Protocol Dispatcher**:
  - Encapsulates single-byte `#` framed commands over UART to STM32.

## 🛠️ Flash Guide
1. Connect ESP32-C3 via USB-C.
2. Run automated flashing script:
   ```bash
   python build_and_flash.py
   ```
   Or use LuatTools to flash `LuatOS-SoC_V1004_ESP32C3.soc` and `main.lua`.
