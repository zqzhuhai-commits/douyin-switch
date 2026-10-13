#include "douyin_sign.hpp"
#include <cassert>
int main(){
 assert(DouyinSign::hex(DouyinSign::sm3(DouyinSign::raw("abc")))=="66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0");
 assert(DouyinSign::aBogus("aid=6383&count=10","Mozilla/5.0 Chrome/151.0.0.0",1791904742000LL,[]{return .5;})=="O7UfhzU7dNmfCdMbuKr47rNUzy9MrPSyPTToSY1TSxuUL1tGZWNXTOaaJxq8szoy3bB0iF3HtnUMbDVcqsUkpK9pKmpDuNhWvGVnV60L0qw4TzkQLqbiewXFww0n0RGq-554i1W62UJo6jVAwqdD/BA9HKoF5c8BB3xfk/YbO9B6ZMgADpcePdtkEXNeUtcU");
 assert(DouyinSign::encode("hello 世界&+")=="hello%20%E4%B8%96%E7%95%8C%26%2B");
}
