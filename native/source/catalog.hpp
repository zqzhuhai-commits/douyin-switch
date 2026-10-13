#pragma once
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>
// Track website pagination internally while exposing one continuous catalog.
struct Catalog {
 int nextPage=1,pagesLoaded=0,noNewPages=0;
 bool done=false,exhausted=false;
 std::unordered_set<std::string> ids;
 void reset(){nextPage=1;pagesLoaded=noNewPages=0;done=exhausted=false;ids.clear();}
 template<class Video> std::vector<Video> accept(int page,const std::vector<Video>& videos,bool rawEmpty){
  if(done||page!=nextPage)throw std::logic_error("Unexpected catalog page");
  std::vector<Video> added;
  for(const auto& video:videos)if(!video.id.empty()&&ids.insert(video.id).second)added.push_back(video);
  ++pagesLoaded;++nextPage;
  noNewPages=added.empty()?noNewPages+1:0;
  exhausted=rawEmpty;done=exhausted||noNewPages>=3;
  return added;
 }
};
