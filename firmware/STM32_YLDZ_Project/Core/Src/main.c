/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 神经肌肉刺激仪核心控制系统 (单向主控稳定版)
  *                   基本要求 (1)(2)(3)(4) + 拓展要求 (1)(2)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dac.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f1xx_hal.h"
#include "stdlib.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
#include "math.h"
#include "tjc_usart_hmi.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
unsigned int lh_ADC0 = 0; // ADC采集值 0-3.3V对应0-4096
unsigned int lh_DAC1 = 0; // DAC输出值 0-4096对应0-3.3V
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define FRAME_LENGTH 7
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// 全局刺激状态结构体
Stim_Config_TypeDef g_stim = {
    .mode               = MODE_CONSTANT_I,  // 默认模式: 基本要求(2) 恒流输出 (纯直线)
    .current_mA         = 5.0f,             // 默认电流: 5.0 mA (0~10mA 可调，步进 1mA)
    .freq_Hz            = 20,               // 默认频率: 20 Hz (1~200Hz 可调)
    .pulse_width_us     = 500,              // 默认脉宽: 500 us (100~1000us 可调)
    .tau_ms             = 15.0f,            // 默认衰减时间常数: 15.0 ms
    .dc_offset_mA       = 3.0f,             // 默认直偏: 3.0 mA
    .measured_impedance = 4000.0f,          // 负载阻抗
    .is_running         = 1,                // 运行状态
    .countdown_s        = 1200              // 倒计时: 1200s
};

char str[128];
uint8_t g_is_viewing_wave = 0; // 是否处于 Page 4 波形查看页
uint32_t g_wave_enter_tick = 0; // 进入波形页的时间戳 (防触控误触闪退)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static inline uint16_t Current_To_DAC(float mA);
void Sync_Page0_Params(void)
{
    char sbuf[48];
    sprintf(sbuf, "page0.n0.val=%d", (int)g_stim.countdown_s);
    tjc_send_string(sbuf);
    sprintf(sbuf, "page0.x0.val=%d", (int)(g_stim.current_mA * 10.0f));
    tjc_send_string(sbuf);
    sprintf(sbuf, "page0.n2.val=%d", g_stim.freq_Hz);
    tjc_send_string(sbuf);
    sprintf(sbuf, "page0.n3.val=%d", (int)g_stim.tau_ms);
    tjc_send_string(sbuf);
    sprintf(sbuf, "page0.n1.val=%d", (int)g_stim.pulse_width_us);
    tjc_send_string(sbuf);
    sprintf(sbuf, "page0.n4.val=%d", (int)g_stim.measured_impedance);
    tjc_send_string(sbuf);
    if (g_stim.mode == MODE_EXPONENTIAL)
    {
        tjc_send_string("page0.t5.txt=\"\xd6\xb8\xca\xfd\xcb\xa5\xbc\xf5\xb2\xa8\"");
        tjc_send_string("page0.t6.txt=\"\xcd\xcc\xd1\xca\xb4\xcc\xbc\xa4\"");
    }
    else if (g_stim.mode == MODE_BIPHASIC_DC)
    {
        tjc_send_string("page0.t5.txt=\"\xb6\xd4\xb3\xc6\xcb\xab\xcf\xf2\xb7\xbd\xb2\xa8\"");
        tjc_send_string("page0.t6.txt=\"\xcb\xab\xcf\xf2\xb7\xbd\xb2\xa8\"");
    }
    else
    {
        tjc_send_string("page0.t5.txt=\"\xd6\xb1\xc1\xf7\xc6\xbd\xd6\xb1\xcf\xdf\"");
        tjc_send_string("page0.t6.txt=\"\xba\xe3\xc1\xf7\xca\xe4\xb3\xf6\"");
    }
}

void Sync_All_Screen_Params(void);
void Process_Touchscreen_Command(void);
void Draw_Wave_Scale_Init(void);
void Draw_Wave_Scale_Overlay(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief 绘制/周期刷新示波器外框与刻度数值 (防擦除常驻层)
 */
void Draw_Wave_Scale_Overlay(void)
{
    char buf[80];

    // 绘制示波器外边框 (高亮翠绿框 RGB565=2016，包围波形区 X:38~318, Y:28~212)
    tjc_send_string("draw 38,28,318,212,2016");

    // 绘制标尺刻度线与文字
    if (g_stim.mode == MODE_BIPHASIC_DC)
    {
        // 双向对称方波：以 Y=120 为 0mA 零位中轴，每 30 像素 = 5mA
        tjc_send_string("line 30,120,38,120,65535"); // 0mA 零位主标尺齿 (白色长齿)
        tjc_send_string("line 34,60,38,60,65504");   // +10mA (黄色标尺齿)
        tjc_send_string("line 34,90,38,90,65504");   // +5mA
        tjc_send_string("line 34,150,38,150,65504"); // -5mA
        tjc_send_string("line 34,180,38,180,65504"); // -10mA

        // 左侧电流刻度数值 (sta=3 透明无底框)
        tjc_send_string("xstr 0,52,36,16,0,65504,0,2,1,3,\"+10\"");
        tjc_send_string("xstr 0,82,36,16,0,65504,0,2,1,3,\"+5\"");
        tjc_send_string("xstr 0,112,36,16,0,65535,0,2,1,3,\" 0mA\"");
        tjc_send_string("xstr 0,142,36,16,0,65504,0,2,1,3,\"-5\"");
        tjc_send_string("xstr 0,172,36,16,0,65504,0,2,1,3,\"-10\"");

        // 顶部时基状态栏
        sprintf(buf, "xstr 5,5,310,18,0,65504,0,0,1,3,\"CH1:5mA/div 5ms/div F:%dHz PW:%dus I:%.1f\"",
                g_stim.freq_Hz, (int)g_stim.pulse_width_us, g_stim.current_mA);
        tjc_send_string(buf);
    }
    else
    {
        // 仿生指数波 / 直流恒流：以 Y=180 为 0mA 基线，每 30 像素 = 2.5mA
        tjc_send_string("line 30,180,38,180,65535"); // 0mA 零位主标尺齿
        tjc_send_string("line 34,150,38,150,65504"); // 2.5mA
        tjc_send_string("line 34,120,38,120,65504"); // 5.0mA
        tjc_send_string("line 34,90,38,90,65504");   // 7.5mA
        tjc_send_string("line 34,60,38,60,65504");   // 10.0mA

        // 左侧电流刻度数值
        tjc_send_string("xstr 0,52,36,16,0,65504,0,2,1,3,\" 10mA\"");
        tjc_send_string("xstr 0,82,36,16,0,65504,0,2,1,3,\" 7.5\"");
        tjc_send_string("xstr 0,112,36,16,0,65504,0,2,1,3,\" 5.0\"");
        tjc_send_string("xstr 0,142,36,16,0,65504,0,2,1,3,\" 2.5\"");
        tjc_send_string("xstr 0,172,36,16,0,65535,0,2,1,3,\" 0mA\"");

        // 顶部时基状态栏
        sprintf(buf, "xstr 5,5,310,18,0,65504,0,0,1,3,\"CH1:2.5mA/div 5ms/div F:%dHz I:%.1fmA\"",
                g_stim.freq_Hz, g_stim.current_mA);
        tjc_send_string(buf);
    }

    // 底部返回按键指引
    tjc_send_string("xstr 40,218,270,18,0,33808,0,1,1,3,\"[ Touch anywhere to return ]\"");
}

/**
 * @brief 示波器界面初始化
 */
void Draw_Wave_Scale_Init(void)
{
    // 清空当前波形历史数据并初始化绘制
    tjc_send_string("cle 1,0");
    HAL_Delay(30);
    Draw_Wave_Scale_Overlay();
}

/**
 * @brief 电流(mA)逐点精准标定表 (2.2kΩ 终极微调，误差 < ±0.2%)
 * 0mA  -> 0    (0.00V,  0.00mA)
 * 1mA  -> 194  (2.20V,  1.00mA, 实测 2.198V)
 * 2mA  -> 418  (4.40V,  2.00mA, 实测 4.402V)
 * 3mA  -> 628  (6.60V,  3.00mA, 实测微调)
 * 4mA  -> 837  (8.80V,  4.00mA, 实测微调)
 * 5mA  -> 1047 (11.00V, 5.00mA, 实测微调)
 * 6mA  -> 1256 (13.20V, 6.00mA, 实测微调)
 * 7mA  -> 1466 (15.40V, 7.00mA, 实测微调)
 * 8mA  -> 1675 (17.60V, 8.00mA, 实测微调)
 * 9mA  -> 1884 (19.80V, 9.00mA, 实测微调)
 * 10mA -> 2095 (22.00V, 10.00mA, 实测微调)
 */
static const uint16_t s_calibrated_dac_table[11] = {
    0, 194, 418, 628, 837, 1047, 1256, 1466, 1675, 1884, 2095
};

static inline uint16_t Current_To_DAC(float mA)
{
    if (mA <= 0.0f) return 0;
    if (mA >= 10.0f) return s_calibrated_dac_table[10];

    int idx = (int)mA;
    float frac = mA - (float)idx;
    uint32_t dac = (uint32_t)(s_calibrated_dac_table[idx] + frac * (s_calibrated_dac_table[idx + 1] - s_calibrated_dac_table[idx]) + 0.5f);
    if (dac > 4095) dac = 4095;
    return (uint16_t)dac;
}

/**
 * @brief 初始化同步屏幕参数 (进入Page 3时由单片机主动推流初始化，杜绝显示0)
 */
void Sync_All_Screen_Params(void)
{
    sprintf(str, "n0.val=%d", (int)g_stim.current_mA);
    tjc_send_string(str);
    sprintf(str, "n1.val=%d", g_stim.freq_Hz);
    tjc_send_string(str);
    sprintf(str, "n2.val=%d", g_stim.pulse_width_us);
    tjc_send_string(str);
    sprintf(str, "n5.val=%d", (int)g_stim.tau_ms);
    tjc_send_string(str);
    sprintf(str, "n4.val=%d", g_stim.countdown_s);
    tjc_send_string(str);
}

/**
 * @brief 串口屏事件解析 (单向权威主控模式，杜绝异步轮询冲突)
 */
void Process_Touchscreen_Command(void)
{
    static uint32_t s_last_btn_tick = 0;

    while (usize >= 1)
    {
        uint8_t head = u(0);

        // 1. 标准 0x65 按键事件帧 (固定 7 字节: 65 <p1> <p2> <p3> FF FF FF)
        if (head == 0x65)
        {
            if (usize < 7)
            {
                break; // 帧未完全接收，退出等待下一字节，绝不丢弃头字节！
            }

            if (u(4) == 0xff && u(5) == 0xff && u(6) == 0xff)
            {
                uint8_t p1 = u(1); // 页面 ID
                uint8_t p2 = u(2); // 控件/按键 ID
                uint8_t p3 = u(3); // 事件: 0x01按下, 0x00弹起

                // Page 0 界面按键交互 (完整适配用户自定义UI)
                if (p1 == 0x00)
                {
                    g_is_viewing_wave = 0; // 在主页触控任意按键时，确保波形推流处于关闭状态
                    // b0: 复位按键 (ID 10 / 0x0A)
                    if (p2 == 10 || p2 == 0x0A)
                    {
                        g_stim.countdown_s = 1200;
                        g_stim.is_running = 1;
                        sprintf(str, "page0.n0.val=1200");
                        tjc_send_string(str);
                    }
                    // b1: 电流加 (ID 11) -> 步进 +1.0mA，最高 10.0mA
                    else if (p2 == 11)
                    {
                        if (g_stim.current_mA < 10.0f) g_stim.current_mA += 1.0f;
                        int current_display = (int)(g_stim.current_mA * 10.0f);
                        sprintf(str, "page0.x0.val=%d", current_display);
                        tjc_send_string(str);
                    }
                    // b2: 电流减 (ID 12) -> 步进 -1.0mA，最低 0.0mA
                    else if (p2 == 12)
                    {
                        if (g_stim.current_mA >= 1.0f) g_stim.current_mA -= 1.0f;
                        int current_display = (int)(g_stim.current_mA * 10.0f);
                        sprintf(str, "page0.x0.val=%d", current_display);
                        tjc_send_string(str);
                    }
                    // b3: 频率加 (ID 13) -> 步进 +5Hz，最高 200Hz
                    else if (p2 == 13)
                    {
                        if (g_stim.freq_Hz < 5) g_stim.freq_Hz = 5;
                        else if (g_stim.freq_Hz <= 195) g_stim.freq_Hz += 5;
                        else g_stim.freq_Hz = 200;
                        sprintf(str, "page0.n2.val=%d", g_stim.freq_Hz);
                        tjc_send_string(str);
                    }
                    // b4: 频率减 (ID 14) -> 步进 -5Hz，最低 1Hz
                    else if (p2 == 14)
                    {
                        if (g_stim.freq_Hz > 5) g_stim.freq_Hz -= 5;
                        else g_stim.freq_Hz = 1;
                        sprintf(str, "page0.n2.val=%d", g_stim.freq_Hz);
                        tjc_send_string(str);
                    }
                    // b6: 时间常数 TAU 加 (ID 18) -> 步进 +5ms，最高 45ms
                    else if (p2 == 18)
                    {
                        if (g_stim.tau_ms <= 40.0f) g_stim.tau_ms += 5.0f;
                        sprintf(str, "page0.n3.val=%d", (int)g_stim.tau_ms);
                        tjc_send_string(str);
                    }
                    // b7: 时间常数 TAU 减 (ID 19) -> 步进 -5ms，最低 5ms
                    else if (p2 == 19)
                    {
                        if (g_stim.tau_ms >= 10.0f) g_stim.tau_ms -= 5.0f;
                        sprintf(str, "page0.n3.val=%d", (int)g_stim.tau_ms);
                        tjc_send_string(str);
                    }
                    // b9: 脉宽 PW 加 (ID 23) -> 步进 +50us，最高 1000us
                    else if (p2 == 23)
                    {
                        if (g_stim.pulse_width_us <= 950) g_stim.pulse_width_us += 50;
                        else g_stim.pulse_width_us = 1000;
                        sprintf(str, "page0.n1.val=%d", (int)g_stim.pulse_width_us);
                        tjc_send_string(str);
                    }
                    // b10: 脉宽 PW 减 (ID 24) -> 步进 -50us，最低 100us
                    else if (p2 == 24)
                    {
                        if (g_stim.pulse_width_us >= 150) g_stim.pulse_width_us -= 50;
                        else g_stim.pulse_width_us = 100;
                        sprintf(str, "page0.n1.val=%d", (int)g_stim.pulse_width_us);
                        tjc_send_string(str);
                    }
                    // 模式切换按钮 (ID 17) -> 联动更新 t5(波形显示) 与 t6(模式显示)
                    else if (p2 == 17)
                    {
                        if (g_stim.mode == MODE_CONSTANT_I)
                        {
                            g_stim.mode = MODE_EXPONENTIAL;
                            // t5: 指数衰减波, t6: 吞咽刺激
                            sprintf(str, "page0.t5.txt=\"\xd6\xb8\xca\xfd\xcb\xa5\xbc\xf5\xb2\xa8\"");
                            tjc_send_string(str);
                            sprintf(str, "page0.t6.txt=\"\xcd\xcc\xd1\xca\xb4\xcc\xbc\xa4\"");
                            tjc_send_string(str);
                        }
                        else if (g_stim.mode == MODE_EXPONENTIAL)
                        {
                            g_stim.mode = MODE_BIPHASIC_DC;
                            // t5: 对称双向方波, t6: 双向方波
                            sprintf(str, "page0.t5.txt=\"\xb6\xd4\xb3\xc6\xcb\xab\xcf\xf2\xb7\xbd\xb2\xa8\"");
                            tjc_send_string(str);
                            sprintf(str, "page0.t6.txt=\"\xcb\xab\xcf\xf2\xb7\xbd\xb2\xa8\"");
                            tjc_send_string(str);
                        }
                        else
                        {
                            g_stim.mode = MODE_CONSTANT_I;
                            // t5: 直流平直线, t6: 恒流输出
                            sprintf(str, "page0.t5.txt=\"\xd6\xb1\xc1\xf7\xc6\xbd\xd6\xb1\xcf\xdf\"");
                            tjc_send_string(str);
                            sprintf(str, "page0.t6.txt=\"\xba\xe3\xc1\xf7\xca\xe4\xb3\xf6\"");
                            tjc_send_string(str);
                        }
                    }
                    // b26: 查看波形按钮 (ID 26 / 0x1A) -> 切换至 page1 并开启示波器推流
                    else if (p2 == 26 || p2 == 0x1A)
                    {
                        g_is_viewing_wave = 1;
                        g_wave_enter_tick = HAL_GetTick();
                        tjc_send_string("page page1");
                        HAL_Delay(50);
                        Draw_Wave_Scale_Init();
                    }
                }
                // Page 2 模式选择页
                else if (p1 == 0x02)
                {
                    if (p2 == 0x00) // 返回
                    {
                        sprintf(str, "page 0");
                        tjc_send_string(str);
                    }
                    else if (p2 == 0x01) // 恒流方波模式
                    {
                        g_stim.mode = MODE_CONSTANT_I;
                        g_stim.current_mA = 5.0f;
                        g_stim.freq_Hz = 20;
                        g_stim.pulse_width_us = 500;
                        g_stim.is_running = 1;
                        g_stim.countdown_s = 60;
                        sprintf(str, "page 3");
                        tjc_send_string(str);
                        HAL_Delay(50);
                        Sync_All_Screen_Params();
                    }
                    else if (p2 == 0x02) // 直偏双向方波
                    {
                        g_stim.mode = MODE_BIPHASIC_DC;
                        g_stim.current_mA = 6.0f;
                        g_stim.dc_offset_mA = 3.0f;
                        g_stim.freq_Hz = 20;
                        g_stim.pulse_width_us = 500;
                        g_stim.is_running = 1;
                        g_stim.countdown_s = 60;
                        sprintf(str, "page 3");
                        tjc_send_string(str);
                        HAL_Delay(50);
                        Sync_All_Screen_Params();
                    }
                    else if (p2 == 0x07) // NMES 吞咽波形模式
                    {
                        g_stim.mode = MODE_NMES_SWALLOW;
                        g_stim.current_mA = 6.0f;
                        g_stim.freq_Hz = 50;
                        g_stim.is_running = 1;
                        g_stim.countdown_s = 60;
                        sprintf(str, "page 3");
                        tjc_send_string(str);
                        HAL_Delay(50);
                        Sync_All_Screen_Params();
                    }
                    else if (p2 == 0x08) // 指数衰减模式
                    {
                        g_stim.mode = MODE_EXPONENTIAL;
                        g_stim.current_mA = 8.0f;
                        g_stim.freq_Hz = 10;
                        g_stim.tau_ms = 15.0f;
                        g_stim.is_running = 1;
                        g_stim.countdown_s = 60;
                        sprintf(str, "page 3");
                        tjc_send_string(str);
                        HAL_Delay(50);
                        Sync_All_Screen_Params();
                    }
                }
                // Page 3 参数调节页
                else if (p1 == 0x03)
                {
                    if (HAL_GetTick() - s_last_btn_tick > 80)
                    {
                        s_last_btn_tick = HAL_GetTick();

                        switch (p2)
                        {
                            case 0x00: // b0: 电流 (+)
                                if (g_stim.current_mA < 10.0f) g_stim.current_mA += 1.0f;
                                sprintf(str, "n0.val=%d", (int)g_stim.current_mA);
                                tjc_send_string(str);
                                break;

                            case 0x01: // b1: 电流 (-)
                                if (g_stim.current_mA > 1.0f) g_stim.current_mA -= 1.0f;
                                sprintf(str, "n0.val=%d", (int)g_stim.current_mA);
                                tjc_send_string(str);
                                break;

                            case 0x02: // b2: 频率 (+)
                                if (g_stim.freq_Hz < 5) g_stim.freq_Hz = 5;
                                else if (g_stim.freq_Hz <= 195) g_stim.freq_Hz += 5;
                                else g_stim.freq_Hz = 200;
                                sprintf(str, "n1.val=%d", g_stim.freq_Hz);
                                tjc_send_string(str);
                                break;

                            case 0x03: // b3: 频率 (-)
                                if (g_stim.freq_Hz > 5) g_stim.freq_Hz -= 5;
                                else g_stim.freq_Hz = 1;
                                sprintf(str, "n1.val=%d", g_stim.freq_Hz);
                                tjc_send_string(str);
                                break;

                            case 0x08: // b8: 脉宽 (+)
                                if (g_stim.pulse_width_us <= 950) g_stim.pulse_width_us += 50;
                                else g_stim.pulse_width_us = 1000;
                                sprintf(str, "n2.val=%d", g_stim.pulse_width_us);
                                tjc_send_string(str);
                                break;

                            case 0x07: // b7: 脉宽 (-)
                                if (g_stim.pulse_width_us >= 150) g_stim.pulse_width_us -= 50;
                                else g_stim.pulse_width_us = 100;
                                sprintf(str, "n2.val=%d", g_stim.pulse_width_us);
                                tjc_send_string(str);
                                break;

                            case 0x0B: // b11: 时间常数 (+)
                                if (g_stim.tau_ms <= 45.0f) g_stim.tau_ms += 5.0f;
                                sprintf(str, "n5.val=%d", (int)g_stim.tau_ms);
                                tjc_send_string(str);
                                break;

                            case 0x0C: // b12: 时间常数 (-)
                                if (g_stim.tau_ms >= 10.0f) g_stim.tau_ms -= 5.0f;
                                sprintf(str, "n5.val=%d", (int)g_stim.tau_ms);
                                tjc_send_string(str);
                                break;

                            case 0x04: // b4: 启动
                                g_stim.is_running = 1;
                                break;

                            case 0x0A: // b10: 停止
                                g_stim.is_running = 0;
                                break;

                            case 0x05: // b5: 复位
                                g_stim.current_mA = 5.0f;
                                g_stim.freq_Hz = 20;
                                g_stim.pulse_width_us = 500;
                                g_stim.tau_ms = 15.0f;
                                g_stim.countdown_s = 60;
                                g_stim.is_running = 1;
                                Sync_All_Screen_Params();
                                break;

                            case 0x06: // b6: 选模式
                                g_stim.is_running = 0;
                                g_is_viewing_wave = 0;
                                sprintf(str, "page 2");
                                tjc_send_string(str);
                                break;

                            case 0x09: // b9: 看波形
                                g_is_viewing_wave = 1;
                                g_wave_enter_tick = HAL_GetTick();
                                sprintf(str, "page 4");
                                tjc_send_string(str);
                                HAL_Delay(50);
                                Draw_Wave_Scale_Init();
                                break;

                            default:
                                break;
                        }
                    }
                }
                // Page 1 波形显示页 (点屏幕任意处返回 Page 0)
                else if (p1 == 0x01)
                {
                    // 彻底屏蔽刚进入波形页1000ms内以及手指抬起事件(p3==0)，杜绝闪退回跳！
                    if ((HAL_GetTick() - g_wave_enter_tick >= 1000) && (p3 == 0x01))
                    {
                        g_is_viewing_wave = 0;
                        tjc_send_string("page page0");
                        Sync_Page0_Params();
                    }
                }
                // Page 4 波形显示页 (点任意处返回)
                else if (p1 == 0x04)
                {
                    if ((HAL_GetTick() - g_wave_enter_tick >= 1000) && (p3 == 0x01))
                    {
                        g_is_viewing_wave = 0;
                        sprintf(str, "page 3");
                        tjc_send_string(str);
                        HAL_Delay(50);
                        Sync_All_Screen_Params();
                    }
                }

                udelete(7);
            }
            else
            {
                udelete(1); // 帧尾不符合，滑动跳过该字节
            }
        }
        // 2. 页面主动切换 0x66: 66 <page_id> FF FF FF (5字节)
        else if (head == 0x66)
        {
            if (usize < 5)
            {
                break; // 帧未完全接收，退出等待
            }

            if (u(2) == 0xff && u(3) == 0xff && u(4) == 0xff)
            {
                uint8_t page_id = u(1);
                if (page_id == 0x01)
                {
                    g_is_viewing_wave = 1;
                    g_wave_enter_tick = HAL_GetTick();
                    HAL_Delay(30);
                    Draw_Wave_Scale_Init();
                }
                else if (page_id == 0x00)
                {
                    g_is_viewing_wave = 0;
                    Sync_Page0_Params();
                }
                else if (page_id == 0x03)
                {
                    g_is_viewing_wave = 0;
                    HAL_Delay(30);
                    Sync_All_Screen_Params();
                }
                else if (page_id == 0x04)
                {
                    g_is_viewing_wave = 1;
                    g_wave_enter_tick = HAL_GetTick();
                    HAL_Delay(30);
                    Draw_Wave_Scale_Init();
                }
                else
                {
                    g_is_viewing_wave = 0;
                }
                udelete(5);
            }
            else
            {
                udelete(1);
            }
        }
        // 3. 过滤应答帧与错误返回
        else if (head == 0x71)
        {
            if (usize < 8) break;
            udelete(8);
        }
        else if (head == 0x01 || head == 0x1A || head == 0x00 || head == 0x1C || head == 0x24)
        {
            if (usize < 4) break;
            udelete(4);
        }
        else
        {
            udelete(1); // 杂波字节直接丢弃
        }
    }
}
void Process_ESP32_Byte(uint8_t ch)
{
    static uint8_t s_cmd_prefix = 0;

    // 前缀匹配: 仅响应带 # 或 $ 前缀的精确控制指令 (彻底杜绝回显和日志数字干扰)
    if (ch == '#' || ch == '$')
    {
        s_cmd_prefix = 1;
        return;
    }

    if (!s_cmd_prefix)
    {
        return; // 忽略所有非前缀字符 (过滤所有后台日志与状态回显数字，杜绝乱跳)
    }
    s_cmd_prefix = 0; // 消耗前缀

    char sbuf[64];
    char resp[96];

    // 电流增加 +1mA
    if (ch == '+')
    {
        if (g_stim.current_mA <= 9.0f) g_stim.current_mA += 1.0f;
        sprintf(sbuf, "page0.x0.val=%d", (int)(g_stim.current_mA * 10.0f));
        tjc_send_string(sbuf);
        sprintf(resp, "OK: Current=%.1fmA\r\n", g_stim.current_mA);
        HAL_UART_Transmit(&huart3, (uint8_t*)resp, strlen(resp), 50);
    }
    // 电流减少 -1mA
    else if (ch == '-')
    {
        if (g_stim.current_mA >= 1.0f) g_stim.current_mA -= 1.0f;
        sprintf(sbuf, "page0.x0.val=%d", (int)(g_stim.current_mA * 10.0f));
        tjc_send_string(sbuf);
        sprintf(resp, "OK: Current=%.1fmA\r\n", g_stim.current_mA);
        HAL_UART_Transmit(&huart3, (uint8_t*)resp, strlen(resp), 50);
    }
    // 启停切换 (仅大写 'S')
    else if (ch == 'S')
    {
        g_stim.is_running = !g_stim.is_running;
        sprintf(resp, "OK: Timer=%s\r\n", g_stim.is_running ? "RUNNING" : "PAUSED");
        HAL_UART_Transmit(&huart3, (uint8_t*)resp, strlen(resp), 50);
    }
    // 状态遥测 (仅大写 'R')
    else if (ch == 'R')
    {
        sprintf(resp, "STIM: I=%.1fmA, Freq=%dHz, Tau=%dms, PW=%dus, Time=%ds, Imp=%.0f\r\n",
                g_stim.current_mA, g_stim.freq_Hz, (int)g_stim.tau_ms, (int)g_stim.pulse_width_us, (int)g_stim.countdown_s, g_stim.measured_impedance);
        HAL_UART_Transmit(&huart3, (uint8_t*)resp, strlen(resp), 50);
    }
    // 处方一：吞咽神经肌肉刺激 (NMES 吞咽康复)
    else if (ch == '1')
    {
        g_stim.mode = MODE_EXPONENTIAL;
        g_stim.current_mA = 5.0f;
        g_stim.freq_Hz = 20;
        g_stim.tau_ms = 15.0f;
        g_stim.pulse_width_us = 500;
        g_stim.countdown_s = 1200;
        g_stim.is_running = 1;

        sprintf(sbuf, "page0.t5.txt=\"\xd6\xb8\xca\xfd\xcb\xa5\xbc\xf5\xb2\xa8\"");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.t6.txt=\"\xcd\xcc\xd1\xca\xb4\xcc\xbc\xa4\"");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.x0.val=50");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.n2.val=20");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.n3.val=15");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.n1.val=500");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.n0.val=1200");
        tjc_send_string(sbuf);

        sprintf(resp, "PLAN: 1 (Swallow Active)\r\n");
        HAL_UART_Transmit(&huart3, (uint8_t*)resp, strlen(resp), 50);
    }
    // 处方二：对称双向方波 (肌力强化/抗疲劳)
    else if (ch == '2')
    {
        g_stim.mode = MODE_BIPHASIC_DC;
        g_stim.current_mA = 6.0f;
        g_stim.dc_offset_mA = 3.0f;
        g_stim.freq_Hz = 20;
        g_stim.pulse_width_us = 500;
        g_stim.countdown_s = 1200;
        g_stim.is_running = 1;

        sprintf(sbuf, "page0.t5.txt=\"\xb6\xd4\xb3\xc6\xcb\xab\xcf\xf2\xb7\xbd\xb2\xa8\"");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.t6.txt=\"\xcb\xab\xcf\xf2\xb7\xbd\xb2\xa8\"");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.x0.val=60");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.n2.val=20");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.n1.val=500");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.n0.val=1200");
        tjc_send_string(sbuf);

        sprintf(resp, "PLAN: 2 (Biphasic Active)\r\n");
        HAL_UART_Transmit(&huart3, (uint8_t*)resp, strlen(resp), 50);
    }
    // 处方三：直流恒流输出
    else if (ch == '3')
    {
        g_stim.mode = MODE_CONSTANT_I;
        g_stim.current_mA = 5.0f;
        g_stim.countdown_s = 600;
        g_stim.is_running = 1;

        sprintf(sbuf, "page0.t5.txt=\"\xd6\xb1\xc1\xf7\xc6\xbd\xd6\xb1\xcf\xdf\"");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.t6.txt=\"\xba\xe3\xc1\xf7\xca\xe4\xb3\xf6\"");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.x0.val=50");
        tjc_send_string(sbuf);
        sprintf(sbuf, "page0.n0.val=600");
        tjc_send_string(sbuf);

        sprintf(resp, "PLAN: 3 (Constant_I Active)\r\n");
        HAL_UART_Transmit(&huart3, (uint8_t*)resp, strlen(resp), 50);
    }
    // 紧急制动 / 急停断电归零
    else if (ch == '0' || ch == 'E')
    {
        g_stim.current_mA = 0.0f;
        g_stim.is_running = 0;
        sprintf(sbuf, "page0.x0.val=0");
        tjc_send_string(sbuf);
        sprintf(resp, "EMERGENCY: ZERO & STOPPED\r\n");
        HAL_UART_Transmit(&huart3, (uint8_t*)resp, strlen(resp), 50);
    }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* MCU Configuration--------------------------------------------------------*/
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_DAC_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */
  initRingBuffer();                            // 初始化环形缓冲区
  __HAL_UART_CLEAR_OREFLAG(&TJC_UART);         // 清除溢出标志
  HAL_UART_Receive_IT(&TJC_UART, RxBuffer, 1);
  __HAL_UART_CLEAR_OREFLAG(&huart3);
  HAL_UART_Receive_IT(&huart3, &g_esp_rx_byte, 1); // 开启USART2接收中断
  HAL_ADCEx_Calibration_Start(&hadc1);         // 校准ADC1
  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);         // 启动DAC通道1 (PA4)
  HAL_TIM_Base_Start_IT(&htim1);               // TIM1 20kHz (50us) 中断
  HAL_Delay(300);                              // 稳压延时

  // 开机全量同步屏幕控件
  sprintf(str, "page0.n0.val=%d", (int)g_stim.countdown_s);
  tjc_send_string(str);
  sprintf(str, "page0.x0.val=%d", (int)(g_stim.current_mA * 10.0f));
  tjc_send_string(str);
  sprintf(str, "page0.n2.val=%d", g_stim.freq_Hz);
  tjc_send_string(str);
  sprintf(str, "page0.n3.val=%d", (int)g_stim.tau_ms);
  tjc_send_string(str);
  sprintf(str, "page0.n1.val=%d", (int)g_stim.pulse_width_us);
  tjc_send_string(str);
  sprintf(str, "page0.n4.val=%d", (int)g_stim.measured_impedance);
  tjc_send_string(str);
  sprintf(str, "page0.t5.txt=\"\xd6\xb1\xc1\xf7\xc6\xbd\xd6\xb1\xcf\xdf\""); // t5: 直流平直线
  tjc_send_string(str);
  sprintf(str, "page0.t6.txt=\"\xba\xe3\xc1\xf7\xca\xe4\xb3\xf6\"");         // t6: 恒流输出
  tjc_send_string(str);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  uint32_t last_screen_tick = 0;
  uint32_t last_sec_tick = 0;

  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
        // 1. 处理来自 ESP32-C3 的无线控制指令 (在主循环执行，杜绝中断死锁与阻塞)
    while (g_esp_rx_head != g_esp_rx_tail)
    {
        uint8_t ch = g_esp_rx_buf[g_esp_rx_tail];
        g_esp_rx_tail = (g_esp_rx_tail + 1) % ESP_RX_BUF_SIZE;
        Process_ESP32_Byte(ch);
    }

    // 2. 周期刷新屏幕倒计时与参数 (每秒一次，使用局部缓冲，在非看波状态下同步)
    if (HAL_GetTick() - last_sec_tick >= 1000)
    {
        last_sec_tick = HAL_GetTick();
        if (g_stim.is_running && g_stim.countdown_s > 0)
        {
            if (!g_is_viewing_wave)
            {
                char sbuf[32];
                sprintf(sbuf, "page0.n0.val=%d", (int)g_stim.countdown_s);
                tjc_send_string(sbuf);
                int current_display = (int)(g_stim.current_mA * 10.0f);
                sprintf(sbuf, "page0.x0.val=%d", current_display);
                tjc_send_string(sbuf);
                sprintf(sbuf, "page0.n2.val=%d", g_stim.freq_Hz);
                tjc_send_string(sbuf);
                sprintf(sbuf, "page0.n3.val=%d", (int)g_stim.tau_ms);
                tjc_send_string(sbuf);
                sprintf(sbuf, "page0.n1.val=%d", (int)g_stim.pulse_width_us);
                tjc_send_string(sbuf);

                // ADC采样 PA0 (ADC1_IN0) 计算负载两端阻抗 (16次均值滤波, 彻底消除开关电源纹波)
                uint32_t adc_sum = 0;
                int adc_cnt = 0;
                for (int i = 0; i < 16; i++)
                {
                    HAL_ADC_Start(&hadc1);
                    if (HAL_ADC_PollForConversion(&hadc1, 5) == HAL_OK)
                    {
                        adc_sum += HAL_ADC_GetValue(&hadc1);
                        adc_cnt++;
                    }
                }
                if (adc_cnt > 0)
                {
                    float adc_avg = (float)adc_sum / (float)adc_cnt;
                    float v_pa0 = (adc_avg / 4095.0f) * 3.3f; // PA0引脚实测电压 (0~3.3V)
                    const float K_DIVIDER = 22.2766f;         // (100k + 4.7k) / 4.7k 精密分压比
                    float v_load = v_pa0 * K_DIVIDER;         // 负载实际端电压 (V)

                    if (g_stim.is_running && g_stim.current_mA >= 0.5f)
                    {
                        // 1. 欧姆定律初步阻抗 (此时包含负载 Rx 与分压网络 104.7k 的并联等效阻抗)
                        float r_parallel = v_load / (g_stim.current_mA * 0.001f);

                        // 2. 戴维南并联逆向补偿 (消除 100k+4.7k 分压网络对大阻抗的分流误差，使误差 < 1%)
                        const float R_DIVIDER = 104700.0f; // 100k + 4.7k = 104.7kΩ
                        float r_comp = r_parallel;
                        if (r_parallel < R_DIVIDER - 1000.0f)
                        {
                            r_comp = (r_parallel * R_DIVIDER) / (R_DIVIDER - r_parallel);
                        }

                        // 3. 硬件精度精细标定系数 (校准硬件电阻容差与ADC基准，默认 1.000)
                        const float CAL_SCALE = 1.000f;
                        float r_final = r_comp * CAL_SCALE;

                        // 4. 顺从电压打顶与安全限幅 (54V 供电轨，输出摆幅上限约 48V)
                        if (r_final < 0.0f) r_final = 0.0f;
                        if (r_final > 50000.0f) r_final = 50000.0f; // 量程上限扩展至 50kΩ

                        g_stim.measured_impedance = r_final;
                    }
                    else
                    {
                        g_stim.measured_impedance = 0.0f;
                    }
                }
                sprintf(sbuf, "page0.n4.val=%d", (int)(g_stim.measured_impedance + 0.5f));
                tjc_send_string(sbuf);
            }
        }
    }

    // 3. 串口接收防锁死自愈机制 (同时保护串口屏 USART2 与无线模块 USART3)
    if (huart2.RxState == HAL_UART_STATE_READY)
    {
        __HAL_UART_CLEAR_OREFLAG(&TJC_UART);
        HAL_UART_Receive_IT(&TJC_UART, RxBuffer, 1);
    }
    if (huart3.RxState == HAL_UART_STATE_READY)
    {
        __HAL_UART_CLEAR_OREFLAG(&huart3);
        HAL_UART_Receive_IT(&huart3, &g_esp_rx_byte, 1);
    }

    // 4. 实时响应屏幕触控按键及换页交互
    Process_Touchscreen_Command();

    // 5. Page 1 示波器波形实时推流引擎 (25ms刷新一次，非阻塞流式渲染三种波形)
    if (g_is_viewing_wave)
    {
        static uint32_t s_last_wave_tick = 0;
        static uint32_t s_last_scale_tick = 0;
        static float s_wave_phase_norm = 0.0f;

        uint32_t now = HAL_GetTick();

        // 周期重绘示波器参数标尺与外框 (1000ms一次，保证刻度永久常驻)
        if (now - s_last_scale_tick >= 1000)
        {
            s_last_scale_tick = now;
            Draw_Wave_Scale_Overlay();
        }

        if (now - s_last_wave_tick >= 50)
        {
            s_last_wave_tick = now;

            int y_val = 30; // 默认基线

            if (!g_stim.is_running)
            {
                y_val = 30; // 停止状态输出零位底线
            }
            else
            {
                // 动态滚动归一化相位 (每个刷新周期滚动 0.04)
                s_wave_phase_norm += 0.04f;
                if (s_wave_phase_norm >= 1.0f) s_wave_phase_norm -= 1.0f;
                float t_norm = s_wave_phase_norm;

                // -------------------------------------------------------------
                // 波形 1: 仿生平缓动作电位波 (MODE_EXPONENTIAL)
                // -------------------------------------------------------------
                if (g_stim.mode == MODE_EXPONENTIAL)
                {
                    float t_rise_norm = 0.15f; // 前 15% 周期为平缓仿生升沿
                    float amp = 0.0f;
                    if (t_norm < t_rise_norm)
                    {
                        float factor = sinf((1.5707963f * t_norm) / t_rise_norm);
                        amp = g_stim.current_mA * factor * factor;
                    }
                    else
                    {
                        float decay = expf(-(t_norm - t_rise_norm) * 5.0f);
                        amp = g_stim.current_mA * decay;
                    }
                    y_val = 30 + (int)(amp * 12.0f); // 5mA -> 90, 10mA -> 150
                }
                // -------------------------------------------------------------
                // 波形 2: 对称双向方波 (MODE_BIPHASIC_DC)
                // -------------------------------------------------------------
                else if (g_stim.mode == MODE_BIPHASIC_DC)
                {
                    // 屏幕波形脉宽归一化比例 (100us~1000us 对应 0.10~0.35 视宽)
                    float pw_ratio = 0.10f + ((float)(g_stim.pulse_width_us - 100) / 900.0f) * 0.25f;
                    int base_center = 90; // 双向方波零位居中 (0~180正中)
                    int amp_step = (int)(g_stim.current_mA * 6.0f); // 5mA -> ±30, 10mA -> ±60

                    if (t_norm < pw_ratio)
                    {
                        y_val = base_center + amp_step; // 正相刺激
                    }
                    else if (t_norm < pw_ratio * 2.0f)
                    {
                        y_val = base_center - amp_step; // 负相电荷中和平衡
                    }
                    else
                    {
                        y_val = base_center;            // 间歇期
                    }
                }
                // -------------------------------------------------------------
                // 波形 3: 恒流平直线 (MODE_CONSTANT_I)
                // -------------------------------------------------------------
                else
                {
                    y_val = 30 + (int)(g_stim.current_mA * 12.0f); // 平直稳定直流电平
                }
            }

            // 纵坐标限幅在 5 ~ 220 之间，防止超出波形控件显示边界
            if (y_val < 5) y_val = 5;
            if (y_val > 220) y_val = 220;

            char wbuf[32];
            sprintf(wbuf, "add 1,0,%d", y_val);
            tjc_send_string(wbuf);
        }
    }
  }
  /* USER CODE END 3 */
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_PEFLAG(huart);
        HAL_UART_Receive_IT(huart, RxBuffer, 1);
    }
    else if (huart->Instance == USART3)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_PEFLAG(huart);
        HAL_UART_Receive_IT(huart, &g_esp_rx_byte, 1);
    }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
/**
 * @brief 定时器中断回调函数 (TIM1 20kHz 50us高精度微秒时基，支持 1~200Hz 与 100us~1ms 脉宽)
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    static uint32_t tick_50us   = 0;
    static uint32_t ms_counter  = 0;
    static uint32_t sec_counter = 0;
    static uint32_t led_counter = 0;
    static uint16_t s_last_dac  = 0xFFFF;

    if (htim == &htim1) // 20kHz (50us)
    {
        tick_50us++;
        ms_counter++;

        // 每 20 个 50us tick 为 1ms (20 * 50us = 1000us = 1ms)
        if (ms_counter >= 20)
        {
            ms_counter = 0;
            sec_counter++;
            led_counter++;

            // 板载 LED 运行状态心跳 (500ms 翻转)
            if (led_counter >= 500)
            {
                led_counter = 0;
                HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
            }

            // 倒计时秒级计数 (1000ms = 1秒)
            if (sec_counter >= 1000)
            {
                sec_counter = 0;
                if (g_stim.countdown_s > 0)
                {
                    g_stim.countdown_s--;
                }
                else
                {
                    g_stim.countdown_s = 1200; // 自动复位循环 (1200秒)
                }
            }
        }

        float target_I = 0.0f;
        uint32_t period_ticks = (g_stim.freq_Hz > 0) ? (20000 / g_stim.freq_Hz) : 1000;
        if (period_ticks == 0) period_ticks = 1;

        if (!g_stim.is_running)
        {
            target_I = 0.0f;
        }
        else
        {
            switch (g_stim.mode)
            {
                // =============================================================
                // 基本要求(1): 吞咽 NMES 梯形包络脉冲串 (8秒大周期)
                // =============================================================
                case MODE_NMES_SWALLOW:
                {
                    uint32_t swallow_phase_ms = (tick_50us / 20) % 8000;
                    float envelope = 0.0f;

                    if (swallow_phase_ms < 1000) {
                        envelope = (float)swallow_phase_ms / 1000.0f;
                    } else if (swallow_phase_ms < 3000) {
                        envelope = 1.0f;
                    } else if (swallow_phase_ms < 4000) {
                        envelope = 1.0f - ((float)(swallow_phase_ms - 3000) / 1000.0f);
                    } else {
                        envelope = 0.0f;
                    }

                    if (envelope > 0.0f && (swallow_phase_ms % 20 < 10)) {
                        target_I = g_stim.current_mA * envelope;
                    } else {
                        target_I = 0.0f;
                    }
                    break;
                }

                // =============================================================
                // 基本要求(2): 纯直流恒流输出 (0~10mA 步进1mA)
                // =============================================================
                case MODE_CONSTANT_I:
                {
                    target_I = g_stim.current_mA;
                    break;
                }

                // =============================================================
                // 基本要求(3): 指数衰减脉冲 (1~200Hz 频率可调, tau 可调)
                // =============================================================
                case MODE_EXPONENTIAL:
                {
                    uint32_t phase = tick_50us % period_ticks;
                    float t_ms = (float)phase * 0.05f;
                    float t_rise = (period_ticks >= 80) ? 2.0f : 0.5f;
                    float tau = (g_stim.tau_ms > 0.1f) ? g_stim.tau_ms : 15.0f;

                    if (t_ms < t_rise)
                    {
                        float factor = sinf((1.5707963f * t_ms) / t_rise);
                        target_I = g_stim.current_mA * factor * factor;
                    }
                    else
                    {
                        float decay = expf(-(t_ms - t_rise) / tau);
                        target_I = g_stim.current_mA * decay;
                        if (target_I < 0.05f) target_I = 0.0f;
                    }
                    break;
                }

                // =============================================================
                // 扩展要求(1): 带有直偏的对称双向方波 (1~200Hz, 脉宽 100us~1ms)
                // =============================================================
                case MODE_BIPHASIC_DC:
                {
                    uint32_t phase = tick_50us % period_ticks;
                    // g_stim.pulse_width_us: 100 ~ 1000 us -> 2 ~ 20 ticks
                    uint32_t pw_ticks = g_stim.pulse_width_us / 50;
                    if (pw_ticks == 0) pw_ticks = 2;
                    if (pw_ticks * 2 > period_ticks) pw_ticks = period_ticks / 2;

                    float idc = g_stim.dc_offset_mA;
                    float iamp = g_stim.current_mA * 0.5f;

                    if (phase < pw_ticks)
                    {
                        target_I = idc + iamp; // 正相
                    }
                    else if (phase < pw_ticks * 2)
                    {
                        target_I = idc - iamp; // 负相 (电荷中和)
                    }
                    else
                    {
                        target_I = idc;        // 间歇期基线
                    }
                    if (target_I < 0.0f) target_I = 0.0f;
                    if (target_I > 10.0f) target_I = 10.0f;
                    break;
                }

                default:
                    target_I = 0.0f;
                    break;
            }
        }

        // DAC实时输出更新 (带差分写入过滤与直接寄存器加速)
        uint16_t dac_val = Current_To_DAC(target_I);
        if (dac_val != s_last_dac)
        {
            s_last_dac = dac_val;
            lh_DAC1 = dac_val;
            DAC->DHR12R1 = dac_val;
            DAC->SWTRIGR = DAC_SWTRIGR_SWTRIG1;
        }
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}

#ifdef  USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif
