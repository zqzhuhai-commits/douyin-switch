#include "live_feed.hpp"
#include <cassert>
int main(){
 auto cfg=nlohmann::json::parse(R"({"user_agent":"Mozilla/5.0 Chrome/151.0.0.0","visitor":{"uifid":"test-visitor","verify_fp":"test-fp","ms_token":"test-token"},"params":[["aid","6383"],["support_h265","1"],["support_dash","1"],["a_bogus","old-signature"],["timestamp","1"]]})");
 auto one=LiveFeed::sign(cfg,"/aweme/v1/web/tab/feed/",LiveFeed::pageParams(1,{}),1791904742000LL,[]{return .5;});
 auto two=LiveFeed::sign(cfg,"/aweme/v1/web/tab/feed/",LiveFeed::pageParams(2,{"123","456"}),1791904743000LL,[]{return .5;});
 assert(one.url!=two.url);
 assert(one.url.find("support_h265=0")!=std::string::npos&&one.url.find("support_dash=0")!=std::string::npos);
 assert(two.url.find("refresh_index=1")!=std::string::npos&&two.url.find("filterGids=123%2C456")!=std::string::npos);
 assert(one.url.find("old-signature")==std::string::npos);
 assert(one.url.find("timestamp=1791904742")!=std::string::npos);
 assert(one.headers.size()==5);
 cfg["visitor"]["uifid"]="";bool rejected=false;
 try{LiveFeed::sign(cfg,"/aweme/v1/web/tab/feed/",{});}catch(...){rejected=true;}
 assert(rejected);
}
