#pragma once
struct CropUV { float left=0, top=0, right=1, bottom=1; };
// Fill a poster without changing the source image's proportions.
inline CropUV cropToFill(float sourceWidth,float sourceHeight,float width,float height){
 CropUV uv;
 if(sourceWidth<=0||sourceHeight<=0||width<=0||height<=0)return uv;
 float sourceAspect=sourceWidth/sourceHeight,targetAspect=width/height;
 if(sourceAspect>targetAspect){float span=targetAspect/sourceAspect;uv.left=(1-span)/2;uv.right=1-uv.left;}
 else {float span=sourceAspect/targetAspect;uv.top=(1-span)/2;uv.bottom=1-uv.top;}
 return uv;
}
