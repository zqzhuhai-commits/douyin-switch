#pragma once
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <map>
#include <stdexcept>
#include <string>
#include "image_layout.hpp"
struct Tex { GLuint id=0; int w=1,h=1; };
class UI {
 GLuint program=0,vao=0,vbo=0; GLint tintLoc=-1,useLoc=-1;std::map<std::string,Tex> texts;bool clipped=false;int clipX=0,clipY=0,clipW=0,clipH=0;
 public: TTF_Font* font=nullptr;
 void clip(float x,float y,float w,float h){GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);clipped=true;clipX=viewport[0]+x*viewport[2]/1280;clipY=viewport[1]+(720-y-h)*viewport[3]/720;clipW=w*viewport[2]/1280;clipH=h*viewport[3]/720;}
 void clearClip(){clipped=false;glDisable(GL_SCISSOR_TEST);}
 void init(TTF_Font* f){font=f;const char* vs="#version 330 core\nlayout(location=0) in vec2 pos;layout(location=1) in vec2 uv;out vec2 v;void main(){gl_Position=vec4(pos,0,1);v=uv;}";
 const char* fs="#version 330 core\nin vec2 v;out vec4 o;uniform sampler2D image;uniform vec4 tint;uniform int textured;void main(){o=tint*(textured==1?texture(image,v):vec4(1));}";
 auto shader=[](GLenum kind,const char* s){GLuint a=glCreateShader(kind);glShaderSource(a,1,&s,nullptr);glCompileShader(a);GLint ok;glGetShaderiv(a,GL_COMPILE_STATUS,&ok);if(!ok)throw std::runtime_error("UI shader failed");return a;};
 auto a=shader(GL_VERTEX_SHADER,vs),b=shader(GL_FRAGMENT_SHADER,fs);program=glCreateProgram();glAttachShader(program,a);glAttachShader(program,b);glLinkProgram(program);glDeleteShader(a);glDeleteShader(b);GLint ok;glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)throw std::runtime_error("UI link failed");tintLoc=glGetUniformLocation(program,"tint");useLoc=glGetUniformLocation(program,"textured");glUseProgram(program);glUniform1i(glGetUniformLocation(program,"image"),0);
 glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);}
 Tex surface(SDL_Surface* s){if(!s)return {};auto* cv=SDL_ConvertSurfaceFormat(s,SDL_PIXELFORMAT_ABGR8888,0);if(!cv)return {};Tex t;t.w=cv->w;t.h=cv->h;glGenTextures(1,&t.id);glBindTexture(GL_TEXTURE_2D,t.id);glPixelStorei(GL_UNPACK_ALIGNMENT,4);glPixelStorei(GL_UNPACK_ROW_LENGTH,cv->pitch/4);glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA,t.w,t.h,0,GL_RGBA,GL_UNSIGNED_BYTE,cv->pixels);glPixelStorei(GL_UNPACK_ROW_LENGTH,0);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);SDL_FreeSurface(cv);return t;}
 Tex image(const std::string& data){if(data.empty())return {};SDL_Surface* s=IMG_Load_RW(SDL_RWFromConstMem(data.data(),data.size()),1);if(!s)return {};int w=std::min(s->w,512),h=std::max(1,s->h*w/s->w);auto* small=SDL_CreateRGBSurfaceWithFormat(0,w,h,32,SDL_PIXELFORMAT_ABGR8888);SDL_BlitScaled(s,nullptr,small,nullptr);auto t=surface(small);SDL_FreeSurface(small);SDL_FreeSurface(s);return t;}
 void rect(float x,float y,float w,float h,float r,float g,float b,float a=1,GLuint texture=0,float u0=0,float v0=0,float u1=1,float v1=1){
 float l=x/640-1,rr=(x+w)/640-1,top=1-y/360,bot=1-(y+h)/360;float v[]={l,top,u0,v0,l,bot,u0,v1,rr,bot,u1,v1,l,top,u0,v0,rr,bot,u1,v1,rr,top,u1,v0};
 glBindFramebuffer(GL_FRAMEBUFFER,0);glUseProgram(program);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STREAM_DRAW);glEnableVertexAttribArray(0);glEnableVertexAttribArray(1);glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)0);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,4*sizeof(float),(void*)(2*sizeof(float)));glDisable(GL_DEPTH_TEST);if(clipped){glEnable(GL_SCISSOR_TEST);glScissor(clipX,clipY,clipW,clipH);}else glDisable(GL_SCISSOR_TEST);glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,texture);glUniform4f(tintLoc,r,g,b,a);glUniform1i(useLoc,texture?1:0);glDrawArrays(GL_TRIANGLES,0,6);}
 void cover(float x,float y,float w,float h,const Tex& t){auto uv=cropToFill(t.w,t.h,w,h);rect(x,y,w,h,1,1,1,1,t.id,uv.left,uv.top,uv.right,uv.bottom);}
 void text(std::string s,float x,float y,float maxWidth=1150,float r=.92f,float g=.94f,float b=1){if(!font||s.empty())return;int w,h;TTF_SizeUTF8(font,s.c_str(),&w,&h);bool cut=false;while(w>maxWidth&&s.size()>3){size_t k=s.size()-1;while(k>0&&((unsigned char)s[k]&0xc0)==0x80)--k;s.resize(k);TTF_SizeUTF8(font,(s+"…").c_str(),&w,&h);cut=true;}if(cut)s+="…";auto it=texts.find(s);if(it==texts.end()){if(texts.size()>128){for(auto& a:texts)glDeleteTextures(1,&a.second.id);texts.clear();}auto* sf=TTF_RenderUTF8_Blended(font,s.c_str(),{255,255,255,255});auto t=surface(sf);SDL_FreeSurface(sf);it=texts.emplace(s,t).first;}const auto& t=it->second;rect(x,y,t.w,t.h,r,g,b,1,t.id);}
 void finish(){for(auto& a:texts)glDeleteTextures(1,&a.second.id);texts.clear();glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteProgram(program);}
};
