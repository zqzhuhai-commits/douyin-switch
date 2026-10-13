#pragma once
#include <chrono>
#include <fstream>
#include <mutex>
#include <string>
inline std::mutex diagnosticMutex;
inline std::string redactMediaUrls(std::string s) {
 for (const char* prefix : {"https://", "http://"}) {
  size_t at=0;
  while((at=s.find(prefix,at))!=std::string::npos){size_t end=s.find_first_of(" \t\r\n\"'",at);if(end==std::string::npos)end=s.size();s.replace(at,end-at,"<media-url>");at+=11;}
 }
 if(s.size()>1500)s.resize(1500);
 return s;
}
inline void diagnostic(const std::string& stage,const std::string& message){
 std::lock_guard<std::mutex> lock(diagnosticMutex);
 std::ofstream f("sdmc:/switch/douyin-native/playback.log",std::ios::app);
 auto t=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
 f<<t<<" ["<<stage<<"] "<<redactMediaUrls(message)<<"\n";
}
