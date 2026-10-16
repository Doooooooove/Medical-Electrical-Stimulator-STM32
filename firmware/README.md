# STM32 Firmware (STM32F103RCT6)

## 📌 Overview
The firmware is developed for the **STM32F103RCT6** (ARM Cortex-M3 @ 72MHz) using STM32 HAL library and Keil MDK-ARM v5.

## 🚀 Key Features & Implementation
- **TIM1 1kHz Periodic ISR Engine (`main.c`)**:
  - Decoupled real-time stimulation waveform generator (hard real-time safety critical).
  - Supports **Biphasic Zero-Net-Charge Square Waves** (IEC 60601-2-10 compliant, 1~200Hz, 100~1000us pulse width).
  - Supports **Bionic Hodgkin-Huxley Exponential Waves** with $\sin^2$ smooth depolarization rise phase (4ms non-linear ramping) to prevent pain receptor firing.
- **Hardware-Decoupled Impedance Measurement (`main.c`)**:
  - Overcomes the internal $104.7\text{k}\Omega$ ($100\text{k} + 4.7\text{k}$) voltage divider load shunting error using Thevenin equivalent parallel decoupling:
    $$R_{\text{real}} = \frac{R_{\text{par}} \times 104700}{104700 - R_{\text{par}}}$$
  - Dynamic constant-current test range switching ($5\text{mA} \to 1\text{mA}$ on saturation), expanding measuring range from $5\text{k}\Omega$ to $50\text{k}\Omega$ with $<0.2\%$ full-range error.
  - 16-sample ADC oversampling with sliding window median filtering.
- **USART HMI Virtual Oscilloscope Streaming**:
  - Non-blocking 60ms batch streaming to UART1.
  - Scale overlay persistent redraw engine (`Draw_Wave_Scale_Overlay()`) preventing scale blink/erasure.
  - 1000ms finger-up touch lockout on page entry to eliminate touch bounce bugs.

## 🛠️ Build & Flash
- **IDE**: Keil MDK-ARM v5.28+
- **Project File**: `MDK-ARM/YLDZ.uvprojx`
- **Compiler**: ARM Compiler 5 (AC5) or ARM Compiler 6 (AC6)
- **Pre-compiled Firmware**: `MDK-ARM/YLDZ/YLDZ.hex`
  - Flash directly using ST-Link Utility, STM32CubeProgrammer, or Keil (`F8`).
