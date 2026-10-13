#!/usr/bin/env python3
"""Extract Douyin video addresses from HAR response bodies; never export cookies."""
import argparse
import base64
import json
from pathlib import Path
from urllib.parse import urlsplit


def first_url(value):
    if isinstance(value, dict):
        value = value.get("url_list", [])
    if isinstance(value, list):
        for url in value:
            if isinstance(url, str) and url.startswith(("https://", "http://")):
                return url
    return ""


def extract(har):
    videos = {}

    def walk(obj):
        if isinstance(obj, dict):
            video = obj.get("video")
            if isinstance(video, dict) and obj.get("aweme_id"):
                url = first_url(video.get("play_addr"))
                if not url:
                    for bitrate in video.get("bit_rate", []):
                        if isinstance(bitrate, dict):
                            url = first_url(bitrate.get("play_addr"))
                            if url:
                                break
                if url:
                    key = str(obj["aweme_id"])
                    author = obj.get("author", {})
                    videos[key] = {"id": key, "title": str(obj.get("desc") or "抖音视频"),
                                   "author": str(author.get("nickname", "")) if isinstance(author, dict) else "",
                                   "cover": first_url(video.get("cover")) or first_url(video.get("origin_cover")),
                                   "url": url}
            for value in obj.values():
                walk(value)
        elif isinstance(obj, list):
            for value in obj:
                walk(value)

    for entry in har.get("log", {}).get("entries", []):
        host = urlsplit(entry.get("request", {}).get("url", "")).hostname or ""
        if not any(host == domain or host.endswith("." + domain)
                   for domain in ("douyin.com", "iesdouyin.com", "amemv.com", "snssdk.com")):
            continue
        content = entry.get("response", {}).get("content", {})
        text = content.get("text", "")
        if not text:
            continue
        try:
            if content.get("encoding") == "base64":
                text = base64.b64decode(text, validate=True).decode("utf-8")
            walk(json.loads(text))
        except (ValueError, UnicodeError):
            continue
    return {"list": list(videos.values())}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("har", type=Path)
    parser.add_argument("--output", type=Path, default=Path("playlist.json"))
    args = parser.parse_args()
    result = extract(json.loads(args.har.read_text(encoding="utf-8-sig")))
    if not result["list"]:
        parser.exit(2, "没有找到带播放地址的视频响应。请导出包含响应正文的 HAR，并先实际播放几条视频。\n")
    args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(f"已导出 {len(result['list'])} 条视频到 {args.output}；未导出请求 Cookie 或登录头。")
    print("播放地址可能过期；此导入工具不会刷新抖音推荐流或绕过登录校验。")


if __name__ == "__main__":
    main()
