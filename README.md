# 抖音 Switch 测试项目

## 原生播放器 v0.2.0

[下载原生版 NSP](https://github.com/zqzhuhai-commits/douyin-switch/raw/refs/heads/main/dist/DouyinNativeSwitch-0.2.0-test.nsp) · [下载完整测试包](https://github.com/zqzhuhai-commits/douyin-switch/raw/refs/heads/main/dist/DouyinNativeSwitch-0.2.0-test.zip) · [原生版使用说明与源码](native/README.md)

原生版采用自绘界面和本机播放引擎，支持平滑触摸滚动、播放时上下滑/手柄 L/R 切换、自动连播、进度调整、画面旋转和本机收藏。

**尚未接通抖音实时推荐流。** 可用本地视频、媒体直链或 HAR 导入列表测试。已通过编译、相关逻辑测试和安装包完整性检查，尚未进行 Switch 实机验证。

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
