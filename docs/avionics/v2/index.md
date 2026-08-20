# V2版本航电

文档AI等级：2

<figure markdown>
  ![照片](assets/picture/picture.jpg){ height="300" }
  ![V2 航电系统实物照片与 PCB 布局](assets/picture/PCB.png){ height="300" }
</figure>

## 设计目标

- 上一代模块化飞控采用所有模块单独焊接到一个独立板子上的构造，这种构造非常浪费板内空间，所以这次版本航电采用最小电子元件集成式的焊接模式。
- 且上一代飞控的软件系统过于简陋，本次飞控软件系统建立全新的火箭标准库RSL，并且引入FreeRtos来进行任务调度，总体复杂度和可开发性上升一个量级。
- 支持实验火箭自主飞行数据采集
- 支持实时LoRa遥测
- 支持飞行状态判断
- 支持后续扩展视觉识别模块
- 提供长期维护的软件架构

## 版本信息

| 项目 | 内容 |
|-|-|
| 硬件版本 | V2.1 |
| 开始设计 | 2026-07 |
| 当前状态 | 软件集成阶段 |
| MCU | STM32F411 |
| RTOS | FreeRTOS |
| PCB状态 | 已完成 |
| 飞行状态 | 未进行实际飞行 |

## 基本介绍

本航电于2026年7月开始设计，基于STM32F411与FreeRTOS的实验火箭飞行控制系统，集成姿态解算、GNSS 定位、LoRa 遥测与飞行数据记录。

## 硬件介绍

| 模块 | 型号 | 用途 |
|-|-|-|
| MCU | STM32F411 | 主控制器 |
| IMU | BMI088 | 六轴惯性测量 |
| GNSS | NEO-M9N | 定位 |
| LoRa | SX1268 | 无线通信 |
| Flash | W25Q128 | 数据存储 |
| 气压计 | BMP388 | 高度测量 |

## 系统功能

### 姿态系统

- BMI088数据采集
- 四元数AHRS解算
- 姿态输出


### 通信系统

- SX1268驱动
- LoRa双向通信
- 遥测发送
- 地面站指令接收


### 数据系统

- Flash存储
- Logger框架
- 自描述数据格式


### 飞行控制

- 状态机
- 飞行阶段识别
- 开伞控制

## 当前进度

项目目前处于软件功能集成与系统联调阶段。

### 已完成

* [x] BMI088 IMU 驱动与数据采集
* [x] 基于四元数的卡尔曼滤波解算与 AHRS 基础功能
* [x] W25Q128 Flash 驱动
* [x] SX1268 LoRa 驱动
* [x] GNSS 驱动
* [x] LoRa 基础数据发送与接收
* [x] Command 指令系统设计与实现
* [x] 飞行数据 Logger 多类型飞行数据记录框架
* [x] FreeRTOS 多任务运行框架
* [x] GNSS 数据读取验证
* [x] 基础遥测数据结构设计
* [x] Communicator 通信模块基础框架

### 进行中

* [ ] LoRa 双向通信流程完善与稳定性测试
* [ ] 遥测数据编码、封包与解析
* [ ] Communicator 与各飞控模块的数据流集成
* [ ] GNSS 在正式飞控 PCB 上的硬件联调
* [ ] AHRS 参数调试
* [ ] 地面站遥测数据解析

### 待实现

* [ ] 火箭飞行状态机
* [ ] 飞行阶段识别与状态转换逻辑
* [ ] 降落伞部署控制
* [ ] 电池电压监测
* [ ] 完整系统异常处理与故障保护
* [ ] 地面站控制与数据可视化
* [ ] 整机地面测试
* [ ] 实际飞行测试

## 硬件信息

- 原理图：[原理图.pdf](assets/hardware/原理图.pdf)
- PCB图：[PCB.pdf](assets/hardware/PCB图.pdf)
- BOM: [BOM](assets/hardware/BOM.xlsx)
- 交互式BOM：[iBOM](assets/hardware/iBOM.html)
- Gerber：[Gerber](assets/hardware/Gerber.zip)
- 3D模型：[3d模型](assets/hardware/3d模型.step)
- 贴片坐标：[坐标文件](assets/hardware/PickAndPlace.xlsx)
- 工程源文件：[源文件](assets/hardware/ProPrj_火箭飞行控制板初版_2026-07-07_12-04-26_2026-08-20.epro2)


## 软件源码

源码位置：[Rocket_FlightControlSystem](https://github.com/SophonSnwflake/Rocket_FlightControlSystem)