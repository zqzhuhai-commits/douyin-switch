#pragma once
#include <stdexcept>
#include <string>
#include <vector>
template<class Video> struct FeedRefreshResult {std::vector<Video> videos;std::string note;};
template<class Video,class Fetch,class Cache,class Builtin,class Save>
FeedRefreshResult<Video> refreshFeed(Fetch fetch,Cache cache,Builtin builtin,Save save){
 try{
  auto videos=fetch();
  if(videos.empty())throw std::runtime_error("remote playlist is empty");
  std::string note="启动更新成功（公开示例源）";
  try{save(videos);}catch(...){note="JSON 更新成功，缓存保存失败";}
  return {std::move(videos),note};
 }catch(...){
  auto videos=cache();
  if(!videos.empty())return {std::move(videos),"更新失败，使用上次缓存"};
  return {builtin(),"更新失败，使用内置示例"};
 }
}
