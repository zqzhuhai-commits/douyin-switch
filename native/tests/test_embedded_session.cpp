#include "api.hpp"
#include <cassert>
#include <filesystem>
int main(){
 namespace fs=std::filesystem;
 auto dir=fs::temp_directory_path()/"douyin-embedded-session-test";
 fs::remove_all(dir);fs::create_directories(dir/"sdmc:/switch/douyin-native");fs::create_directories(dir/"romfs:");fs::current_path(dir);
 json seed={{"user_agent","test-agent"},{"params",json::array()},{"visitor",{{"uifid","seed-test"}}}};
 std::ofstream("romfs:/visitor-seed.json")<<seed.dump();
 assert(Api::session()==seed);
 assert(json::parse(readFile("sdmc:/switch/douyin-native/douyin-session.json"))==seed);
 auto updated=seed;updated["visitor"]["uifid"]="updated-test";
 std::ofstream("sdmc:/switch/douyin-native/douyin-session.json")<<updated.dump();
 assert(Api::session()==updated);
 std::ofstream("sdmc:/switch/douyin-native/douyin-session.json")<<"invalid-json";
 std::ofstream("sdmc:/switch/douyin-native/douyin-session.json.bak")<<updated.dump();
 assert(Api::session()==updated);
 std::ofstream("sdmc:/switch/douyin-native/douyin-session.json.bak")<<"{}";
 assert(Api::session()==seed);
 fs::current_path(fs::temp_directory_path());fs::remove_all(dir);
}
