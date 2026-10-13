# 抖音原生播放器 Switch · 0.3.0 实时推荐测试版

使用 SDL/OpenGL 自绘界面及 libmpv/FFmpeg 本机播放器；运行期间仅需要 Switch 联网。

## 安装

1. 安装 `DouyinNativeSwitch-0.3.0-test.nsp`，从 HOME 启动。
2. 将单独提供的私人 `douyin-session.json` 复制到 SD 卡 `/switch/douyin-native/douyin-session.json`。
3. 在“实时推荐”页按 Y 加载。以后启动自动请求推荐，接近列表/播放队列末尾时按需追加下一批。

安装包、源码和公开仓库不含私人配置、HAR、登录 Cookie 或抓包旧签名。访客配置在本机保存；请勿公开上传。
本版没有帐号登录功能，使用导入的访客资料请求推荐。访客资料失效时可能需要更新配置；不能保证永久有效。

## 推荐与播放

- 请求抖音 `/aweme/v1/web/tab/feed/`，每次在 Switch 上重新计算 A-Bogus 和 Web Signature。
- 更新批次索引并携带最近已加载的视频 ID，去重并追加到连续列表。
- 播放前请求 `/aweme/v1/web/aweme/detail/` 获取当前地址，优先选低码率 H.264 MP4。
- 推荐接口返回的 `x-ms-token` 会写回私人配置，不放进公开安装包或仓库；首次尝试注册普通访客 ttwid。
- 显示真实网络/接口错误，保留已加载列表；Y 重试。没有内置历史示例或旧抓包列表回退。

## 操作

列表支持 6 列、9:16 封面、平滑触摸滚动和滚动条；A 播放、X 收藏、L/R 分类、Y 刷新。
播放时上滑下一条、下滑上一条；L/R 手柄切换；默认连播，X 开关。
点击显示控制栏；A 暂停/继续、B 返回、Y 旋转；进度条触摸调整，方向键左右快退/快进、上下调音量。
“本地/导入”读取 `/switch/douyin-native/videos/` 和 `playlist.json`；列表减号可添加媒体直链。

## 验证状态

已完成：HAR 推荐响应解析、6 条视频 H.264 MP4 选择验证、签名 C++/Python 交叉校验、参数/分页/去旧签名测试、Switch 编译及安装包完整性检查。
**尚未完成：当前新签名请求的线上接受验证与 Switch 实机播放。** 构建环境无法连通抖音接口；请按实验测试版使用，不代表已确认可持续刷推荐。
接口可能拒绝或限制原生客户端，访客资料可能失效；失败时请反馈屏幕错误及本机日志。
日志：`/switch/douyin-native/playback.log`、`error.log`；日志会隐藏媒体 URL。不要公开 HAR 或私人配置。

## 构建与来源

需要 devkitPro/devkitA64、libnx、SDL2、OpenGL、libmpv、FFmpeg、curl 等 Switch 静态依赖。`make` 生成 NRO，NSP 打包参考 `tools/package.sh`。密钥自备，源码不含密钥。
动态签名基于 Evil0ctal/Douyin_TikTok_Download_API 的 Apache-2.0 原生算法移植，来源与许可证见 `third_party/`。
Title ID：`0100D0A11A020000`，可覆盖旧原生版；与网页版独立。
