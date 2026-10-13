#pragma once
#include <curl/curl.h>
#include "diagnostics.hpp"
#include "categories.hpp"
#include "feed_refresh.hpp"
#include "video_media.hpp"
#include "live_feed.hpp"
#ifdef __SWITCH__
#include "tls_compat.hpp"
#endif
#include <json.hpp>
#include <fstream>
#include <sstream>
#include <vector>
#include <atomic>
#include <stdexcept>
#include <algorithm>
#include <dirent.h>
#include <cstdio>
#include <cerrno>
using json = nlohmann::json;
struct Video { std::string id, user, title, cover, url; };
struct VideoPage {std::vector<Video> videos;bool rawEmpty=false;std::string note;};
inline std::string readFile(const std::string& p) { std::ifstream f(p, std::ios::binary); return {std::istreambuf_iterator<char>(f), {}}; }
struct NetworkError : std::runtime_error { using std::runtime_error::runtime_error; };
class Api {
 public:
 static constexpr const char* userAgent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36";
 std::string base="https://www.douyin.com",feed="https://www.douyin.com/aweme/v1/web/tab/feed/";
 std::atomic<bool> cancel{false};std::vector<std::string> seen;std::string lastMsToken,lastTtwid;time_t lastServerTime=0;
 static std::string stableId(const std::string& value){uint64_t h=14695981039346656037ULL;for(unsigned char c:value){h^=c;h*=1099511628211ULL;}return std::to_string(h);}
 static bool remote(const std::string& url){return url.rfind("https://",0)==0||url.rfind("http://",0)==0;}
 static bool media(const std::string& url){return remote(url)||url.rfind("sdmc:/switch/douyin-native/videos/",0)==0;}
 static size_t write(char* p,size_t s,size_t n,void* v){auto& b=*(std::string*)v;if(b.size()+s*n>16*1024*1024)return 0;b.append(p,s*n);return s*n;}
 static int progress(void* p,curl_off_t,curl_off_t,curl_off_t,curl_off_t){return ((Api*)p)->cancel.load()?1:0;}
 static size_t header(char*p,size_t s,size_t n,void*userdata){
  auto*api=static_cast<Api*>(userdata);std::string line(p,s*n),name=line.substr(0,line.find(':'));
  std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return std::tolower(c);});
  if(name=="x-ms-token"){auto value=line.substr(line.find(':')+1);auto a=value.find_first_not_of(" \t\r\n"),b=value.find_last_not_of(" \t\r\n");if(a!=std::string::npos&&b-a<4096)api->lastMsToken=value.substr(a,b-a+1);}
  if(name=="date"){auto stamp=curl_getdate(line.substr(line.find(':')+1).c_str(),nullptr);if(stamp>0)api->lastServerTime=stamp;}
  if(name=="set-cookie"){
   auto at=line.find("ttwid=");if(at!=std::string::npos){at+=6;auto end=line.find_first_of(";\r\n",at);auto value=line.substr(at,end-at);if(value.size()<4096)api->lastTtwid=value;}
  }
  return s*n;
 }
 std::string request(const std::string& url,const std::string& body="",const std::vector<std::string>&extra={},const std::string&agent="") {
  CURL* c=curl_easy_init();if(!c)throw std::runtime_error("Network init failed");std::string out;char error[CURL_ERROR_SIZE]={};curl_slist* headers=nullptr;
  curl_easy_setopt(c,CURLOPT_URL,url.c_str());curl_easy_setopt(c,CURLOPT_REFERER,(base+"/").c_str());curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,write);curl_easy_setopt(c,CURLOPT_WRITEDATA,&out);curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT,8L);curl_easy_setopt(c,CURLOPT_TIMEOUT,20L);curl_easy_setopt(c,CURLOPT_NOSIGNAL,1L);curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(c,CURLOPT_MAXREDIRS,3L);curl_easy_setopt(c,CURLOPT_USERAGENT,agent.empty()?userAgent:agent.c_str());curl_easy_setopt(c,CURLOPT_ERRORBUFFER,error);curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,1L);
#ifdef __SWITCH__
  curl_easy_setopt(c,CURLOPT_CAINFO,"romfs:/cacert.pem");
#endif
  curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,2L);curl_easy_setopt(c,CURLOPT_XFERINFOFUNCTION,progress);curl_easy_setopt(c,CURLOPT_XFERINFODATA,this);curl_easy_setopt(c,CURLOPT_NOPROGRESS,0L);
  lastMsToken.clear();lastTtwid.clear();lastServerTime=0;curl_easy_setopt(c,CURLOPT_HEADERFUNCTION,header);curl_easy_setopt(c,CURLOPT_HEADERDATA,this);
  for(const auto&h:extra)headers=curl_slist_append(headers,h.c_str());
  if(!body.empty()){if(std::none_of(extra.begin(),extra.end(),[](const std::string&h){return h.rfind("Content-Type:",0)==0;}))headers=curl_slist_append(headers,"Content-Type: application/x-www-form-urlencoded; charset=UTF-8");headers=curl_slist_append(headers,("Origin: "+base).c_str());curl_easy_setopt(c,CURLOPT_HTTPHEADER,headers);curl_easy_setopt(c,CURLOPT_POSTFIELDS,body.c_str());curl_easy_setopt(c,CURLOPT_POSTFIELDSIZE,(long)body.size());}
  if(headers)curl_easy_setopt(c,CURLOPT_HTTPHEADER,headers);
  
#ifdef __SWITCH__
  resetTlsError();
#endif
  auto rc=curl_easy_perform(c);long status=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&status);curl_slist_free_all(headers);curl_easy_cleanup(c);if(rc!=CURLE_OK){auto reason=std::string(curl_easy_strerror(rc))+" HTTP="+std::to_string(status);if(error[0])reason+=" "+redactMediaUrls(error);
#ifdef __SWITCH__
  auto tlsReason=lastTlsError();if(!tlsReason.empty())reason=tlsReason+"; "+reason;
#endif
  diagnostic("network",reason);throw NetworkError(reason);}if(status!=200){auto reason="HTTP "+std::to_string(status);diagnostic("network",reason);if(status==401||status==403)throw std::runtime_error(reason);throw NetworkError(reason);}return out;
 }
 static std::string firstUrl(const json& value){
  if(value.is_string())return value.get<std::string>();
  if(value.is_object()&&value.contains("url_list"))return firstUrl(value.at("url_list"));
  if(value.is_array())for(const auto& x:value)if(x.is_string()&&!x.get<std::string>().empty())return x.get<std::string>();
  return {};
 }
 static std::vector<Video> parseList(const json& data){
  std::vector<Video> out;
  const json* list=&data;
  if(data.is_object()){
   if(data.contains("list"))list=&data.at("list");
   else if(data.contains("aweme_list"))list=&data.at("aweme_list");
   else if(data.contains("item_list"))list=&data.at("item_list");
   else throw std::runtime_error("JSON 无视频列表");
  }
  if(!list->is_array())throw std::runtime_error("JSON 列表格式错误");
  for(const auto& x:*list){
   if(!x.is_object())continue;
   std::string url,title,author,cover,original;
   if(x.contains("video")&&x.at("video").is_object()){
    const auto& video=x.at("video");
    url=switchVideoUrl(video);
    original=x.value("aweme_id",std::string());title=x.value("desc",std::string("抖音视频"));
    if(x.contains("author")&&x.at("author").is_object())author=x.at("author").value("nickname",std::string());
    if(video.contains("cover"))cover=firstUrl(video.at("cover"));
    if(!cover.empty()&&!remote(cover)&&cover.find('/')==std::string::npos&&cover.find("..") == std::string::npos)
     cover="https://raw.githubusercontent.com/zyronon/douyin/master/public/images/"+cover;
   }else{
    url=x.value("url",x.value("mv_play_url",std::string()));
    original=x.value("id",x.value("mv_id",url));title=x.value("title",x.value("mv_title",std::string("抖音视频")));
    author=x.value("author",x.value("mu_id",std::string()));cover=x.value("cover",x.value("mv_img_url",std::string()));
   }
   if(!media(url))continue;
   auto id=original;
   if(id.empty()||id.size()>64||!std::all_of(id.begin(),id.end(),[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-';}))id=stableId(original.empty()?url:original);
   out.push_back({id,author,title,remote(cover)?cover:std::string(),url});
  }return out;
 }
 static std::vector<Video> readPlaylist(const std::string& path){try{auto s=readFile(path);if(s.empty())return {};return parseList(json::parse(s));}catch(...){return {};}}
 static json encodeList(const std::vector<Video>& videos){json list=json::array();for(const auto&v:videos)list.push_back({{"id",v.id},{"title",v.title},{"author",v.user},{"cover",v.cover},{"url",v.url}});return {{"list",list}};}

 static std::vector<Video> localVideos(){
  std::vector<Video> out;const std::string path="sdmc:/switch/douyin-native/videos/";
  DIR* dir=opendir(path.c_str());if(!dir)return out;
  while(auto* e=readdir(dir)){std::string name=e->d_name;auto pos=name.find_last_of('.');if(pos==std::string::npos)continue;
   std::string ext=name.substr(pos);std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return std::tolower(c);});
   if(ext!=".mp4"&&ext!=".mkv"&&ext!=".mov"&&ext!=".webm")continue;
   out.push_back({stableId(name),"本地",name,"",path+name});
  }closedir(dir);std::sort(out.begin(),out.end(),[](const Video&a,const Video&b){return a.title<b.title;});return out;
 }
 static json session(){
  auto text=readFile("sdmc:/switch/douyin-native/douyin-session.json");
  if(text.empty())text=readFile("sdmc:/switch/douyin-native/douyin-session.json.bak");
  if(text.empty())throw std::runtime_error("缺少访客配置：复制 douyin-session.json 后按 Y");
  return json::parse(text);
 }
 static void saveSession(const json&cfg){
  const std::string target="sdmc:/switch/douyin-native/douyin-session.json",tmp=target+".tmp";
  std::ofstream f(tmp,std::ios::binary);auto data=cfg.dump();f.write(data.data(),data.size());f.close();
  if(!f)return;
  if(std::rename(tmp.c_str(),target.c_str())!=0&&errno==EEXIST){
   auto backup=target+".bak";std::remove(backup.c_str());
   if(std::rename(target.c_str(),backup.c_str())==0){if(std::rename(tmp.c_str(),target.c_str())!=0)std::rename(backup.c_str(),target.c_str());else std::remove(backup.c_str());}
  }
 }
 json liveRequest(const std::string&path,const DouyinSign::Pairs&extra){
  static std::mutex mutex;std::lock_guard<std::mutex> lock(mutex);
  auto cfg=session();
  if(cfg.at("visitor").value("ttwid",std::string()).empty()){
   try{
    request("https://ttwid.bytedance.com/ttwid/union/register/",R"({"region":"cn","aid":1768,"needFid":false,"service":"www.ixigua.com","migrate_info":{"ticket":"","source":"node"},"cbUrlProtocol":"https","union":true})",{"Content-Type: application/json"},cfg.at("user_agent").get<std::string>());
    if(!lastTtwid.empty())cfg["visitor"]["ttwid"]=lastTtwid;
    if(lastServerTime>0)cfg["clock_offset_seconds"]=int64_t(lastServerTime)-int64_t(std::time(nullptr));
    saveSession(cfg);
   }catch(const std::exception&e){diagnostic("visitor-bootstrap",e.what());}
  }
  auto clock=LiveFeed::nowMs()+cfg.value("clock_offset_seconds",int64_t(0))*1000;
  auto signedRequest=LiveFeed::sign(cfg,path,extra,clock);
  auto text=request(signedRequest.url,"",signedRequest.headers,cfg.at("user_agent").get<std::string>());
  if(text.empty())throw std::runtime_error("推荐接口返回空响应，访客信息或签名可能已失效");
  auto response=json::parse(text);auto code=response.value("status_code",-1);
  if(code!=0)throw std::runtime_error("抖音接口 status_code="+std::to_string(code));
  if(!lastMsToken.empty())cfg["visitor"]["ms_token"]=lastMsToken;
  if(lastServerTime>0)cfg["clock_offset_seconds"]=int64_t(lastServerTime)-int64_t(std::time(nullptr));
  saveSession(cfg);return response;
 }

 VideoPage listPage(int tab,int page){
  if(tab==Category::All){if(page>1)return {{},true,{}};auto v=localVideos();for(const auto&x:readPlaylist("sdmc:/switch/douyin-native/playlist.json"))if(std::none_of(v.begin(),v.end(),[&](const Video&y){return y.id==x.id;}))v.push_back(x);return {v,v.empty(),"本地与导入视频"};}
  if(feed!="https://www.douyin.com/aweme/v1/web/tab/feed/"){
   if(page>1)return {{},true,{}};
   auto v=parseList(json::parse(request(feed)));return {v,v.empty(),"自定义 JSON 源"};
  }
  auto response=liveRequest("/aweme/v1/web/tab/feed/",LiveFeed::pageParams(page,seen));
  auto videos=parseList(response);
  return {videos,response.value("has_more",0)==0||videos.empty(),"实时推荐已加载第 "+std::to_string(page)+" 批"};
 }

 std::string detail(const Video&v){
  if(!media(v.url))throw std::runtime_error("无有效媒体地址");
  if(v.url.find("douyinvod.com")!=std::string::npos||v.url.find("douyin.com/aweme/v1/play")!=std::string::npos){
   auto response=liveRequest("/aweme/v1/web/aweme/detail/",{{"aweme_id",v.id}});
   if(!response.contains("aweme_detail"))throw std::runtime_error("详情接口没有视频数据");
   auto fresh=parseList(json::array({response.at("aweme_detail")}));
   if(fresh.empty())throw std::runtime_error("视频无 H.264 MP4 播放地址");
   return fresh[0].url;
  }
  return v.url;
 }
};
