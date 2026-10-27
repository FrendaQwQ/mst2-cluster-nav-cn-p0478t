# 🧭 mst2-cluster-nav — CN P0478T / VW AID 791D Port

### 让 Apple CarPlay 导航地图出现在大众原厂液晶仪表中

### Bring Apple CarPlay Navigation to Your Volkswagen Digital Cockpit

基于 [perkthefox/mst2-cluster-nav](https://github.com/perkthefox/mst2-cluster-nav) 开源项目，为中国版大众 MST2 P0478T 固件及 3G0 920 791 D 液晶仪表提供的适配版本。

A firmware-specific adaptation of [perkthefox/mst2-cluster-nav](https://github.com/perkthefox/mst2-cluster-nav), bringing native Apple CarPlay AltScreen navigation to Volkswagen MST2 CN P0478T head units and the 3G0 920 791 D Active Info Display.

> **兼容性限制 / Compatibility**
>
> **本项目仅适用于 TechniSat/Preh 制造的 MST2 车机，不支持 Delphi 制造的 MST2 车机。**
>
> **This project supports TechniSat/Preh MST2 head units only. Delphi MST2 units are NOT supported.**
>
> 请勿仅凭车机属于 MIB2/MST2 系列就判断兼容。安装前必须核对制造商、固件及硬件版本。
>
> Do not assume compatibility based solely on the MIB2/MST2 designation. Verify the manufacturer, firmware and hardware revision before installation.

**简体中文 | [English](#english)**

> **原始项目 / Original Project:** [perkthefox/mst2-cluster-nav](https://github.com/perkthefox/mst2-cluster-nav)  
> **原作者 / Original Author:** Anton Perk ([@perkthefox](https://github.com/perkthefox))  
> **固件适配及实车验证 / Firmware Port & Vehicle Testing:** [@FrendaQwQ](https://github.com/FrendaQwQ)  
> **开发辅助 / Development Assistance:** OpenAI Codex  
> **上游基线 / Upstream Baseline:** v1.0.0  
> **许可证 / License:** CC BY-NC-SA 4.0

---

# 简体中文

## ✨ 项目介绍

首先，衷心感谢 **Anton Perk** 开放了 [mst2-cluster-nav](https://github.com/perkthefox/mst2-cluster-nav) 的源代码与技术研究成果。

原项目实现了一项非常有价值的功能：让原本不支持 CarPlay 辅助屏幕的 MST2 车机，能够将 iPhone 原生生成的导航地图显示到车辆液晶仪表，并提供原厂风格的转向引导。

然而，原项目主要针对欧洲版斯柯达 P0480T 固件和相应液晶仪表开发，无法直接用于中国版大众 P0478T 固件。

**本 Fork 的目的，就是让这项优秀的开源成果能够在另一套 MST2 固件与仪表组合上运行。**

本项目没有重新设计原作者的核心架构，而是在尽可能保留上游实现的基础上，完成特定固件及硬件的兼容性适配。

## 🚗 已测试配置

| 项目 | 配置 |
|---|---|
| 测试车辆 | Volkswagen Passat B8 Variant |
| 车机制造商 | TechniSat/Preh |
| 车机型号 | `3Q0 035 866 C` |
| 硬件版本 | H41 |
| 固件版本 | `MST2_CN_VW_ZR_P0478T` |
| HMI 版本 | `H29.319.93` |
| 液晶仪表 | `3G0 920 791 D` |

**目前仅在上述车辆及硬件组合上完成初步实车验证。其他固件、车机和仪表型号尚未验证。**

**特别注意：本 Fork 不支持 Delphi MST2 车机，也不应直接用于其他 TechniSat/Preh 固件版本。**

## 🗺️ 实现功能

### CarPlay 原生仪表地图

iPhone 为液晶仪表独立生成 CarPlay 导航画面，通过 MST2 原有的 MOST 视频通路显示在仪表导航区域。

这是真正的 CarPlay AltScreen 功能，而不是中控屏幕镜像，也不是由车机重新绘制的第三方地图。

### 原厂风格转向引导

通过上游项目实现的 iAP2、HMI 和 BAP 集成，将 CarPlay 导航指令同步至仪表。

转向箭头、导航距离等信息由原厂仪表界面显示，尽可能保留大众原有的使用体验。

### CarPlay 断开后恢复原厂显示

在测试车辆上，CarPlay 断开连接后，仪表能够正常恢复原有显示。

整个方案继续使用原厂显示及 MOST 传输系统，不需要更换车机或液晶仪表硬件。

## 🔧 本 Fork 做了什么？

原始项目主要针对 `MST2_EU_SK_ZR_P0480T` 固件及 Continental `5E0 920 790 A` 液晶仪表开发。

本 Fork 主要完成以下适配：

**1. DisplayManager 固件适配**

针对 `MST2_CN_VW_ZR_P0478T` 的原厂二进制文件，重新定位并调整相关 ARM 调用接口，使上游的视频处理机制能够适用于目标固件。

**2. HMI 兼容性适配**

适配中国版 P0478T 固件所使用的 `H29.319.93` HMI 版本，保留原项目的 HMI 集成及导航功能。

**3. 791D 液晶仪表适配**

参考 [OneB1t/VcMOSTRenderMqb](https://github.com/OneB1t/VcMOSTRenderMqb) 项目中关于大众 MQB 液晶仪表及 MOST 视频显示的公开研究，对 `3G0 920 791 D` 的辅助屏幕显示配置进行适配。

特别感谢相关开发者对大众液晶仪表技术研究的公开分享。

**本 Fork 保留了上游的主要技术实现，包括 CarPlay 辅助视频接收、DisplayManager 视频处理、MOST 传输、iAP2 导航信息处理及 Toolbox 集成。**

## ✅ 实车测试

在 Volkswagen Passat B8 Variant 上，已完成以下初步验证：

| 功能 | 测试结果 |
|---|---|
| CarPlay 连接 | ✅ 正常 |
| 仪表显示 CarPlay 地图 | ✅ 正常 |
| 原厂风格转向引导 | ✅ 正常 |
| CarPlay 断开后恢复 | ✅ 正常 |
| 长期运行稳定性 | 尚在验证 |
| 其他车型及固件 | 未验证 |

以上结果仅代表测试车辆的实际表现，不构成对其他硬件组合的兼容性保证。

## 📦 安装说明

本 Fork 沿用原作者基于 **MIB STD2 Toolbox** 的安装方式。

特别感谢 [olli991/mib-std2-pq-zr-toolbox](https://github.com/olli991/mib-std2-pq-zr-toolbox) 项目及其贡献者，为 TechniSat/Preh MST2 平台提供开放的工程菜单扩展与工具支持。

### 安装前提

**1. 确认车机硬件平台**

本项目仅支持 **TechniSat/Preh MST2**，不支持 **Delphi MST2**。

请先确认车机制造商及硬件型号，不要将不同厂商的 MST2 固件或补丁混用。

**2. 确保已启用开发者模式**

安装前须确保车机 **5F 信息娱乐系统已启用开发者模式，并能够正常进入 Green Engineering Menu（GEM）**。

开发者模式的启用方法请使用者自行查阅相关资料并完成配置，本项目不提供此项操作教程。

**3. 确认固件与本 Fork 完全一致**

本 Fork 针对以下固件进行适配：

- Firmware：`MST2_CN_VW_ZR_P0478T`
- HMI：`H29.319.93`

请在安装前核对车机实际运行的固件版本。

**本项目不提供、不分发任何大众原厂固件，也不提供固件升级服务。**

使用者需要自行确保车机已经运行与本 Fork 匹配的固件。不要通过修改版本检测、强制安装或刷入其他地区固件来绕过兼容性要求。

**4. 安装 MIB STD2 Toolbox**

本项目依赖以下独立开源项目：

**[olli991/mib-std2-pq-zr-toolbox](https://github.com/olli991/mib-std2-pq-zr-toolbox)**

请按照该项目的官方说明完成 Toolbox 安装，确认能够正常进入 Toolbox 菜单。

Toolbox 由其原项目独立开发和维护，并非本 Fork 的原创组件。

**5. 完成必要备份**

建议提前保存车机当前配置及相关恢复数据，并确认备份有效。

修改车机启动文件存在风险，不能仅凭安装程序显示成功就认定备份完整。

### 安装步骤

完成上述前置检查后，按照原项目的 Toolbox 流程操作：

1. 下载本 Fork 对应的 **CN P0478T 专用安装包**。
2. 将安装包内 `sd/custom/` 中的内容复制到 Toolbox SD 卡的 `custom/` 目录。
3. 进入 **Green Engineering Menu → MIB STD2 Toolbox → Customization → GreenMenu**。
4. 选择 **Copy custom Green Engineering Menu screens and scripts to unit**，导入自定义菜单。
5. 重新进入 Toolbox，在 **Customization → Cluster_Navigation** 中找到本项目菜单。
6. 根据原项目流程，执行安装并进行单次 Trial 测试。
7. 在测试车辆处于安全停放状态时，检查 CarPlay 地图、导航引导及断连恢复。
8. 确认相关功能符合预期、检查日志与恢复条件后，再决定是否永久启用。

**请勿直接使用上游 P0480T 的二进制文件安装到 P0478T 设备，也不要将本 Fork 安装到 Delphi MST2。**

如需详细的安装、卸载及诊断方法，请参考原项目 README 和本 Fork 的安装文档。

### ⚠️ 风险提示

本项目属于非官方实验性适配，**目前仅在上述测试车辆上验证了相关功能可以正常运行**。

不同车机、固件、硬件版本和车辆配置可能产生不同结果。

修改车机系统存在软件异常、功能失效或需要专业设备恢复的风险。

**请自行评估并承担安装、修改及使用过程中的相关风险。项目作者及维护者不对设备损坏或其他损失作出保证。**

## 🙏 特别感谢

### 感谢原作者 Anton Perk

**特别感谢 [Anton Perk (@perkthefox)](https://github.com/perkthefox) 对 mst2-cluster-nav 的开发，以及愿意将研究成果和源代码公开分享给整个社区。**

CarPlay AltScreen 协商、H.264 视频处理、原厂 MOST 通路复用、iAP2 导航数据处理、HMI/BAP 集成与 Toolbox 安装机制，均建立在原作者的研究与开发之上。

**没有原作者的开放成果，就不可能有这个 Fork。**

本项目只是站在前人的工作之上，尝试为另一套固件和仪表组合提供支持。

同时感谢：

- [OneB1t/VcMOSTRenderMqb](https://github.com/OneB1t/VcMOSTRenderMqb) 所公开的液晶仪表及 MOST 视频研究。
- [olli991/mib-std2-pq-zr-toolbox](https://github.com/olli991/mib-std2-pq-zr-toolbox) 提供的 MST2 工具与工程菜单扩展能力。
- 原项目所致谢的所有开发者与研究者。
- 长期分享大众 MIB2、CarPlay 和 MQB 平台技术成果的社区成员。
- OpenAI Codex 在本次固件分析、代码适配和构建验证中的协助。

### 关于开源

我始终认为，开源的价值不仅在于让一项技术可以被使用，更在于让它能够被理解、验证、改进，并继续帮助其他人。

**当一项功能已经建立在社区公开研究成果之上，最值得增加的应该是新的代码与贡献，而不只是一个更高的报价。**

希望这个 Fork 能够延续上游的分享精神，让更多人受益于开放、透明和可持续的技术协作。

如果本项目对你有所帮助，请优先访问原作者仓库并为其点亮 Star：

⭐ **[perkthefox/mst2-cluster-nav](https://github.com/perkthefox/mst2-cluster-nav)**

## 📄 许可证与免责声明

本 Fork 继承原项目的 **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International（CC BY-NC-SA 4.0）** 许可证，并保留原作者署名、LICENSE、NOTICE 和 Git 提交历史。

本项目仅用于非商业性的技术研究与交流。

本项目与 Volkswagen、Apple 及相关汽车电子供应商不存在官方关联，也不提供原厂专有固件或授权文件。

---

# English

## ✨ About This Fork

First of all, a sincere thank-you to **Anton Perk** for making the source code and technical research behind [mst2-cluster-nav](https://github.com/perkthefox/mst2-cluster-nav) available to the community.

The original project achieves something remarkable: bringing native Apple CarPlay navigation maps and factory-style turn-by-turn guidance to MST2 instrument clusters that were never designed to support CarPlay's auxiliary display functionality.

However, the original implementation primarily targets European Škoda P0480T firmware and its corresponding instrument cluster.

**This fork aims to bring that excellent open-source work to a different MST2 firmware and instrument cluster combination.**

Rather than redesigning the original implementation, this fork focuses on firmware-specific and hardware-specific compatibility while preserving the upstream architecture as much as possible.

## 🚗 Tested Configuration

| Component | Configuration |
|---|---|
| Test Vehicle | Volkswagen Passat B8 Variant |
| Head Unit Manufacturer | TechniSat/Preh |
| Head Unit | `3Q0 035 866 C` |
| Hardware Revision | H41 |
| Firmware | `MST2_CN_VW_ZR_P0478T` |
| HMI Version | `H29.319.93` |
| Instrument Cluster | `3G0 920 791 D` |

**Initial vehicle testing has been completed only on the configuration listed above. Other firmware versions, head units and instrument clusters have not been verified.**

**Important: Delphi MST2 head units are not supported. This fork must not be assumed compatible with other TechniSat/Preh firmware versions.**

## 🗺️ Features

### Native CarPlay Maps in the Instrument Cluster

The iPhone renders a dedicated CarPlay navigation view for the instrument cluster.

The auxiliary video is delivered through the MST2 system's existing MOST video path.

This is genuine CarPlay AltScreen functionality, not center-screen mirroring or a separately rendered third-party navigation interface.

### Factory-Style Turn-by-Turn Guidance

The upstream project's iAP2, HMI and BAP integration enables CarPlay navigation instructions to appear through the instrument cluster's original navigation interface.

Turn arrows, navigation distances and related guidance retain the factory Volkswagen visual experience.

### Factory Display Recovery

On the tested vehicle, the original cluster display successfully returns when CarPlay disconnects.

The implementation continues to use the factory display and MOST transport system, without replacing the head unit or instrument cluster hardware.

## 🔧 What Does This Fork Change?

The original project primarily targets `MST2_EU_SK_ZR_P0480T` firmware and the Continental `5E0 920 790 A` instrument cluster.

This fork introduces the following platform-specific adaptations:

**1. DisplayManager Firmware Compatibility**

Firmware-specific ARM call interfaces were identified and adjusted for `MST2_CN_VW_ZR_P0478T`, allowing the upstream video processing mechanism to work with the target firmware.

**2. HMI Compatibility**

Support was adapted for the `H29.319.93` HMI version used by the target CN P0478T firmware, while preserving the original HMI integration and navigation features.

**3. VW AID 791D Instrument Cluster Compatibility**

The auxiliary display configuration was adapted for the `3G0 920 791 D`, with reference to the publicly available MQB instrument-cluster and MOST video research in [OneB1t/VcMOSTRenderMqb](https://github.com/OneB1t/VcMOSTRenderMqb).

Special thanks to the researchers who shared their findings on Volkswagen instrument-cluster technology.

**The core upstream implementation remains intact**, including CarPlay auxiliary video reception, DisplayManager processing, MOST transport, iAP2 navigation handling and Toolbox integration.

## ✅ Vehicle Testing

Initial testing on a Volkswagen Passat B8 Variant produced the following results:

| Feature | Result |
|---|---|
| CarPlay connection | ✅ Working |
| CarPlay map in instrument cluster | ✅ Working |
| Factory-style turn guidance | ✅ Working |
| Factory display recovery after disconnect | ✅ Working |
| Long-term stability | Still under evaluation |
| Other vehicles and firmware versions | Not verified |

These results describe the tested vehicle only and do not guarantee compatibility with other hardware configurations.

## 📦 Installation

This fork retains the original installation workflow using **MIB STD2 Toolbox**.

Special thanks to the independent [olli991/mib-std2-pq-zr-toolbox](https://github.com/olli991/mib-std2-pq-zr-toolbox) project and its contributors for providing engineering-menu customization and tooling for TechniSat/Preh MST2 systems.

### Prerequisites

**1. Verify the Head Unit Platform**

This project supports **TechniSat/Preh MST2** head units only.

**Delphi MST2 units are NOT supported.**

Verify the head unit manufacturer and hardware model before proceeding. Do not mix firmware or patches intended for different MST2 manufacturers.

**2. Enable Developer Mode**

Before installation, make sure that **Developer Mode is enabled in the infotainment control module (5F)** and that the **Green Engineering Menu (GEM)** is accessible.

Users are responsible for enabling Developer Mode on their own. This repository does not provide instructions for this procedure.

**3. Verify the Exact Firmware Version**

This fork is specifically adapted for:

- Firmware: `MST2_CN_VW_ZR_P0478T`
- HMI: `H29.319.93`

Confirm the firmware currently installed on your head unit before proceeding.

**This repository does not provide or distribute Volkswagen factory firmware and does not offer firmware upgrade services.**

Users are responsible for ensuring that their head unit is already running the firmware supported by this fork.

Do not bypass compatibility checks through forced installation, modified version identifiers or cross-region firmware flashing.

**4. Install MIB STD2 Toolbox**

This fork depends on the independent project:

**[olli991/mib-std2-pq-zr-toolbox](https://github.com/olli991/mib-std2-pq-zr-toolbox)**

Follow the Toolbox project's official installation instructions and confirm that its menu is accessible and functioning correctly.

Toolbox is developed and maintained independently. It is not an original component of this fork.

**5. Prepare and Verify Backups**

Back up the relevant existing configuration and recovery data before modifying the system.

Ensure the backups are complete and usable. An installation success message alone does not prove backup integrity.

### Installation Steps

After completing the prerequisites, follow the upstream Toolbox workflow:

1. Download the **CN P0478T-specific installation package** for this fork.
2. Copy the contents of `sd/custom/` from the package into the `custom/` directory on your Toolbox SD card.
3. Open **Green Engineering Menu → MIB STD2 Toolbox → Customization → GreenMenu**.
4. Select **Copy custom Green Engineering Menu screens and scripts to unit**.
5. Reopen Toolbox and navigate to **Customization → Cluster_Navigation**.
6. Follow the original installation procedure and perform the one-time Trial Boot.
7. While the vehicle is safely parked, verify CarPlay maps, navigation guidance and recovery after disconnection.
8. Review the results, diagnostic logs and recovery conditions before deciding whether to enable permanent operation.

**Do not install upstream P0480T binaries on a P0478T head unit. Do not install this fork on Delphi MST2 hardware.**

For detailed installation, removal and diagnostic procedures, refer to the upstream README and the installation guide provided with this fork.

### ⚠️ Disclaimer

This is an unofficial, experimental firmware adaptation.

**The described functionality has only been confirmed on the specific test vehicle listed above.**

Different firmware versions, hardware revisions and vehicle configurations may behave differently.

Modifying head-unit software can result in system instability, unavailable functions or recovery procedures requiring specialized equipment.

**You are responsible for evaluating and accepting the risks associated with installation, modification and use. No guarantee against device damage or other losses is provided.**

## 🙏 Special Thanks

### Thanks to Anton Perk

**A very special thank-you to [Anton Perk (@perkthefox)](https://github.com/perkthefox) for developing mst2-cluster-nav and generously sharing the source code and research with the community.**

CarPlay AltScreen negotiation, H.264 video handling, factory MOST transport integration, iAP2 navigation processing, HMI/BAP integration and the Toolbox installation mechanism are all built upon the upstream author's work.

**Without the original author's openness and technical contributions, this fork would not exist.**

This adaptation simply builds upon that foundation to support another firmware and instrument cluster combination.

Additional thanks to:

- [OneB1t/VcMOSTRenderMqb](https://github.com/OneB1t/VcMOSTRenderMqb) for publicly sharing research on instrument clusters and MOST video.
- [olli991/mib-std2-pq-zr-toolbox](https://github.com/olli991/mib-std2-pq-zr-toolbox) for MST2 engineering tools and customization support.
- All developers and researchers acknowledged by the upstream project.
- The wider community sharing knowledge about Volkswagen MIB2, CarPlay and MQB systems.
- OpenAI Codex for assisting with firmware analysis, platform adaptation and build validation.

### A Note on Open Source

I believe the real value of open source is not merely making technology accessible, but allowing it to be understood, verified, improved and shared with others.

**When something is already built upon publicly shared community research, the most meaningful value to add should be new code and contributions — not simply a higher price tag.**

I hope this fork continues the spirit of the upstream project and helps make these capabilities more accessible through open, transparent and sustainable collaboration.

If you find this project useful, please visit and star the original repository first:

⭐ **[perkthefox/mst2-cluster-nav](https://github.com/perkthefox/mst2-cluster-nav)**

## 📄 License and Disclaimer

This fork retains the original project's **Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)** license, original attribution, LICENSE, NOTICE and Git history.

This project is intended for non-commercial research and community collaboration.

It is not affiliated with or endorsed by Volkswagen, Apple or any related automotive electronics supplier, and does not distribute proprietary manufacturer firmware or activation files.

---

**Original Project & Core Development:** Anton Perk ([@perkthefox](https://github.com/perkthefox))  
**CN P0478T Firmware Adaptation & Vehicle Validation:** [@FrendaQwQ](https://github.com/FrendaQwQ)  
**Development Assistance:** OpenAI Codex

*Built on open research. Improved through collaboration. Shared with the community.*