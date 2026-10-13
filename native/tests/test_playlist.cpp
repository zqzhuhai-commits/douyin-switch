#include "api.hpp"
#include <cassert>
int main(){
 auto videos=Api::parseList(json::parse(R"JSON({"list":[{"id":"../../unsafe","title":"test","url":"https://video.example/a.mp4"},{"id":"bad","url":"javascript:alert(1)"},{"id":"local","url":"sdmc:/switch/douyin-native/videos/a.mp4"}]})JSON"));
 assert(videos.size()==2);
 assert(videos[0].id==Api::stableId("../../unsafe"));
 assert(videos[0].id.find('/')==std::string::npos);
 assert(videos[1].id=="local");
 auto cached=Api::parseList(json::parse(R"JSON({"list":[{"mv_id":"123","mv_title":"imported","mv_play_url":"https://v.example/123.mp4"}]})JSON"));
 assert(cached.size()==1&&cached[0].id=="123"&&cached[0].title=="imported");
 auto raw=Api::parseList(json::parse(R"JSON([{"aweme_id":"7267478481213181238","desc":"公开源","author":{"nickname":"作者"},"video":{"play_addr":{"url_list":["https://www.douyin.com/aweme/v1/play/?video_id=demo"]},"cover":{"url_list":["example.png"]}}}])JSON"));
 assert(raw.size()==1&&raw[0].user=="作者"&&raw[0].cover=="https://raw.githubusercontent.com/zyronon/douyin/master/public/images/example.png");
 assert(Api::parseList(json::parse(R"JSON({"aweme_list":[{"aweme_id":"1","video":{"play_addr":{"url_list":["https://v.example/1.mp4"]}}}]})JSON")).size()==1);
}
