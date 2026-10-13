#pragma once
namespace Category {
constexpr int Hot=0, All=1, Favorites=2;
constexpr const char* names[]={"实时推荐","本地/导入","收藏"};
inline const char* listPath(int tab){return tab==Hot?"/api/videos/listHot":"/api/videos/listAll";}
inline const char* cacheFile(int tab){return tab==Hot?"live-hot.json":"live-all.json";}
}
