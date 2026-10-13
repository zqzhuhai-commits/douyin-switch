#pragma once
#include <curl/curl.h>
#include "diagnostics.hpp"
#include "categories.hpp"
#include "feed_refresh.hpp"
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
 static constexpr const char* userAgent="Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36";
 std::string base="https://www.douyin.com",feed="https://raw.githubusercontent.com/zyronon/douyin/master/src/assets/data/posts6.json";
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
    if(video.contains("play_addr"))url=firstUrl(video.at("play_addr"));
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
 VideoPage listPage(int tab,int page){
  if(page>1)return {{},true,{}};
  if(tab==Category::All){auto v=localVideos();return {v,v.empty(),"本地视频"};}
  auto updated=refreshFeed<Video>([&]{
    try{return parseList(json::parse(request(feed)));}catch(const std::exception&e){diagnostic("feed-refresh",e.what());throw;}
   },[]{auto v=readPlaylist("sdmc:/switch/douyin-native/feed-cache.json");if(v.empty())v=readPlaylist("sdmc:/switch/douyin-native/feed-cache.json.bak");return v;},
   []{return readPlaylist("romfs:/builtin.json");},[](const auto& videos){
    const std::string cache="sdmc:/switch/douyin-native/feed-cache.json",tmp=cache+".tmp";
    std::ofstream f(tmp,std::ios::binary);auto data=encodeList(videos).dump();f.write(data.data(),data.size());f.close();
    if(!f)throw std::runtime_error("cache write failed");
    if(std::rename(tmp.c_str(),cache.c_str())!=0){
     if(errno!=EEXIST)throw std::runtime_error("cache rename failed");
     const auto backup=cache+".bak";
     std::remove(backup.c_str());
     if(std::rename(cache.c_str(),backup.c_str())!=0)throw std::runtime_error("cache backup failed");
     if(std::rename(tmp.c_str(),cache.c_str())!=0){std::rename(backup.c_str(),cache.c_str());throw std::runtime_error("cache replace failed");}
     std::remove(backup.c_str());
    }
   });
  auto videos=std::move(updated.videos);auto note=updated.note;
  for(const auto&v:readPlaylist("sdmc:/switch/douyin-native/playlist.json")){
   auto it=std::find_if(videos.begin(),videos.end(),[&](const Video& x){return x.id==v.id;});
   if(it==videos.end())videos.push_back(v);else *it=v;
  }
  return {videos,videos.empty(),note};
 }

 std::string detail(const Video& v){
  if(!media(v.url))throw std::runtime_error("无有效媒体地址，请重新导入播放列表");
  if(remote(v.url))for(const char* path:{"sdmc:/switch/douyin-native/playlist.json","sdmc:/switch/douyin-native/feed-cache.json"}){
   for(const auto&x:readPlaylist(path))if(x.id==v.id)return x.url;
  }
  return v.url;
 }
};
