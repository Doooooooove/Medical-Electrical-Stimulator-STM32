# Hardware Design & Schematics

## 📁 Files Included
- `电流板原理图_Howland恒流源.pdf`:
  - Schematic of the **54V High-Voltage Howland Voltage-Controlled Current Source (VCCS)**.
  - Core ICs: **TI LM5158** (Wide-input boost converter stepping up to 54V compliant rail) and **TI OPA551** (High-voltage 60V, high-slew-rate operational amplifier).
  - High-precision low-drift sense resistor network and 104.7kΩ impedance divider sensing circuit.
- `STM32F103RCT6核心板原理图.pdf`:
  - Schematic of the main MCU controller board (STM32F103RCT6, 64-pin LQFP, crystal oscillator, reset, SWD, USB-UART CH340C, power supply regulation).
- `电流板PCB_3D.pdf`:
  - 3D PCB layout and visualization file for the current driver board.

## ⚡ Key Circuit Specifications
| Parameter | Value | Note |
| :--- | :--- | :--- |
| Boost Converter Rail | +54V DC | Driven by LM5158 |
| Op-Amp Supply | Dual / Unipolar High Voltage | OPA551 (60V max rating) |
| Output Current Range | 0.0 ~ 10.0 mA | Step: 0.1 ~ 1.0 mA |
| Compliance Voltage | > 50V | Supports up to 5kΩ load @ 10mA |
| Impedance Sampling | 100kΩ + 4.7kΩ | Attenuation ratio 1:22.27 to PA0 ADC |
