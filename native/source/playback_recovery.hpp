#pragma once
#include <cmath>
#include <cstdint>
struct PlaybackRecovery {
 int attempts=0;
 uint64_t at=0;
 double resume=0;
 void reset(){attempts=0;cancel();resume=0;}
 void cancel(){at=0;}
 bool pending()const{return at!=0;}
 bool due(uint64_t now)const{return pending()&&now>=at;}
 bool schedule(uint64_t now,double position){
  cancel();if(attempts>=2)return false;
  ++attempts;resume=std::isfinite(position)&&position>0?position:0;
  at=now+1000*attempts;return true;
 }
};
