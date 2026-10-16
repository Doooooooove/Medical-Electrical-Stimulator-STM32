# HMI Touchscreen Project (TaoJingChi USART HMI)

## 📌 Hardware Model
- **Screen**: TaoJingChi 3.2-inch TFT Touchscreen (`TJC3224T132_011`, Resolution 320x240)
- **Communication**: UART (Baudrate: 115200, 8-N-1)

## 📁 Files
- `HMI32YLDZ.HMI`: Complete USART HMI Editor project source file.
- `gb2312-24.zi`: 24-pixel Chinese GB2312 character font library.

## 🛠️ How to Edit & Flash
1. Download and open **USART HMI Software** (陶晶驰串口屏上位机软件).
2. Open `HMI32YLDZ.HMI`.
3. Click **Compile** to generate `.tft` file.
4. **Flashing**:
   - **Method A (SD Card)**: Copy the compiled `.tft` file to an empty FAT32 MicroSD card, insert into the screen power-off, power on to flash automatically.
   - **Method B (USB-TTL)**: Connect screen TX/RX/GND/5V to USB-TTL adapter and click **Download** in USART HMI.
