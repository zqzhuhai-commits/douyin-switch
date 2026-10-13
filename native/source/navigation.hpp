#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
namespace Grid {
constexpr int columns=6, visibleRows=1;
constexpr float left=40, top=158, columnPitch=196;
constexpr float cardWidth=184, coverHeight=cardWidth*16.f/9.f;
constexpr float cardHeight=coverHeight+40, rowPitch=cardHeight+10;
constexpr float barLeft=1224, barWidth=14, viewportHeight=462, trackHeight=viewportHeight;
inline int rows(int count){return (std::max(0,count)+columns-1)/columns;}
inline int maxScroll(int count){return std::max(0,rows(count)-visibleRows);}
inline int clampScroll(int row,int count){return std::clamp(row,0,maxScroll(count));}
inline int followSelection(int selected,int scroll,int count){if(count==0)return 0;int r=selected/columns;if(r<scroll)scroll=r;if(r>=scroll+visibleRows)scroll=r-visibleRows+1;return clampScroll(scroll,count);}
inline int keepVisible(int selected,int scroll,int count){if(!count)return 0;return std::clamp(selected,scroll*columns,std::min(count-1,(scroll+visibleRows)*columns-1));}
inline int hit(float x,float y,int scroll,int count){if(x<left||y<top)return -1;int col=(x-left)/columnPitch,row=(y-top)/rowPitch;if(col>=columns||row>=visibleRows)return -1;if(x-left-col*columnPitch>=cardWidth||y-top-row*rowPitch>=cardHeight)return -1;int i=(scroll+row)*columns+col;return i<count?i:-1;}
inline float thumbHeight(int count){return rows(count)<=visibleRows?trackHeight:std::max(24.f,trackHeight*visibleRows/rows(count));}
inline float thumbTop(int scroll,int count){return maxScroll(count)?top+(trackHeight-thumbHeight(count))*clampScroll(scroll,count)/maxScroll(count):top;}
inline int barRow(float y,float offset,int count){float travel=trackHeight-thumbHeight(count);if(travel<=0)return 0;return clampScroll(std::lround((y-top-offset)/travel*maxScroll(count)),count);}
inline double contentHeight(int count){return count?rows(count)*rowPitch-10:0;}
inline double maxOffset(int count){return std::max(0.0,contentHeight(count)-viewportHeight);}
inline int firstRow(double offset){return std::max(0,(int)(offset/rowPitch));}
inline int endRow(double offset,int count){return std::min(rows(count),(int)std::ceil((offset+viewportHeight)/rowPitch));}
inline int hitAt(float x,float y,double offset,int count){
 if(x<left||y<top||y>=top+viewportHeight)return -1;
 double local=y-top+offset;int col=(x-left)/columnPitch,row=local/rowPitch;
 if(col>=columns||x-left-col*columnPitch>=cardWidth||local-row*rowPitch>=cardHeight)return -1;
 int i=row*columns+col;return i<count?i:-1;
}
inline double visibleOffset(int selected,double offset,int count){
 if(!count)return 0;
 double start=selected/columns*rowPitch,end=start+cardHeight;
 if(start<offset)offset=start;else if(end>offset+viewportHeight)offset=end-viewportHeight;
 return std::clamp(offset,0.0,maxOffset(count));
}
inline int keepVisibleAt(int selected,double offset,int count){return count?std::clamp(selected,firstRow(offset)*columns,std::min(count-1,endRow(offset,count)*columns-1)):0;}
inline double thumbSize(int count){return contentHeight(count)<=viewportHeight?trackHeight:std::max(24.0,trackHeight*viewportHeight/contentHeight(count));}
inline double thumbPosition(double offset,int count){return maxOffset(count)?top+(trackHeight-thumbSize(count))*std::clamp(offset,0.0,maxOffset(count))/maxOffset(count):top;}
inline double barPosition(float y,float grab,int count){double travel=trackHeight-thumbSize(count);return travel>0?std::clamp((y-top-grab)/travel*maxOffset(count),0.0,maxOffset(count)):0;}
}
inline std::optional<int> nextPlaybackIndex(int index,int count){if(index<0||index+1>=count)return {};return index+1;}
