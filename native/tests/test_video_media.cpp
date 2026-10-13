#include "video_media.hpp"
#include <cassert>
int main(){
 auto v=nlohmann::json::parse(R"JSON({"play_addr":{"url_list":["https://v.example/default"]},"bit_rate":[{"is_h265":1,"format":"mp4","bit_rate":100,"play_addr":{"url_list":["https://v.example/hevc"]}},{"is_h265":0,"format":"dash","bit_rate":200,"play_addr":{"url_list":["https://v.example/dash"]}},{"is_h265":0,"format":"mp4","bit_rate":600000,"play_addr":{"url_list":["https://v.example/high"]}},{"is_h265":0,"format":"mp4","bit_rate":400000,"play_addr":{"url_list":["https://v.example/low"]}}]})JSON");
 assert(switchVideoUrl(v)=="https://v.example/low");
 assert(switchVideoUrl(nlohmann::json::parse(R"JSON({"play_addr_h264":{"url_list":["https://v.example/h264"]},"play_addr":{"url_list":["https://v.example/default"]}})JSON"))=="https://v.example/h264");
 assert(httpMediaUrl(nlohmann::json::parse(R"JSON({"url_list":["javascript:alert(1)"]})JSON")).empty());
}
