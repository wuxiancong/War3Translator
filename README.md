# War3Translator 

**War3Translator** 是一款专为魔兽争霸 III（Warcraft III）对战玩家设计的实时聊天翻译插件。它通过底层内存钩子（Hook）拦截游戏网络同步包，并集成云端翻译 API，实现跨国对战时的无障碍交流。

[![Platform](https://img.shields.io/badge/Platform-Win32%20%7C%20Win64-blue.svg)](https://github.com/your-repo)
[![Framework](https://img.shields.io/badge/Framework-Qt%205.15-green.svg)](https://www.qt.io/)
[![License](https://img.shields.io/badge/License-MIT-orange.svg)](LICENSE)

---

## 核心功能

- **实时拦截**: 深度 Hook `NetEventChatFromHost` 事件，在聊天内容显示前完成拦截与解析。
- **多语种支持**: 支持英文、俄语、德语、韩语等多种主流语言自动识别并翻译为本地语言。
- **双引擎驱动**: 内置 **Google Translate**（首选）与 **百度翻译**（备选）双引擎，确保高可用性。
- **智能缓存**: 
  - **三端同步存储**: 自动将翻译结果持久化到程序根目录、`AppData` 及系统注册表，减少重复 API 请求，节省流量。
  - **秒级响应**: 命中缓存的消息 0 延时显示。
- **自动化运维**: 自动检测 API 频率限制（QPS 控制），自动维护缓存容量防止占用过载。

---

## 技术原理

1. **汇编级注入**: 使用 `Naked` 函数通过内联汇编精确操控 ESP/EBP 栈指针，拦截原始同步数据包。
2. **内存重定向**: 在内存中动态重写 `ppPayload`（数据包指针）和 `pDataSize`（长度），实现无感知内容替换。
3. **异步处理流**: 采用 Qt 事件循环机制，翻译请求不阻塞游戏主线程，确保游戏过程不卡顿。

---

## 安装要求

### 1. 基础环境
- **Warcraft III**: 支持 1.20 ~ 1.27 (x86) 以及重制版 (x64) 进程。
- **OpenSSL 运行库**: 由于翻译 API 使用 HTTPS 协议，必须在游戏根目录放置 OpenSSL 依赖：
  - `libcrypto-1_1.dll`
  - `libssl-1_1.dll`
  > *注：请确保 DLL 位数（32位/64位）与你的游戏客户端一致。*

### 2. 注入插件
将编译生成的 `War3Translator.dll` 通过注入器（如 War3Launcher）注入到魔兽争霸进程中。

---

## 配置说明

在 `i18n_cache.json` 或设置面板中，你可以配置：

- **百度翻译 API**: 填入你的 `AppID` 和 `SecretKey` 以启用备选引擎。
- **目标语种**: 默认为 `zh_CN`（简体中文）。
- **翻译阈值**: 自动忽略短指令（如 `-clear`, `-stats`）或表情符号。

---

---

## 免责声明

1. **风险提示**: 本项目涉及内存操作。虽然主要用于交流翻译，但在某些严格的对战平台（如高强度反作弊环境）使用可能会有封号风险，请自行承担责任。
2. **API 费用**: 使用百度翻译高级版或大量调用 Google API 可能会产生费用或 IP 封禁，建议合理配置缓存策略。

---

## 贡献

欢迎提交 Pull Request 或 Issue。如果你在使用中遇到 `TLS initialization failed` 或 `Stack Offset Error`，请参考 [Wiki](https://github.com/your-repo/wiki) 中的诊断步骤。

---

**War3Translator** - 让竞技不再有语言隔阂。 