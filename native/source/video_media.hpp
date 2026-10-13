#pragma once
#include <json.hpp>
#include <limits>
#include <string>
inline std::string httpMediaUrl(const nlohmann::json& address){
 if(!address.is_object()||!address.contains("url_list")||!address["url_list"].is_array())return {};
 for(const auto& value:address["url_list"]){
  if(!value.is_string())continue;
  auto url=value.get<std::string>();
  if(url.rfind("https://",0)==0||url.rfind("http://",0)==0)return url;
 }
 return {};
}
inline std::string switchVideoUrl(const nlohmann::json& video){
 std::string best;int64_t bitrate=std::numeric_limits<int64_t>::max();
 if(video.contains("bit_rate")&&video["bit_rate"].is_array())for(const auto& rate:video["bit_rate"]){
  if(!rate.is_object()||rate.value("is_h265",1)!=0||rate.value("format",std::string())!="mp4"||!rate.contains("play_addr"))continue;
  auto url=httpMediaUrl(rate["play_addr"]);
  auto current=rate.value("bit_rate",std::numeric_limits<int64_t>::max());
  if(!url.empty()&&(best.empty()||current<bitrate)){best=url;bitrate=current;}
 }
 if(!best.empty())return best;
 if(video.contains("play_addr_h264")){best=httpMediaUrl(video["play_addr_h264"]);if(!best.empty())return best;}
 if(video.contains("play_addr"))return httpMediaUrl(video["play_addr"]);
 return {};
}
