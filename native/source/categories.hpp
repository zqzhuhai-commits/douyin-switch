#pragma once
namespace Category {
constexpr int Hot=0, All=1, Favorites=2;
constexpr const char* names[]={"公开视频","本地视频","收藏"};
inline const char* listPath(int tab){return tab==Hot?"/api/videos/listHot":"/api/videos/listAll";}
inline const char* cacheFile(int tab){return tab==Hot?"hot.json":"all.json";}
}
