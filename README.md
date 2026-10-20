# STM32 Programmable Electrical Stimulation Prototype

[简体中文](README_zh.md) · [Validation records](docs/validation.md) · [Firmware](firmware/README.md) · [Project brief](docs/portfolio.md)

A teaching and engineering prototype from the Intelligent Medical Practice course at Nanjing University of Aeronautics and Astronautics. The system integrates an STM32F103RCT6, a Howland current source, a UART touchscreen and an ESP32-C3.

Maintainer: **Shen Gao / 高申 (Doooooooove)**. The maintainer reports independently completing hardware assembly and soldering, firmware integration, touchscreen and wireless integration, oscilloscope measurements and calibration, with AI assistance for code development.

> This repository documents a teaching prototype and resistor-load bench work. It does not establish clinical efficacy, medical-device certification or IEC 60601 compliance.

## Prototype and measurements

![Integrated bench prototype](docs/assets/prototype-bench.jpeg)

Original photograph from slide 18 of the course defense. Instrument images and readings are preserved.

| Square-wave measurement | Smooth-rise / exponential-decay measurement |
| --- | --- |
| ![Square waveform](docs/assets/scope-square.jpeg) | ![Exponential waveform](docs/assets/scope-exponential.jpeg) |

The defense photographs show waveform shapes and frequency displays around 20 Hz. Probe location, attenuation and current conversion are not fully documented with the photographs, so they are not used to establish current accuracy or charge balance.

## Implemented features

- Constant-current, smooth-rise/exponential-decay, DC-offset two-level square-wave and fixed-envelope demonstration modes.
- An eleven-point DAC calibration table covering 0–10 mA with linear interpolation.
- ADC averaging, voltage-divider reconstruction and parallel-branch compensation for impedance estimation.
- Touchscreen parameter control, status display and a waveform preview calculated from model parameters.
- Wi-Fi SoftAP / HTTP control through an ESP32-C3 UART bridge.

The touchscreen preview is model-generated. External instruments are used to inspect the physical output. Bluetooth control is not implemented in the current source.

## Current source configuration

| Function | Configuration |
| --- | --- |
| Waveform timer | TIM1, 20 kHz / 50 μs update interval |
| Analog output | DAC1, PA4 |
| Analog input | ADC1_IN0, PA0 |
| Touchscreen | USART2, PA2 TX / PA3 RX, 9600 bps, 8N1 |
| ESP32-C3 link | USART3, PB10 TX / PB11 RX, 9600 bps, 8N1 |
| Wi-Fi access point | `Medical-Stim`, defined in `wireless/main.lua` |

See [timer configuration](firmware/STM32_YLDZ_Project/Core/Src/tim.c) and [UART configuration](firmware/STM32_YLDZ_Project/Core/Src/usart.c).

The 2026-10-10 defense describes a 1 kHz architecture. The current source uses 20 kHz; the historical measurements have not been mapped to a specific firmware commit.

## Calibration records

Slide 8 records ten 1–10 mA setpoints with a nominal 2.2 kΩ resistor. Recalculation from its voltage entries gives a maximum absolute relative deviation of approximately **0.0455% for that table**. This excludes resistor tolerance and instrument uncertainty and is not a full-range accuracy specification.

Slide 16 contains a different set of current values. Both datasets are preserved separately:

- [Slide 8 transcription](data/defense-slide08-current-calibration.csv)
- [Slide 16 transcription](data/defense-slide16-current-calibration.csv)
- [Methods, provenance and limitations](docs/validation.md)

Recalculate both tables with Python 3, without installing packages:

```sh
python3 tools/calibration_summary.py
```

## Build and inspect

1. Check the [hardware wiring](hardware/README.md) against the actual bench.
2. Open `firmware/STM32_YLDZ_Project/MDK-ARM/YLDZ.uvprojx` in Keil MDK, check the configured toolchain and target, then rebuild.
3. Compile the [touchscreen project](hmi/README.md), confirming a 9600 bps UART configuration.
4. Load the [ESP32-C3 software](wireless/readme.md), join its Wi-Fi network and open the gateway address assigned by the module.

Existing HEX files are historical build outputs, not verified builds of the current source. The source currently starts in a running state and resets its countdown after zero. Review the behavior and remaining bench checks in [validation.md](docs/validation.md) before operating the prototype.

## Repository map

| Path | Contents |
| --- | --- |
| [firmware/](firmware/README.md) | STM32 source and build notes |
| [hardware/](hardware/README.md) | Current-source schematic and PCB documents |
| [hmi/](hmi/README.md) | Touchscreen project and font |
| [wireless/](wireless/readme.md) | LuatOS Web interface and command protocol |
| [docs/](docs/validation.md) | Measurement evidence and implementation notes |
| [data/](data/defense-slide08-current-calibration.csv) | Transcribed defense measurements |
| [tools/](tools/calibration_summary.py) | Standard-library calibration calculation |

## License and attribution

See the root [MIT License](LICENSE). STM32 HAL/CMSIS, LuatOS firmware and other third-party materials retain their own notices and rights.

New photographs and tabulated measurements come from the maintainer's course defense. [Asset provenance](docs/assets/README.md) lists the slide and embedded-media source for each image.
