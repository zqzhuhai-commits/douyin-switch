# 抖音 Switch 测试项目

## 原生播放器 v0.3.1 · 内置访客配置实验版

[下载内置版 NSP](https://github.com/zqzhuhai-commits/douyin-switch/raw/refs/heads/main/dist/DouyinNativeSwitch-0.3.1-embedded.nsp) · [完整测试包](https://github.com/zqzhuhai-commits/douyin-switch/raw/refs/heads/main/dist/DouyinNativeSwitch-0.3.1-embedded.zip) · [使用说明与源码](native/README.md)

直接安装 NSP，从 HOME 启动，无需手动复制 JSON。首次自动读取内置访客参数并保存到 SD 卡，之后优先使用本地更新的配置；缺失或损坏时回退到内置参数。

按用户明确授权，本版公开了内置普通访客参数。不含账号登录 Cookie、密码、HAR 原文件或抓包旧签名。访客参数可能失效，内置并不保证永久有效。

Switch 本机动态生成签名并请求推荐/详情，接近末尾按需加载，支持触摸/手柄切换与自动连播。**已通过配置加载测试、离线协议验证、编译及安装包完整性检查；线上接受与 Switch 实机播放仍待验证。**

---

# 抖音网页版 Switch 测试版

非官方 Switch 自制网页启动器，启动后调用系统网页组件打开抖音推荐页。

## 下载和安装

直接下载 [DouyinSwitch-0.1.0-test.nsp](https://github.com/zqzhuhai-commits/douyin-switch/raw/refs/heads/main/dist/DouyinSwitch-0.1.0-test.nsp)，安装后从 HOME 菜单启动。仅适用于可安装自制 NSP 的 Switch。

## 操作

- 网页内：触摸或左摇杆光标，按网页组件底部提示操作。
- 返回启动器后：A 推荐页；X 首页；Y 精选页；+ 退出。

## 验证状态

已通过 devkitA64/libnx 编译、NSP 结构和内容完整性检查。未完成 Switch 实机验证；视频播放、登录、自动播放和登录状态保留均不保证兼容。
出现白屏或无法播放时，请反馈错误画面、系统版本及 Atmosphère 版本。

## 构建

安装 devkitPro 的 Switch 开发工具后执行 `make`。生成的 NRO 应在完整应用模式运行，不能从相册 applet 模式启动。
NSP 打包需要 elf2nso、npdmtool 和 hacBrewPack，使用本机合法取得的打包密钥；仓库不包含密钥或任天堂系统文件。

Title ID：`0100D0A11A010000`。
