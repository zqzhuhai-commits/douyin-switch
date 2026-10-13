#include "gesture.hpp"
#include <cassert>
int main(){
 assert(playerGesture(10,-120)==PlayerGesture::Next);
 assert(playerGesture(5,100)==PlayerGesture::Previous);
 assert(playerGesture(100,100)==PlayerGesture::None);
 assert(playerGesture(0,-30)==PlayerGesture::None);
 assert(playerGesture(3,4)==PlayerGesture::Tap);
}
