#pragma once
#include <cmath>
enum class PlayerGesture {None, Tap, Previous, Next};
inline PlayerGesture playerGesture(float dx,float dy){
 if(std::abs(dy)>=80 && std::abs(dy)>std::abs(dx)*1.3f)
  return dy<0?PlayerGesture::Next:PlayerGesture::Previous;
 if(std::abs(dx)<16 && std::abs(dy)<16)return PlayerGesture::Tap;
 return PlayerGesture::None;
}
