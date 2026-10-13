#include "feed_refresh.hpp"
#include <cassert>
int main(){
 int saves=0;auto cache=[]{return std::vector<int>{2};};auto builtin=[]{return std::vector<int>{3};};
 auto save=[&](const std::vector<int>&v){assert(v[0]==1);saves++;};
 auto ok=refreshFeed<int>([]{return std::vector<int>{1};},cache,builtin,save);
 assert(ok.videos[0]==1&&saves==1);
 auto empty=refreshFeed<int>([]{return std::vector<int>{};},cache,builtin,save);
 assert(empty.videos[0]==2&&saves==1);
 auto invalid=refreshFeed<int>([]()->std::vector<int>{throw std::runtime_error("invalid JSON");},[]{return std::vector<int>{};},builtin,save);
 assert(invalid.videos[0]==3&&saves==1);
 auto diskError=refreshFeed<int>([]{return std::vector<int>{1};},cache,builtin,[](const auto&){throw std::runtime_error("disk full");});
 assert(diskError.videos[0]==1&&diskError.note=="JSON 更新成功，缓存保存失败");
}
