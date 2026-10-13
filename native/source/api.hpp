#pragma once
#include <curl/curl.h>
#include "diagnostics.hpp"
#include "categories.hpp"
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
using json = nlohmann::json;
struct Video { std::string id, user, title, cover, url; };
struct VideoPage {std::vector<Video> videos;bool rawEmpty=false;};
inline std::string readFile(const std::string& p) { std::ifstream f(p, std::ios::binary); return {std::istreambuf_iterator<char>(f), {}}; }
struct NetworkError : std::runtime_error { using std::runtime_error::runtime_error; };
class Api {
 public:
 static constexpr const char* userAgent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";
 std::string base="https://www.douyin.com",feed;
 std::atomic<bool> cancel{false};
 static std::string stableId(const std::string& value){uint64_t h=14695981039346656037ULL;for(unsigned char c:value){h^=c;h*=1099511628211ULL;}return std::to_string(h);}
 static bool remote(const std::string& url){return url.rfind("https://",0)==0||url.rfind("http://",0)==0;}
 static bool media(const std::string& url){return remote(url)||url.rfind("sdmc:/switch/douyin-native/videos/",0)==0;}
 static size_t write(char* p,size_t s,size_t n,void* v){auto& b=*(std::string*)v;if(b.size()+s*n>16*1024*1024)return 0;b.append(p,s*n);return s*n;}
 static int progress(void* p,curl_off_t,curl_off_t,curl_off_t,curl_off_t){return ((Api*)p)->cancel.load()?1:0;}
 std::string request(const std::string& url,const std::string& body="") {
  CURL* c=curl_easy_init();if(!c)throw std::runtime_error("Network init failed");std::string out;char error[CURL_ERROR_SIZE]={};curl_slist* headers=nullptr;
  curl_easy_setopt(c,CURLOPT_URL,url.c_str());curl_easy_setopt(c,CURLOPT_REFERER,(base+"/").c_str());curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,write);curl_easy_setopt(c,CURLOPT_WRITEDATA,&out);curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT,8L);curl_easy_setopt(c,CURLOPT_TIMEOUT,20L);curl_easy_setopt(c,CURLOPT_NOSIGNAL,1L);curl_easy_setopt(c,CURLOPT_FOLLOWLOCATION,1L);curl_easy_setopt(c,CURLOPT_MAXREDIRS,3L);curl_easy_setopt(c,CURLOPT_USERAGENT,userAgent);curl_easy_setopt(c,CURLOPT_ERRORBUFFER,error);curl_easy_setopt(c,CURLOPT_SSL_VERIFYPEER,1L);
#ifdef __SWITCH__
  curl_easy_setopt(c,CURLOPT_CAINFO,"romfs:/cacert.pem");
#endif
  curl_easy_setopt(c,CURLOPT_SSL_VERIFYHOST,2L);curl_easy_setopt(c,CURLOPT_XFERINFOFUNCTION,progress);curl_easy_setopt(c,CURLOPT_XFERINFODATA,this);curl_easy_setopt(c,CURLOPT_NOPROGRESS,0L);
  if(!body.empty()){headers=curl_slist_append(headers,"Content-Type: application/x-www-form-urlencoded; charset=UTF-8");headers=curl_slist_append(headers,("Origin: "+base).c_str());curl_easy_setopt(c,CURLOPT_HTTPHEADER,headers);curl_easy_setopt(c,CURLOPT_POSTFIELDS,body.c_str());curl_easy_setopt(c,CURLOPT_POSTFIELDSIZE,(long)body.size());}
  
#ifdef __SWITCH__
  resetTlsError();
#endif
  auto rc=curl_easy_perform(c);long status=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&status);curl_slist_free_all(headers);curl_easy_cleanup(c);if(rc!=CURLE_OK){auto reason=std::string(curl_easy_strerror(rc))+" HTTP="+std::to_string(status);if(error[0])reason+=" "+redactMediaUrls(error);
#ifdef __SWITCH__
  auto tlsReason=lastTlsError();if(!tlsReason.empty())reason=tlsReason+"; "+reason;
#endif
  diagnostic("network",reason);throw NetworkError(reason);}if(status!=200){auto reason="HTTP "+std::to_string(status);diagnostic("network",reason);if(status==401||status==403)throw std::runtime_error(reason);throw NetworkError(reason);}return out;
 }
 static std::vector<Video> parseList(const json& data){
  std::vector<Video> out;
  const json& list=data.is_array()?data:data.at("list");
  for(const auto& x:list){
   auto url=x.value("url",x.value("mv_play_url",std::string()));
   if(!media(url))continue;
   auto original=x.value("id",x.value("mv_id",url));
   auto id=original;
   if(id.empty()||id.size()>64||!std::all_of(id.begin(),id.end(),[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-';}))id=stableId(original);
   out.push_back({id,x.value("author",x.value("mu_id",std::string())),x.value("title",x.value("mv_title",std::string("抖音视频"))),remote(x.value("cover",x.value("mv_img_url",std::string())))?x.value("cover",x.value("mv_img_url",std::string())):std::string(),url});
  }return out;
 }
 static std::vector<Video> localVideos(){
  std::vector<Video> out;const std::string path="sdmc:/switch/douyin-native/videos/";
  DIR* dir=opendir(path.c_str());if(!dir)return out;
  while(auto* e=readdir(dir)){std::string name=e->d_name;auto pos=name.find_last_of('.');if(pos==std::string::npos)continue;
   std::string ext=name.substr(pos);std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return std::tolower(c);});
   if(ext!=".mp4"&&ext!=".mkv"&&ext!=".mov"&&ext!=".webm")continue;
   out.push_back({stableId(name),"本地",name,"",path+name});
  }closedir(dir);std::sort(out.begin(),out.end(),[](const Video&a,const Video&b){return a.title<b.title;});return out;
 }
 VideoPage listPage(int tab,int page){
  if(page>1)return {{},true};
  if(tab==Category::All)return {localVideos(),false};
  auto text=feed.empty()?readFile("sdmc:/switch/douyin-native/playlist.json"):request(feed);
  if(text.empty())return {{},true};
  auto videos=parseList(json::parse(text));return {videos,videos.empty()};
 }
 std::string detail(const Video& v){
  if(!media(v.url))throw std::runtime_error("无有效媒体地址，请重新导入播放列表");
  if(remote(v.url)){
   auto text=readFile("sdmc:/switch/douyin-native/playlist.json");
   if(!text.empty())try{for(const auto& x:parseList(json::parse(text)))if(x.id==v.id)return x.url;}catch(...){}
  }return v.url;
 }
};
