# 1.项目规划
首先创建项目文件夹，命名为 Pet
接下来创建子文件夹分别用于存放不同文件
Pet/ # 项目文件夹
├─ .vscode/     # 项目配置目录
├─ Build/       # 编译中间文件
├─ Distribution/# 面向用户的最终可执行文件
├─ Docs/        # 项目指导文档
└─ Source/      # 源码
## 1.1.为vscord配置环境
并添加编译代码的脚本，通过vscord启动编译脚本
├─ .vscode/     # 项目配置目录
│  ├─ c_cpp_properties.json # 为vscord解释代码
│  ├─ build.ps1             # 项目编译脚本
│  └─ tasks.json            # 通过vscord进行编译的途径
## 1.2.配置编译脚本
使编译脚本自动检索Source/并进行编译
将obj编译中间文件生成到Build/文件夹
将exe可执行文件生成到Distribution/文件夹
├─ Build/       # 编译中间文件
│  └─ obj
## 1.3.规划发行的文件结构
创建不同种类资源所用的文件夹，方便整理资源
├─ Distribution/# 面向用户的最终可执行文件
│  ├─ alpha/     # 内测版本
│  │  └─ Miss_qing/ # 以角色名命名素材文件夹，以便用户快捷更换自定义桌宠
│  │     ├─ assets/ # 资源
│  │     │  ├─ audio/  # 音频
│  │     │  ├─ chat/   # 文本
│  │     │  └─ images/ # 图像
│  │     ├─ config/ # 配置文件
│  │     │  ├─ settings.json # 设置
│  │     │  └─ state.json    # 桌宠状态
│  │     ├─ diary.txt  # 桌宠每次结束运行时会写的日记
│  │     └─ Pet.exe    # 可执行exe文件
│  └─ beta/      # 公测版本
## 1.4.添加各方面的指导文档
├─ Docs/        # 项目指导文档
│  ├─ Architecture分层与依赖规则.md
│  ├─ Changelog版本变更记录.md
│  ├─ CodingStyle命名与代码风格.md
│  ├─ DevelopmentGuide开发流程手册.md
│  ├─ ProjectGuide项目总指导.md
│  └─ Roadmap开发计划和里程碑.md
## 1.5.最终效果
最终的项目目录大概是这样的结构
先创建文件夹与空文件，后期再里面添加内容
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
└─ Source/      # 源码

# 2.模块划分
Source/
├─ Core/    #（基础库）存放整个系统都会用到的小工具，方便各层级调用
├─ Domain/  #（流程库）预设了各种流程，形成用户可直接用的功能
├─ Engine/  #（引擎层）提供底层接口，Systems 调用它来实现功能
├─ Runtime/ #（协议层）用于分发事件，是系统的通信中枢
├─ Systems/ #（系统层）存放着actor和其挂载的Component、Panel，用于实现业务逻辑
└─ main.cpp # 程序入口（初始化系统、启动主循环）
## 2.1.Core基础库
不依赖任何模块，并且被任何模块所依赖
存放一些很基础的功能
先创建几个空文件，方便目录的区分
├─ Core/ # 基础库
│  ├─ Path.h/cpp       # 计算和处理文件路径
│  ├── FileSystem.h/cpp  # 在磁盘中读写文件（依赖于Path）
│  ├─── TextFile.h/cpp     # 支持热更新的读写文本功能（依赖于FileSystem）
│  ├─── Logger.h/cpp       # 输出日志文件（依赖于FileSystem）
│  ├─── Config.h/cpp       # 配置管理器（依赖于FileSystem）
│  └─ Timer.h/cpp      # 程序统一的时间工具
## 2.2.Domain流程库
这个目前还空着
## 2.3.Engine引擎层
依赖Core，被Systems依赖
封装操作系统接口
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
## 2.4.Runtime协议层
依赖Core，被Systems依赖
负责调度与事件管理
程序初始化时，Systems层会在Runtime层注册自己感兴趣的事件
当事件触发时，Runtime层会通知Systems层调用Engine层的接口来完成具体的功能实现
├─ Runtime/ # 协议层/中枢（无具体业务）
│  ├─ EventBus.h/cpp # 事件发布/订阅中心（模块通信核心）
│  ├─ Scheduler.h/cpp # 定时任务调度（延迟/循环任务）
│  └─ StateManager.h/cpp # 全局状态管理（全局开关/配置）
## 2.5.Systems系统层
依赖Core / Runtime / Engine，不被任何模块依赖
实现具体业务逻辑（桌宠、UI、设置等）
├─ Systems/ # 系统层（核心逻辑）
### 2.5.1.PetActor
│  ├─ Pet/ # 桌宠系统
│  │   ├─ PetActor.h/cpp # 桌宠核心对象（状态机+组件管理）
│  │   └─ Components/ # 桌宠功能组件
│  │       ├─ AudioComponent.h/cpp # 音频逻辑
│  │       ├─ ChatComponent.h/cpp  # 文本逻辑
│  │       ├─ DiaryComponent.h/cpp # 写入桌宠日志/日记
│  │       ├─ InputComponent.h/cpp # 处理用户输入内容
│  │       └─ PetRenderComponent.h/cpp # 规划桌宠的绘制（位置/贴图/动画）
### 2.5.2.UIActor
ChatPanel跟随桌宠移动，ToolPanel有自己的拖动功能。两个都调用Engine层的功能
如果是正常的软件会需要一个MainToolPanel容器ui
但我的桌宠本质上是一个铺满屏幕的大窗口，里面全部内容都是绘制的所以不需要容器
│  ├─ UI/ # UI系统
│  │   ├─ UIActor.h/cpp # UI核心对象（管理所有UI界面）
│  │   ├─ Components/ # UI行为组件（交互逻辑）
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
### 2.5.3.GameActor
│  └─ Game/ # 病娇小游戏
│      ├─ GameActor.h/cpp # 游戏本体
│      ├─ GameComponent/
│      │   ├─ GameAudioComponent.h/cpp # 游戏音频
│      │   ├─ GameChatComponent.h/cpp  # 游戏文本
│      └─ GameMap/
│          ├─ Map1.h/cpp #关卡1
│          ├─ Map2.h/cpp #关卡2
│          ├─ Map3.h/cpp #关卡3
## 2.6.主循环程序
└─ main.cpp # 程序入口（初始化系统、启动主循环）
## 2.7.最终效果
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
│  │   └─ Components/ # 桌宠功能组件
│  │       ├─ AudioComponent.h/cpp # 音频逻辑
│  │       ├─ ChatComponent.h/cpp  # 文本逻辑
│  │       ├─ DiaryComponent.h/cpp # 写入桌宠日志/日记
│  │       ├─ InputComponent.h/cpp # 处理用户输入内容
│  │       └─ PetRenderComponent.h/cpp # 规划桌宠的绘制（位置/贴图/动画）
│  ├─ UI/ # UI系统
│  │   ├─ UIActor.h/cpp # UI核心对象（管理所有UI界面）
│  │   ├─ Components/ # UI行为组件（交互逻辑）
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

# 3.开发规范

## 3.1.职责与依赖关系
Core（基础库）
职责：
- 提供基础工具
- 提供通用工具类
非职责：
- 不包含任何业务逻辑
依赖关系：
- 不依赖任何模块
- 被任何模块依赖

Runtime（通信层）
职责：
- 事件订阅系统EventBus
- 定时调度系统Scheduler
- 全局状态管理StateManager
非职责：
- 不包含任何业务逻辑
依赖关系：
- 依赖Core
- 被Systems依赖

Engine（引擎层）
职责：
- 封装窗口系统、输入系统、渲染
- 提供与操作系统交互的能力
非职责：
- 不包含任何业务逻辑
依赖关系：
- 依赖Core
- 被Systems依赖

Systems（业务层）
职责：
- 实现具体业务逻辑（桌宠、UI、设置等）
非职责：
- 不实现底层工具
- 不处理操作系统级功能
依赖关系：
- 依赖Core / Runtime / Engine
- 不被任何模块依赖

## 3.2.系统边界
Core 只放通用工具
Engine 只提供能力
一切事件统一通过EventBus分发
业务逻辑只存在于Systems
## 3.3.命名规则

## 3.4.注释规范
每一段复杂代码都在开头说明：为什么这么做、输入输出、边界条件