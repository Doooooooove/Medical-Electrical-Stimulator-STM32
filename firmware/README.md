# STM32 固件

目标芯片为 STM32F103RCT6，工程使用 STM32 HAL。以下内容以当前 C 源码为准，历史答辩中的 1 kHz 描述不作为当前配置。

## 编译入口

- Keil 工程：`STM32_YLDZ_Project/MDK-ARM/YLDZ.uvprojx`
- 核心逻辑：`STM32_YLDZ_Project/Core/Src/main.c`
- 状态与模式：`STM32_YLDZ_Project/Core/Inc/main.h`
- 定时器：`STM32_YLDZ_Project/Core/Src/tim.c`
- 串口：`STM32_YLDZ_Project/Core/Src/usart.c`

在 Keil MDK 中打开工程，按项目设置安装匹配的编译器和 STM32F1 器件支持，核对目标芯片后重新构建。已有 HEX 是历史产物，不能代替当前源码的构建验证。本次文档整理未进行 Keil 编译、烧录或实机复测。

## 输出与通信

| 模块 | 源码配置 |
| --- | --- |
| TIM1 | Prescaler = 71，Period = 49；72 MHz 定时器时钟下为 20 kHz、50 μs |
| DAC | DAC1 / PA4，按标定表与插值结果写入片内 DAC 寄存器 |
| ADC | ADC1_IN0 / PA0，采集后按有效样本数取均值 |
| 触屏 | USART2，PA2 TX / PA3 RX，9600 bps、8N1 |
| ESP32-C3 | USART3，PB10 TX / PB11 RX，9600 bps、8N1 |

波形更新在 TIM1 回调中运行，触屏解析、无线命令、ADC 读取及界面更新在主循环执行。主循环仍包含阻塞串口发送、ADC 轮询和页面切换延时，不应描述为完全无阻塞。中断执行时间、抖动及通信压力下的输出行为需要实测。

## 波形模式

- `MODE_CONSTANT_I`：按目标电流持续输出。
- `MODE_EXPONENTIAL`：sin² 上升后指数衰减；当前代码上升时间为 2.0 ms 或 0.5 ms，不是历史文档中的 4 ms。
- `MODE_BIPHASIC_DC`：按直偏值与半幅值产生两个电平，目标电流限制为 0–10 mA。该命名本身不能证明负载端反向电流或零净电荷。
- `MODE_NMES_SWALLOW`：8 s 梯形包络演示，内部脉冲使用固定 20 ms 周期；此分支不使用通用频率和脉宽参数生成载波。

模式名称来自课程任务，不构成治疗方案或疗效结论。

## 标定与阻抗估计

`Current_To_DAC()` 使用十一点标定表与线性插值。ADC 读取电压后还原分压，计算等效阻抗，再补偿 104.7 kΩ 并联支路：

```text
R_parallel = V_load / I_set
R_load = R_parallel × 104700 / (104700 − R_parallel)
```

当前代码使用设定电流，不是独立电流采样。动态波形下的采样时刻和瞬时电流对应关系尚需验证。50000 Ω 是软件数值限幅，不能据此认定测量范围或开路保护已通过验证。

## 当前行为

- `g_stim.is_running` 初始为 1，默认设定电流为 5 mA。
- 倒计时由中断计数，归零后复位到 1200 s，未实现到时停止。
- `#0` / `#E` 设置电流为零并停止运行，属于经串口传递的软件停止命令。
- 当前源码未建立完整的开路故障锁存、独立硬件关断和故障注入验证记录。
- 触屏曲线按参数绘制，不能代替负载端测量。

这些行为在本次整理中保持原样。验证资料和后续检查项见 [实验记录](../docs/validation.md)。
