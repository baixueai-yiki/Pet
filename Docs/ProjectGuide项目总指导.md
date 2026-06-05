# 项目目标


# 目录结构
Pet/ # 项目文件夹
├─ .vscode/     # 项目配置目录build.ps1
│  ├─ c_cpp_properties.json
│  └─ tasks.json
├─ Build/       # 编译中间文件
│  └─ obj
├─ Docs/        # 项目指导文档
│  ├─ Architecture分层与依赖规则.md
│  ├─ Changelog版本变更记录.md
│  ├─ CodingStyle命名与代码风格.md
│  ├─ DevelopmentGuide开发流程手册.md
│  ├─ ProjectGuide项目总指导.md
│  └─ Roadmap开发计划和里程碑.md
├─ Distribution/# 面向用户的最终可执行文件
│  ├─ alpha/     # 内测版本
│  │  └─ Miss_qing/ # 以角色名命名素材文件夹，以便用户快捷更换自定义桌宠
│  │     ├─ assets/ # 资源
│  │     │  ├─ audio/  # 音频
│  │     │  ├─ chat/   # 文本
│  │     │  └─ images/ # 图像
│  │     ├─ config/ # 配置文件
│  │     ├─ diary.txt  # 桌宠每次结束运行时会写的日记
│  │     └─ Pet.exe    # 可执行exe文件
│  └─ beta/      # 公测版本
Source/
├─ Core/ # 基础库
│  ├─ Path.h/cpp       # 计算和处理文件路径
│  ├── FileSystem.h/cpp  # 在磁盘中读写文件（依赖于Path）
│  ├─── TextFile.h/cpp     # 支持热更新的读写文本功能（依赖于FileSystem）
│  ├─── Logger.h/cpp       # 输出日志文件（依赖于FileSystem）
│  ├─── Config.h/cpp       # 配置管理器（依赖于FileSystem）
│  └─ Timer.h/cpp      # 程序统一的时间工具
├─ Domain/ # 流程库（预设的工作流，可直接调用）
│
├─ Engine/    # 引擎层：提供底层接口，Systems 调用它来实现功能
│   ├─ Window/  # 窗口与渲染上下文管理
│   │   ├─ WindowEvents.h/cpp       # 事件轮询与窗口事件读取
│   │   ├─ WindowCore.h/cpp         # 窗口样式
│   │   └─ WindowLifecycle.h/cpp    # 窗口的创建与销毁
│   ├─ Input/   # 输入处理
│   │   ├─ InputDispatcher.h/cpp    # 捕获鼠标/键盘/触屏事件
│   │   ├─ Keyboard.h/cpp           # 键盘状态查询接口
│   │   ├─ Mouse.h/cpp              # 鼠标状态/位置/点击接口
│   │   └─ TextInputHandler.h/cpp   # 文本输入解析（字符/IME 支持）
│   ├─ Render/  # 绘制与渲染封装
│   │   ├─ Renderer.h/cpp           # 图片/UI绘制接口（位图、贴图、动画）
│   │   ├─ Texture.h/cpp            # 贴图加载、管理、缓存
│   │   └─ Animation.h/cpp          # 基础动画接口（帧动画、插值动画）
│   └─ Audio/   # 音频封装
│       ├─ AudioPlayer.h/cpp        # 播放音效/背景音乐
│       └─ AudioResource.h/cpp      # 音频资源加载/缓存
├─ Runtime/ # 协议层/中枢（无具体业务）
│  ├─ EventBus.h/cpp # 事件发布/订阅中心（模块通信核心）
│  ├─ Scheduler.h/cpp # 定时任务调度（延迟/循环任务）
│  └─ StateManager.h/cpp # 全局状态管理（全局开关/配置）
├─ Systems/ # 系统层（核心逻辑）
│  ├─ Pet/ # 桌宠系统
│  │   ├─ PetActor.h/cpp # 桌宠核心对象（状态机+组件管理）
│  │   └─ PetAComponents/ # 桌宠功能组件
│  │       ├─ AudioComponent.h/cpp # 音频逻辑
│  │       ├─ ChatComponent.h/cpp  # 文本逻辑
│  │       ├─ DiaryComponent.h/cpp # 写入桌宠日志/日记
│  │       ├─ InputComponent.h/cpp # 处理用户输入内容
│  │       └─ PetRenderComponent.h/cpp # 规划桌宠的绘制（位置/贴图/动画）
│  ├─ UI/ # UI系统
│  │   ├─ UIActor.h/cpp # UI核心对象（管理所有UI界面）
│  │   ├─ UIComponents/ # UI行为组件（交互逻辑）
│  │   │   ├─ CloseComponent.h/cpp # 关闭全部页面（无页面时为展开输入栏，桌宠气泡不会被关闭）
│  │   │   ├─ ScrollComponent.h/cpp # 滚动处理（列表/界面滚动）
│  │   │   ├─ DragComponent.h/cpp # 拖动窗口/界面
│  │   │   └─ InputComponent.h/cpp # UI输入框处理
│  │   └─ Panels/ # UI界面（结构与渲染）
│  │       ├─ ChatPanel/
│  │       │   ├─ BubbleChatPanel.h/cpp # 桌宠消息气泡UI
│  │       │   ├─ InputChatPanel.h/cpp  # 用户输入对话UI
│  │       │   └─ OptionChatPanel.h/cpp # 用户选项对话UI
│  │       └─ ToolPanel/
│  │           ├─ SettingToolPanel.h/cpp # 设置界面UI
│  │           └─ TaskToolPanel.h/cpp    # 任务管理器UI
│  └─ Game/ # 病娇小游戏
│      ├─ GameActor.h/cpp # 游戏本体
│      ├─ GameComponent/
│      │   ├─ GameAudioComponent.h/cpp # 游戏音频
│      │   ├─ GameChatComponent.h/cpp  # 游戏文本
│      └─ GameMap/
│          ├─ Map1.h/cpp #关卡1
│          ├─ Map2.h/cpp #关卡2
│          ├─ Map3.h/cpp #关卡3
└─ main.cpp # 程序入口（初始化系统、启动主循环）


# 功能拆解

# 关键机制

# 开发规则