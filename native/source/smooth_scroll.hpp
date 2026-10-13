#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
struct SmoothScroll {
 double offset=0,velocity=0,target=0,lastY=0;
 uint64_t lastMotion=0;
 bool dragging=false,animating=false;
 void reset(){offset=velocity=target=0;dragging=animating=false;}
 void jump(double value,double maximum){offset=target=std::clamp(value,0.0,maximum);velocity=0;animating=false;}
 void scrollTo(double value,double maximum){target=std::clamp(value,0.0,maximum);velocity=0;animating=true;}
 void begin(double y,uint64_t time){lastY=y;lastMotion=time;velocity=0;dragging=true;animating=false;}
 void move(double y,uint64_t time,double maximum){
  double delta=lastY-y,dt=std::max(0.008,(time-lastMotion)/1000.0);
  double next=std::clamp(offset+delta,0.0,maximum);
  velocity=next==offset?0:std::clamp(velocity*.55+delta/dt*.45,-2400.0,2400.0);
  offset=next;lastY=y;lastMotion=time;
 }
 void end(uint64_t time){dragging=false;if(time-lastMotion>100)velocity=0;}
 void update(double dt,double maximum){
  dt=std::clamp(dt,0.0,0.05);
  if(!dragging){
   if(animating){offset+=(target-offset)*(1-std::exp(-15*dt));if(std::abs(target-offset)<.3){offset=target;animating=false;}}
   else{offset+=velocity*(1-std::exp(-7*dt))/7;velocity*=std::exp(-7*dt);if(std::abs(velocity)<5)velocity=0;}
  }
  target=std::clamp(target,0.0,maximum);
  if(offset<0||offset>maximum){offset=std::clamp(offset,0.0,maximum);velocity=0;}
 }
};
