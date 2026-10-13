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
}
