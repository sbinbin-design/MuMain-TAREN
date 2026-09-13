# MuMain

## 🎮 一个80后的游戏爱好者

这个项目我想在 `MU_S6_Full_Client_20190930` 基础上优化成中文客户端。如果有相同情怀的朋友或爱好者感兴趣，可以加我微信一起交流（备注：MU）：**alexbin**。

MuMain 是一个面向 **MU Online Season 6 Episode 3** 的跨平台客户端项目，
基于 [Luois 发布的 Season 5.2 客户端源码](https://github.com/LouisEmulator/Main5.2)
持续演进。项目目标是保留原版游戏行为和资源兼容性，同时逐步完善渲染、网络、
界面、本地化和开发工具。

项目可以连接并运行于 [OpenMU](https://github.com/MUnique/OpenMU)。客户端网络层
使用仓库内的 `ClientLibrary/`，由 .NET Native AOT 编译为平台原生共享库。

## 当前状态

目前已经具备可游玩的 Season 6 Episode 3 客户端基础，主要能力包括：

- 使用 SDL GPU 渲染：Windows 使用 D3D12，Linux 使用 Vulkan，macOS 使用 Metal。
- 延迟提交 GPU 命令、索引四边形与条带、可增长的帧缓冲区，以及相邻绘制合并。
- 支持 VSync、帧率限制、帧率计数器、详细性能面板和 SDL GPU 统计面板。
- 支持 Windows x86/x64、Linux x64 和 macOS arm64 原生构建。
- 支持 Season 6 的大师技能树、Rage Fighter、MU Helper、自动重连、仓库和背包扩展。
- 网络协议已适配 Season 6 Episode 3，并扩展了伤害、经验、物品和外观等数据的序列化。
- 使用 UTF-16LE 处理内存中的文本，文件和网络文本使用 UTF-8。
- 支持运行时切换界面语言；翻译资源位于 `src/Localization/`，详见
  [翻译系统](docs/translation-system.md)。
- 集成可选的 MuEditor（基于 Dear ImGui），用于运行时调试和参数调整。
- 资源、着色器、配置文件和网络库会在构建后自动复制到可运行目录。

仍在完善的内容包括 Lucky Items，以及 Season 6 Episode 3 兼容性和跨平台表现的
持续校正。具体变化请查看 [CHANGELOG.md](CHANGELOG.md)。

## 支持矩阵

| 平台 | 架构 | MuEditor | 网络库 | 状态 |
| --- | --- | --- | --- | --- |
| Windows | x86 | 支持 | `MUnique.Client.Library.dll` | 完整客户端 |
| Windows | x64 | 支持 | `MUnique.Client.Library.dll` | 完整客户端 |
| Linux | x64 | 支持 | `MUnique.Client.Library.so` | 完整客户端 |
| macOS | arm64 | 当前预设关闭 | `MUnique.Client.Library.dylib` | 完整客户端 |
| Linux | x86 | 不支持 | 无可用 .NET AOT 网络库 | 不支持 |

Android 和 iOS 目录目前只是后续移植的占位内容，暂时不能构建完整客户端。

## 开发环境

通用要求：

- CMake 3.25 或更高版本
- 支持 C++20 的编译器
- .NET SDK 10.0 或更高版本，用于构建 `ClientLibrary/`
- Ninja；Windows 预设使用 Ninja Multi-Config
- Git 子模块

Windows 原生构建还需要 Visual Studio 2022（安装 C++ 工作负载）和 vcpkg。
Visual Studio、CLion、Rider、VS Code 的环境配置请参考
[构建指南](docs/build/README.md)及其中的平台专用说明。

首次获取代码后，在仓库根目录初始化子模块：

```bash
git submodule update --init --recursive
```

主要子模块位于 `src/ThirdParty/`：

- `SDL`：窗口、输入和音频基础设施
- `SDL_mixer`：音频混音
- `imgui`：MuEditor 使用的调试界面；只有启用 MuEditor 时才需要

## 使用 CMake 构建

项目提供了统一的 `CMakePresets.json`。Windows 提供标准构建和 MuEditor 构建，
Linux 可通过配置选项启用 MuEditor；各平台均提供 Debug、Release 两种配置。
当前 macOS 预设为 MuEditor 关闭的标准构建。

### Windows

在 Visual Studio Developer PowerShell 或已加载 MSVC 环境的终端中执行：

```powershell
# x64 标准构建
cmake --preset windows-x64
cmake --build --preset windows-x64-release

# x86 标准构建
cmake --preset windows-x86
cmake --build --preset windows-x86-release

# x64 MuEditor 构建
cmake --preset windows-x64-mueditor
cmake --build --preset windows-x64-mueditor-debug
```

将 `release` 改为 `debug` 即可构建 Debug 版本。MuEditor 构建可使用 `--editor`
启动，并在游戏中按 **F12** 切换编辑器。

### Linux

Linux 使用原生 x64 构建：

```bash
cmake --preset linux-x64
cmake --build --preset linux-x64-release
```

### macOS

macOS 使用 Apple Silicon arm64 构建：

```bash
cmake --preset macos-arm64
cmake --build --preset macos-arm64-release
```

完整的平台依赖、IDE 配置、WSL/MinGW 交叉编译和常见问题请查看
[docs/build/README.md](docs/build/README.md)。

### 常用 CMake 选项

| 选项 | 默认值 | 说明 |
| --- | --- | --- |
| `ENABLE_EDITOR` | `OFF` | 构建 MuEditor；启用后定义 `_EDITOR` |
| `BUILD_TESTING` | `OFF`（Linux/macOS 预设为 `ON`） | 构建测试并注册 CTest |
| `MU_COPY_RUNTIME_ASSETS` | `ON` | 将 `Data/`、`fonts/` 和配置模板复制到输出目录 |
| `MU_ENABLE_SHADER_COMPILATION` | `OFF` | 使用 glslang、SPIRV-Cross、DXC 重新编译着色器 |

默认构建使用仓库中已提交的着色器二进制文件。只有在需要重新生成着色器时，
才应启用 `MU_ENABLE_SHADER_COMPILATION=ON`，并安装对应工具链。

## 运行客户端

构建完成后，请从包含 `Main`（或 `Main.exe`）、网络库、`Data/` 和 `fonts/` 的
输出目录启动客户端。CMake 的后置步骤会自动准备该目录。

连接服务器的基本格式为：

```text
Main.exe connect /u<服务器地址> /p<端口>
```

例如：

```text
Main.exe connect /u127.0.0.1 /p44406
```

默认连接地址为 `localhost:44406`。客户端当前使用版本 `2.04d` 和序列号
`k1Pk2jcET48mxL3b`；如果服务端检查版本或序列号，请确保两端配置一致。

也可以使用 [OpenMU Launcher](https://github.com/MUnique/OpenMU/releases) 启动。

### 配置文件

客户端从可执行文件所在目录读取 `config.ini`。首次构建时，配置会从
`src/bin/config.ini.template` 生成。常用配置如下：

| 节 | 键 | 默认值 | 说明 |
| --- | --- | --- | --- |
| `[UI]` | `EnableAnimationTaskPool` | `0` | 可见角色达到阈值时启用动画任务池 |
| `[UI]` | `Locale` | `en` | 当前界面语言；选项窗口会保存运行时修改 |
| `[Camera]` | `Zoom` | `1735` | 轨道相机距离 |
| `[Render]` | `VSync` | `1` | 是否启用 VSync |

渲染始终使用 SDL GPU；旧的 `[Render] CoreProfile` 配置项不会切换到 OpenGL。

### 常用参数和聊天命令

- `--enable-taskpool`：强制启用动画任务池，仍受可见角色数量阈值限制。
- `--editor`：在 MuEditor 构建中启动时启用编辑器。
- `$fps <数值>`：设置帧率上限。
- `$vsync on` / `$vsync off`：开启或关闭 VSync。
- `$fpscounter on` / `$fpscounter off`：显示或隐藏简易帧率计数器。
- `$details on` / `$details off`：显示或隐藏详细性能面板。
- `$glstats on` / `$glstats off`：显示或隐藏 SDL GPU 统计。

更多命令请参考 [聊天命令](docs/chat-commands.md)。

## 测试

测试位于 `tests/`，使用 doctest 和 CTest。配置时启用测试后运行：

```bash
cmake -S . -B out/build/test -G Ninja \
  -DBUILD_TESTING=ON -DENABLE_EDITOR=OFF
cmake --build out/build/test --config Release
ctest --test-dir out/build/test --build-config Release --output-on-failure
```

不同平台的测试要求和模块说明见
[测试参考](docs/build/reference-tests.md)。

## 发布包

CI 会验证以下原生 Release 构建，并生成不包含游戏数据的运行包：

- Windows x64，MuEditor 关闭
- Linux x64，MuEditor 关闭
- macOS arm64，MuEditor 关闭

`Data/` 和 `fonts/` 作为独立的数据发布包提供；每个客户端 Release 都会链接
对应的兼容数据版本。下载发布包后，请按照
[发布包组装说明](docs/build/README.md#hosted-releases)合并客户端和数据文件，
并校验数据包的 SHA-256。

Windows x86、Debug、MuEditor 开启及 MinGW 等配置需要在本地构建。

## 文档

- [构建指南](docs/build/README.md)
- [SDL GPU 与 GPU Skinning](docs/GPU%20Skinning/README.md)
- [相机系统](docs/camera-system.md)
- [选项窗口与配置](docs/options-window.md)
- [MuEditor](docs/dev-editor.md)
- [聊天命令](docs/chat-commands.md)
- [翻译系统](docs/translation-system.md)
- [渲染跨平台一致性](docs/build/rendering-parity.md)
- [变更记录](CHANGELOG.md)

## 参与开发

提交代码前请阅读：

- [AGENTS.md](AGENTS.md)：仓库协作和构建入口说明
- [docs/CODING_RULES.md](docs/CODING_RULES.md)：C++、C# 和通用编码规范

请保持修改范围聚焦，并在提交前根据改动范围执行相应的构建或测试。

## 致谢

- Webzen
- [Louis](https://github.com/LouisEmulator)
- Qubit（tuservermu.com.ve）
- RaGEZONE、tuservermu.com.ve 等社区贡献者
- [Nitoy](https://github.com/nitoygo) 提供的 MU Helper
