# MCXA156 光编码器演示应用

## 概述

该演示程序实现了多个关键模块，包括高速ADC采样、多摩川协议串口配置、CRC校验、ATAN2计算优化以及EQDC正交解码等功能。程序基于NXP MCXA156微控制器开发板设计，展示了该芯片的多种高级功能。

MDK工程位置: `sincos_encoder\boards\frdmmcxa156\driver_examples\sin_cos_encoder\sin_cos_encoder`

## 时钟配置

系统使用外部20MHz晶振，并通过FRO自动校准实现高精度时钟源。主要时钟配置：

- 内核时钟: 96MHz
- ADC时钟: 直接使用外部20MHz晶振作为输入时钟
- UART时钟: 直接使用外部20MHz晶振作为输入时钟
- 定时器时钟: 96MHz，用于高精度计时

## 开发环境

- Keil MDK
- SDK: SDK_2_16_000_FRDM-MCXA156

## 功能模块

### ADC采样

- 同时使用两个ADC控制器(ADC0和ADC1)进行同步采样
  - ADC0: 通道0、1、4
  - ADC1: 通道4、0、1
  - 对应引脚: P2_0至P2_5
- 支持硬件平均采样，可通过`app_adc.h`中的`ADC_HW_AVERAGE_MODE`宏定义调整
- 支持链式命令配置，实现多通道顺序采样
- 提供快速转换模式，仅采样特定通道以提高性能
- 支持硬件触发，可通过ARM TXEV事件同步触发

### 多摩川协议CRC校验

实现了多摩川协议使用的CRC-8算法(多项式: x^8 + x^2 + x + 1)，包括：

- 基于查表的软件实现
- 硬件加速实现
- 性能对比和验证功能
- 支持测试向量验证

### ATAN2优化计算

ATAN2优化实现使用多项式近似法，相比标准库函数提供了显著的性能提升：

- 实现快速ATAN2近似计算，使用六阶多项式
- 与标准库函数精度对比
- 性能基准测试
- 支持四象限角度计算

### 高速串口通信

- 支持2.5Mbps波特率配置
- 外部20MHz晶振作为时钟输入，可以完美分频出2.5Mbps波特率(无计算误差)
- 使用LPUART1，引脚P3_21(TX)和P3_20(RX)

### EQDC正交解码

- 支持正交编码器信号解码
- 输入引脚:
  - P3_31: TRG_IN10(ENA)
  - P4_6: TRG IN4(ENI)
  - P2_17: TRG IN9(ENB)

## 硬件要求

- NXP MCXA156微控制器开发板
- 外部时钟源(20MHz)，**需要硬件改动**
- 串口连接(用于调试和命令交互)
- 支持ADC输入的模拟信号源(用于ADC测试)

## 软件架构

该演示程序采用模块化设计，主要组件包括：

- `main.c`: 主程序入口，初始化各个模块并处理用户命令
- `app_adc.c/h`: ADC配置和采样功能
- `app_atan.c/h`: ATAN2优化实现和基准测试
- `app_crc.c/h`: 多摩川协议CRC-8实现和测试
- `app_lpuart.c/h`: 高速UART通信
- `app_timer.c/h`: 高精度定时器
- `app_eqdc.c/h`: EQDC正交解码器配置和使用
- `pin_mux.c/h`: 引脚复用配置

## 使用方法

1. 编译并下载程序到MCXA156开发板
2. 通过串口连接(115200波特率)与演示程序交互
3. 使用以下命令进行功能测试：
   - `a` - ADC演示
   - `f` - 快速ADC演示
   - `u` - UART演示
   - `t` - 多摩川CRC演示
   - `n` - ATAN2性能测试

```
ADC initialization complete
ADC0 channels: 0, 1, 4
ADC1 channels: 4, 0, 1
Hardware average: 1 sample
ADC resolution: 16-bit
Fast conversion configured for channels 0 and 4
CoreClock: 96000000 Hz
MCXA156 Demo Application
Core System Clocks:
  Core Clock:     96000000 Hz ( 96 MHz)
  Bus Clock:      96000000 Hz ( 96 MHz)
========================
Available commands:
  a - ADC Demo
  f - Fast ADC Demo
  u - UART Demo
  t - Tamagawa CRC Demo
  n - ATAN2 Benchmark
  e - EQDC Encoder Demo
> 
```

ADC测试(6通道)

```
--- ADC DEMO ---

ADC0 Results:
Channel 0 - Value: 1474, CmdID: 1
Channel 1 - Value: 499, CmdID: 2
Channel 4 - Value: 6959, CmdID: 3

ADC1 Results:
Channel 4 - Value: 2286, CmdID: 2
Channel 0 - Value: 1667, CmdID: 3
Channel 1 - Value: 3202, CmdID: 4


ADC Conversion Time: 8.7 us
```

快速ADC测试(2通道)

```
--- FAST ADC DEMO ---
ADC0 Channel 0 Value: 2112
ADC1 Channel 4 Value: 5078

Fast ADC Conversion Time: 2.6 us
```

硬件CRC:

```
--- TAMAGAWA CRC DEMO ---

--- Tamagawa Protocol CRC-8 Test ---
Testing command: 0x01 0x05 0x12 0x34 0x56 0x78 0x9A 0xBC 0x00 0x00 0x00 0x00 0x00 0x00 0x00 0x00 

CRC-8 Results:
-------------
1. Table-based SW: 0x08 (6.42 us)
2. Hardware CRC:   0x08 (4.32 us)

Verification:
------------
- Implementations match

Performance:
-----------
- HW vs Table: 1.48x (HW is faster)

Performance test with 1000 iterations:
------------------------------------
1. Table-based SW (1000x): 4700.10 us
2. Hardware CRC (1000x):   1992.17 us

Speed comparison (1000 iterations):
- HW vs Table: 2.36x (HW is faster)
```

软件ATAN优化:

```
--- ATAN2 BENCHMARK ---
ATAN2 Benchmark - Standard vs SVML-Style Optimized
---------------------------------------
| Point |    X    |    Y    | Standard ATAN2 | Fast ATAN2 | Error (rad) | Error (deg) |
|-------|---------|---------|----------------|------------|-------------|-------------|
|     0 |   1.000 |   1.000 |       0.785398 |   0.785396 |  0.00000173 |    0.000099 |
|     1 |  -1.000 |   1.000 |       2.356194 |   2.356196 |  0.00000191 |    0.000109 |
|     2 |   0.000 |   1.000 |       1.570796 |   1.570796 |  0.00000000 |    0.000000 |
|     3 |   0.000 |  -1.000 |      -1.570796 |  -1.570796 |  0.00000000 |    0.000000 |
|     4 |   1.000 |   0.000 |       0.000000 |   0.000000 |  0.00000000 |    0.000000 |
|     5 |  -1.000 |   0.000 |       3.141593 |   3.141593 |  0.00000000 |    0.000000 |
|     6 |   1.000 |  -1.000 |      -0.785398 |  -0.785396 |  0.00000173 |    0.000099 |
|     7 |  -1.000 |  -1.000 |      -2.356194 |  -2.356196 |  0.00000191 |    0.000109 |
|     8 |   0.500 |   0.866 |       1.047185 |   1.047186 |  0.00000155 |    0.000089 |
|     9 | 100.000 |   0.100 |       0.001000 |   0.001000 |  0.00000002 |    0.000001 |
---------------------------------------
Average Error: 0.00000088 rad (0.000051 deg)
Maximum Error: 0.00000191 rad (0.000109 deg)

Single Call Timing Measurement:
Standard atan2 single call: 5.885 us
Fast atan2 single call:     0.438 us
Single call speed ratio:    13.5x

Performance Benchmark (1000 iterations):
Standard atan2: 13227.3 us
Fast atan2:     9451.8 us
Speed improvement: 1.4x
```

