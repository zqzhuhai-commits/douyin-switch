import base64
import importlib.util
import json
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("import_har", Path(__file__).parents[1] / "tools/import_har.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class ImportTest(unittest.TestCase):
    def test_real_shape_base64_duplicates_and_no_credentials(self):
        body = {"aweme_list": [{"aweme_id": "123", "desc": "测试", "author": {"nickname": "作者"},
                               "video": {"play_addr": {"url_list": ["https://video.example/123.mp4"]},
                                         "cover": {"url_list": ["https://image.example/123.jpg"]}}}]}
        entries = [{"request": {"url": "https://www.douyin.com/aweme/v1/web/feed/", "headers": [{"name": "Cookie", "value": "private-cookie"}]},
                    "response": {"content": {"text": json.dumps(body)}}},
                   {"request": {"url": "https://www.douyin.com/aweme/v1/web/feed/"},
                    "response": {"content": {"text": base64.b64encode(json.dumps(body).encode()).decode(), "encoding": "base64"}}}]
        result = module.extract({"log": {"entries": entries}})
        self.assertEqual(len(result["list"]), 1)
        self.assertEqual(result["list"][0]["id"], "123")
        self.assertNotIn("private-cookie", json.dumps(result))

    def test_empty_and_non_douyin_and_missing_play_address(self):
        self.assertEqual(module.extract({"log": {"entries": []}}), {"list": []})
        entry = {"request": {"url": "https://douyin.com.evil.example/api"},
                 "response": {"content": {"text": json.dumps({"aweme_id": "12", "video": {"play_addr": {"url_list": ["https://v.example/v.mp4"]}}})}}}
        self.assertEqual(module.extract({"log": {"entries": [entry]}}), {"list": []})


if __name__ == "__main__":
    unittest.main()
