#pragma once
#include "douyin_sign.hpp"
#include <json.hpp>
#include <set>
#include <chrono>
namespace LiveFeed {
using json=nlohmann::json;
struct Request {std::string url;std::vector<std::string> headers;};
inline void set(DouyinSign::Pairs&p,const std::string&key,const std::string&value){for(auto&x:p)if(x.first==key){x.second=value;return;}p.emplace_back(key,value);}
inline std::string headerSafe(const std::string&s){if(s.find_first_of("\r\n")!=std::string::npos)throw std::runtime_error("访客配置包含无效字符");return s;}
inline std::string clean(const std::string&s){if(s.find_first_of("\r\n;")!=std::string::npos)throw std::runtime_error("访客配置包含无效字符");return s;}
inline int64_t nowMs(){return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
inline Request sign(const json&cfg,const std::string&path,const DouyinSign::Pairs&extra,int64_t now=nowMs(),std::function<double()>rng=DouyinSign::noise){
 if(path!="/aweme/v1/web/tab/feed/"&&path!="/aweme/v1/web/aweme/detail/")throw std::runtime_error("unsupported Douyin endpoint");
 const auto&visitor=cfg.at("visitor");auto uifid=clean(visitor.value("uifid",std::string()));
 if(uifid.empty())throw std::runtime_error("请复制 douyin-session.json 到 /switch/douyin-native/");
 std::string ua=headerSafe(cfg.at("user_agent").get<std::string>()),geometry=cfg.value("browser_info",std::string("1920|947|1920|1032|1920|1032|1920|1080|Win32"));
 DouyinSign::Pairs pairs;
 const std::set<std::string> old={"a_bogus","verifyFp","fp","uifid","timestamp","x-secsdk-web-signature","msToken","filterGids","refresh_index","view_count","webid","globalwid"};
 for(const auto&pair:cfg.at("params")){if(!pair.is_array()||pair.size()!=2||!pair[0].is_string()||!pair[1].is_string())throw std::runtime_error("访客配置参数格式错误");auto key=pair[0].get<std::string>();if(!old.count(key))set(pairs,key,pair[1].get<std::string>());}
 set(pairs,"aid","6383");set(pairs,"support_h265","0");set(pairs,"support_dash","0");
 for(const auto&p:extra)set(pairs,p.first,p.second);
 for(const char*key:{"webid","globalwid"}){auto value=visitor.value(key,std::string());if(!value.empty())set(pairs,key,value);}
 auto token=clean(visitor.value("ms_token",std::string()));if(!token.empty())set(pairs,"msToken",token);
 auto bogus=DouyinSign::aBogus(DouyinSign::query(pairs),ua,now,rng,geometry);pairs.emplace_back("a_bogus",bogus);
 auto fp=clean(visitor.value("verify_fp",std::string()));if(!fp.empty()){pairs.emplace_back("verifyFp",fp);pairs.emplace_back("fp",fp);}
 pairs.emplace_back("uifid",uifid);pairs.emplace_back("timestamp",std::to_string(now/1000));
 auto covered=DouyinSign::query(pairs);auto signature=DouyinSign::webSignature(covered,uifid,now/1000);
 std::string cookies="UIFID_TEMP="+uifid;if(!fp.empty())cookies+="; s_v_web_id="+fp;if(!token.empty())cookies+="; msToken="+token;
 auto ttwid=clean(visitor.value("ttwid",std::string()));if(!ttwid.empty())cookies+="; ttwid="+ttwid;
 return {"https://www.douyin.com"+path+"?"+covered+"&x-secsdk-web-signature="+signature,
  {"Accept: application/json","uifid: "+uifid,"x-secsdk-web-signature: "+signature,"x-secsdk-web-expire: "+std::to_string(now/1000),"Cookie: "+cookies}};
}
inline DouyinSign::Pairs pageParams(int page,const std::vector<std::string>&seen){
 if(page<1)throw std::runtime_error("invalid recommendation page");
 std::string filter;size_t start=seen.size()>100?seen.size()-100:0;
 for(size_t i=start;i<seen.size();i++){if(!filter.empty())filter+=',';filter+=seen[i];}
 return {{"count","10"},{"refresh_index",std::to_string(page-1)},{"pull_type",page==1?"1":"2"},{"filterGids",filter},{"view_count",std::to_string(seen.size())}};
}
}
