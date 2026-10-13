# 抖音原生播放器 Switch · 0.2.1 测试版

本版使用 SDL/OpenGL 自绘界面和 libmpv/FFmpeg 本机播放引擎。应用不调用 WebApplet。

## 当前能力和限制

- 原生视频列表，6 列、9:16 封面；平滑触摸滚动、惯性减速、滚动条。
- 播放界面上滑下一条，下滑上一条；L/R 手柄切换；默认自动连播。
- 点击视频显示控制栏；A 暂停/继续；B 返回列表；Y 旋转画面；X 连播开关。
- 控制栏显示进度并支持触摸调整；方向左右快退/快进，方向上下调节音量。
- 视频列表 X 添加/取消本机收藏；L/R 分类；Y 刷新；减号添加媒体直链。
- 可读取导入的抖音视频列表、HTTP/HTTPS 媒体直链以及 SD 卡本地视频。

**本版内置公开项目的 6 条抖音历史示例视频 JSON，每次启动会联网更新公开源。尚未接通抖音实时推荐、登录、搜索、关注或点赞接口。**
启动时立即显示缓存或内置列表，并在后台获取最新 JSON；远程列表解析有效后保存缓存。更新失败使用上次缓存，首次离线使用内置列表。Y 可手动重新更新。

默认公开源：`https://raw.githubusercontent.com/zyronon/douyin/master/src/assets/data/posts6.json`。该源是历史示例；源文件未变化时内容不会变化。它不是官方推荐接口，示例视频的媒体链接尚未验证有效。

数据来源及许可证位于 `third_party/`。也可继续导入媒体列表、使用本地视频或媒体直链。
HAR 中的播放地址可能过期，部分视频还可能需要会话信息，因此导入不保证每条视频可播放。
应用不会绕过登录、验证码或访问校验。需要新的抖音网络请求样本才能进一步开发数据接入。

## 安装

安装 `DouyinNativeSwitch-0.2.1-test.nsp` 并从 Switch HOME 启动。仅适用于可安装自制 NSP 的系统。
Title ID 为 `0100D0A11A020000`，与网页版测试版不同，两版可分别安装。

## 导入抖音视频

1. 电脑浏览器打开抖音，正常播放多条视频。
2. 开发者工具 Network 导出含响应内容的 HAR；导出时移除敏感请求头。
3. 在电脑安装 Python 3，执行 `python tools/import_har.py douyin.har --output playlist.json`。
4. 把输出复制到 SD 卡 `/switch/douyin-native/playlist.json`。
5. 启动程序或在“公开视频”分类按 Y 刷新。

转换工具仅提取响应中的视频 ID、标题、作者、封面及媒体地址，不导出 Cookie 或登录请求头。
如果导出 0 条视频，需要重新导出包含视频 JSON 响应正文的 HAR。

也可自己提供 JSON：顶层 `list` 数组，每项字段 `id`、`title`、`author`、`cover`、`url`。
`url` 必须是媒体地址，抖音分享页面地址不能直接交给播放器。

## 本地视频

将 MP4/MKV/MOV/WebM 放到 SD 卡 `/switch/douyin-native/videos/`，切换到“本地视频”分类。
是否能播放取决于视频编码、分辨率和 Switch 解码能力。

## 验证与反馈

已通过 Switch 原生编译、播放列表解析、HAR 转换及手势方向分类测试，以及 NSP 结构/内容完整性检查。
尚未进行 Switch 实机验证，触摸体验、音视频解码和抖音 CDN 播放均待实测。
日志：`/switch/douyin-native/playback.log` 和 `error.log`。

## 构建

需要 devkitPro/devkitA64、libnx、SDL2、OpenGL、libmpv、FFmpeg、curl 等 Switch 静态依赖。
执行 `make` 生成 NRO。NSP 打包参考 `tools/package.sh`；密钥需自行合法取得，包内不包含密钥。
NRO 必须在完整应用模式启动。

## 自定义更新地址

可在 SD 卡 `/switch/douyin-native/config.json` 设置 `feed_url` 为自己的 HTTPS JSON 地址。支持 `list`、`aweme_list`、`item_list` 或顶层数组；无效/空远程数据不会覆盖已有缓存。页面 HTML 地址不是 JSON 地址。
