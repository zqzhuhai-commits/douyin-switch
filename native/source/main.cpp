#include <switch.h>
#include <SDL2/SDL.h>
#include <mpv/client.h>
#include <mpv/render_gl.h>
extern "C" {
#include <libavformat/avio.h>
}
#include <future>
#include <chrono>
#include <map>
#include <set>
#include <sys/stat.h>
#include <cmath>
#include "api.hpp"
#include "glui.hpp"
#include "navigation.hpp"
#include "catalog.hpp"
#include "smooth_scroll.hpp"
#include "gesture.hpp"
#include <memory>
#include <deque>
#include <cerrno>
#include "playback_recovery.hpp"
static const std::string root="sdmc:/switch/douyin-native/";
static uint64_t now(){return SDL_GetTicks64();}
static void* proc(void*,const char* name){return SDL_GL_GetProcAddress(name);}
static json encode(const std::vector<Video>& vs){json j=json::array();for(auto& v:vs)j.push_back({{"mv_id",v.id},{"mu_id",v.user},{"mv_title",v.title},{"mv_img_url",v.cover},{"mv_play_url",v.url},{"is_cat_ads",0}});return {{"list",j}};}
static void save(const std::string& p,const std::string& s){std::ofstream f(p,std::ios::binary);if(!f)throw std::runtime_error("Cannot save SD data");f.write(s.data(),s.size());}
static std::vector<Video> load(const std::string& p){try{return Api::parseList(json::parse(readFile(p)));}catch(...){return {};}}
static std::string clockText(double n){if(!std::isfinite(n)||n<0)n=0;int a=n;char s[32];snprintf(s,sizeof(s),"%02d:%02d:%02d",a/3600,(a/60)%60,a%60);return s;}
int main(int argc,char** argv){
 romfsInit();mkdir("sdmc:/switch",0777);mkdir("sdmc:/switch/douyin-native",0777);mkdir((root+"cache").c_str(),0777);mkdir((root+"videos").c_str(),0777);
 std::ofstream(root+"playback.log",std::ios::trunc)<<"DouyinNativeSwitch 0.3.0\n";
 diagnostic("startup","starting");
 socketInitializeDefault();sslInitialize(4);curl_global_init(CURL_GLOBAL_DEFAULT);plInitialize(PlServiceType_User);
 diagnostic("network-backend",curl_version());
 SDL_Window* window=nullptr;SDL_GLContext context=nullptr;TTF_Font* font=nullptr;mpv_handle* mp=nullptr;mpv_render_context* render=nullptr;
 int result=0;std::string fatal;
 try {
  if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_EVENTS)!=0){throw std::runtime_error(SDL_GetError());}
  IMG_Init(IMG_INIT_JPG|IMG_INIT_PNG);TTF_Init();
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,3);SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
  window=SDL_CreateWindow("Video Grid",0,0,1280,720,SDL_WINDOW_OPENGL|SDL_WINDOW_FULLSCREEN);if(!window)throw std::runtime_error(SDL_GetError());context=SDL_GL_CreateContext(window);if(!context||!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))throw std::runtime_error("OpenGL unavailable");SDL_GL_SetSwapInterval(1);
  PlFontData fd;if(R_SUCCEEDED(plGetSharedFontByType(&fd,PlSharedFontType_ChineseSimplified)))font=TTF_OpenFontRW(SDL_RWFromConstMem(fd.address,fd.size),1,22);if(!font)throw std::runtime_error("System font unavailable");
  const char* httpsProtocol=avio_find_protocol_name("https://example.com/test.mp4");diagnostic("protocol",httpsProtocol?httpsProtocol:"HTTPS missing");if(!httpsProtocol){throw std::runtime_error("Player HTTPS protocol missing");}
  UI ui;ui.init(font);Api api;try{auto cfg=json::parse(readFile(root+"config.json"));auto s=cfg.value("feed_url",std::string());if(s.rfind("https://",0)==0)api.feed=s;}catch(...){}
  mp=mpv_create();if(!mp)throw std::runtime_error("Player init failed");
  mpv_set_option_string(mp,"referrer","https://www.douyin.com/");
  mpv_set_option_string(mp,"user-agent",Api::userAgent);
  for(auto kv:std::initializer_list<std::pair<const char*,const char*>>{{"config","no"},{"terminal","no"},{"vo","libmpv"},{"hwdec","auto"},{"cache","yes"},{"demuxer-max-bytes","64MiB"},{"demuxer-max-back-bytes","8MiB"},{"network-timeout","20"},{"tls-verify","yes"},{"tls-ca-file","romfs:/cacert.pem"},{"audio-display","no"}})mpv_set_option_string(mp,kv.first,kv.second);
  mpv_request_log_messages(mp,"warn");
  if(mpv_initialize(mp)<0){throw std::runtime_error("Player startup failed");}
  mpv_opengl_init_params gl={proc,nullptr};const char* gltype=MPV_RENDER_API_TYPE_OPENGL;mpv_render_param params[]={{MPV_RENDER_PARAM_API_TYPE,(void*)gltype},{MPV_RENDER_PARAM_OPENGL_INIT_PARAMS,&gl},{MPV_RENDER_PARAM_INVALID,nullptr}};if(mpv_render_context_create(&render,mp,params)<0)throw std::runtime_error("Video renderer unavailable");
  padConfigureInput(1,HidNpadStyleSet_NpadStandard);PadState pad;padInitializeDefault(&pad);hidInitializeTouchScreen();
  std::vector<Video> items=load(root+"live-hot.json"),favorites=load(root+"favorites.json");std::map<std::string,Tex> covers;
  int tab=Category::Hot,selected=0;SmoothScroll scroll;Catalog catalog;int catalogGeneration=0,listGeneration=0,listPage=1;uint64_t loadAt=0,lastFrame=now();std::string catalogError,refreshNote;std::shared_ptr<Api> listApi;bool loading=false,playing=false,menu=false,paused=false,quitting=false;PlaybackRecovery recovery;bool playbackFailed=false,fileLoaded=false;double position=0,duration=0,volume=100,resumeAfterDetail=0;uint64_t controlsUntil=0,repeatAt=0;u64 navHeld=0;std::string status="正在更新列表…",playerStatus,activeUrl,lastPlayerError;Video active;
  bool autoplay=true,portrait=false,advanceQueued=false,advanceManual=false,waitingForCatalog=false,waitAutomatic=true,awaitingStart=true;int64_t playbackEntry=-1;int queueIndex=-1,queueTab=0;std::vector<Video> queue;std::set<std::string> queuedIds;
  bool barDragging=false,gridTouch=false,touchMoved=false;float barOffset=0,touchX=0,touchY=0;int touchItem=-1;
  try{auto prefs=json::parse(readFile(root+"player-settings.json"));autoplay=prefs.value("autoplay",true);portrait=prefs.value("portrait",false);}catch(...){}
  std::future<VideoPage> listJob;std::future<std::pair<std::string,std::string>> imageJob;std::string pendingCover;std::set<std::string> failedCovers;std::future<std::string> detailJob;bool detailPending=false,loadAfterDetail=false;
  std::future<void> cacheJob;std::deque<std::pair<std::string,std::vector<Video>>> cacheQueue;
  auto cacheCatalog=[&]{if(tab==Category::Favorites||items.empty())return;auto path=root+Category::cacheFile(tab);
   cacheQueue.erase(std::remove_if(cacheQueue.begin(),cacheQueue.end(),[&](const auto& entry){return entry.first==path;}),cacheQueue.end());cacheQueue.emplace_back(path,items);
  };
  auto pumpCache=[&](bool wait=false){
   if(cacheJob.valid()&&(wait||cacheJob.wait_for(std::chrono::seconds(0))==std::future_status::ready)){try{cacheJob.get();}catch(const std::exception& e){diagnostic("catalog-cache",e.what());}}
   if(!cacheJob.valid()&&!cacheQueue.empty()){auto entry=std::move(cacheQueue.front());cacheQueue.pop_front();cacheJob=std::async(std::launch::async,[entry=std::move(entry)]{auto temporary=entry.first+".tmp";save(temporary,encode(entry.second).dump());if(rename(temporary.c_str(),entry.first.c_str())!=0){if(errno!=EEXIST||remove(entry.first.c_str())!=0||rename(temporary.c_str(),entry.first.c_str())!=0)throw std::runtime_error("Cannot replace catalog cache");}});}
  };
  auto catalogStatus=[&]{auto count=std::to_string(items.size());
   if(tab==Category::Favorites)return std::string("收藏保存在本机，共 ")+count+" 个视频";
   if(!catalogError.empty())return "加载中断，已保留 "+count+" 个视频："+catalogError+"；Y 继续";
   if(catalog.done)return refreshNote+"，共 "+count+" 个视频";
   return refreshNote.empty()?"正在请求实时推荐…":refreshNote+"，共 "+count+" 个视频";
  };
  auto fetch=[&]{if(loading||tab==Category::Favorites||catalog.done||!catalogError.empty())return;
   loading=true;listPage=catalog.nextPage;listGeneration=catalogGeneration;int t=tab,p=listPage;
   listApi=std::make_shared<Api>();listApi->base=api.base;listApi->feed=api.feed;for(const auto&v:items)listApi->seen.push_back(v.id);auto worker=listApi;status=catalogStatus();
   listJob=std::async(std::launch::async,[worker,t,p]{return worker->listPage(t,p);});
  };
  auto resetCatalog=[&]{catalogGeneration++;if(listApi)listApi->cancel=true;catalog.reset();catalogError.clear();loadAt=0;status=catalogStatus();fetch();};
  auto switchTab=[&](int t){cacheCatalog();tab=(t+3)%3;selected=0;scroll.reset();failedCovers.clear();
   if(tab==Category::Favorites)items=favorites;
   else{items=load(root+Category::cacheFile(tab));}
   resetCatalog();
  };fetch();
  bool playTouch=false,playSeeking=false,tapControls=false;float playTouchX=0,playTouchY=0;
  auto command=[&](std::initializer_list<const char*> args){std::vector<const char*> a(args);a.push_back(nullptr);int rc=mpv_command_async(mp,0,a.data());if(rc<0)diagnostic("command",mpv_error_string(rc));return rc;};
  auto resolve=[&](double resume){
   activeUrl.clear();awaitingStart=true;playbackEntry=-1;fileLoaded=false;playbackFailed=false;
   resumeAfterDetail=resume;loadAfterDetail=true;detailPending=true;lastPlayerError.clear();
   Video v=active;playerStatus="正在读取媒体地址…";
   detailJob=std::async(std::launch::async,[&api,v]{return api.detail(v);});
  };
  auto failed=[&](const std::string& reason,bool retryable=true){
   diagnostic("play",reason);activeUrl.clear();fileLoaded=false;awaitingStart=true;
   if(retryable&&recovery.schedule(now(),position)){playerStatus="加载失败，重新解析 "+std::to_string(recovery.attempts)+"/2";}
   else{recovery.cancel();playbackFailed=true;playerStatus="播放失败："+redactMediaUrls(reason)+"；A 重试，B 返回";}
   controlsUntil=now()+86400000;
  };
  auto start=[&](const Video& v){if(detailPending){status="正在解析，请稍后再试";return;}
   command({"stop"});active=v;diagnostic("play","request video="+v.id);playing=true;paused=false;waitingForCatalog=false;
   recovery.reset();position=duration=0;menu=false;mpv_set_property_string(mp,"pause","no");resolve(0);
  };
  auto stop=[&]{command({"stop"});playing=false;menu=false;recovery.cancel();activeUrl.clear();fileLoaded=false;playbackFailed=false;advanceQueued=false;advanceManual=false;waitingForCatalog=false;awaitingStart=true;playerStatus.clear();};
  auto savePlayerSettings=[&]{try{save(root+"player-settings.json",json({{"autoplay",autoplay},{"portrait",portrait}}).dump());}catch(const std::exception& e){diagnostic("settings",e.what());}};
  auto applyOrientation=[&]{int64_t rotation=portrait?90:0;int rc=mpv_set_property(mp,"video-rotate",MPV_FORMAT_INT64,&rotation);if(rc<0)diagnostic("orientation",mpv_error_string(rc));controlsUntil=now()+5000;};applyOrientation();
  auto toggleOrientation=[&]{portrait=!portrait;applyOrientation();savePlayerSettings();};
  auto toggleAutoplay=[&]{autoplay=!autoplay;if(!advanceManual)advanceQueued=false;if(waitAutomatic)waitingForCatalog=false;savePlayerSettings();controlsUntil=now()+5000;};
  auto followActive=[&]{auto it=std::find_if(items.begin(),items.end(),[&](const Video& v){return v.id==active.id;});if(it!=items.end()){selected=it-items.begin();scroll.scrollTo(Grid::visibleOffset(selected,scroll.offset,items.size()),Grid::maxOffset(items.size()));}};
  auto beginSelection=[&](int index){if(detailPending){status="正在完成上一次请求，请稍候";return;}if(index<0||index>=(int)items.size())return;
   selected=index;queue=items;queueIndex=index;queueTab=tab;queuedIds.clear();for(auto& v:queue)queuedIds.insert(v.id);advanceQueued=waitingForCatalog=false;scroll.velocity=0;start(queue[queueIndex]);
  };
  auto advance=[&](bool automatic){if(detailPending)return;
   if(auto index=nextPlaybackIndex(queueIndex,queue.size())){queueIndex=*index;waitingForCatalog=false;start(queue[queueIndex]);followActive();return;}
   if(queueTab==Category::Favorites||catalog.done){waitingForCatalog=false;playerStatus="列表已播完：A 重播，B 返回";controlsUntil=now()+86400000;return;}
   waitingForCatalog=true;waitAutomatic=automatic;playerStatus="正在等待更多视频加载…";controlsUntil=now()+86400000;
   if(!catalogError.empty()){catalogError.clear();loadAt=0;}fetch();
  };
  auto seek=[&](double seconds,bool absolute=false){auto n=std::to_string(seconds);command({"seek",n.c_str(),absolute?"absolute":"relative","exact"});controlsUntil=now()+4000;};
  while(appletMainLoop()&&!quitting){
   pumpCache();
   auto frameTime=now();scroll.update((frameTime-lastFrame)/1000.0,Grid::maxOffset(items.size()));lastFrame=frameTime;if(!playing&&!scroll.animating)selected=Grid::keepVisibleAt(selected,scroll.offset,items.size());
   padUpdate(&pad);u64 down=padGetButtonsDown(&pad),held=padGetButtons(&pad);u64 dirs=HidNpadButton_AnyLeft|HidNpadButton_AnyRight|HidNpadButton_AnyUp|HidNpadButton_AnyDown;
   u64 nav=held&dirs;if(nav&&nav==navHeld&&now()>repeatAt){down|=nav;repeatAt=now()+130;}else if(nav!=navHeld){navHeld=nav;repeatAt=now()+420;}
   if(loading&&listJob.wait_for(std::chrono::seconds(0))==std::future_status::ready){
    loading=false;
    try{auto result=listJob.get();if(listGeneration==catalogGeneration&&tab<Category::Favorites){
     auto added=catalog.accept(listPage,result.videos,result.rawEmpty);if(!result.note.empty())refreshNote=result.note;
     if(listPage==1){items.clear();selected=0;scroll.reset();gridTouch=barDragging=false;touchMoved=true;}
     items.insert(items.end(),added.begin(),added.end());
     if(playing&&queueTab==tab){for(const auto& v:added)if(queuedIds.insert(v.id).second)queue.push_back(v);}
     status=catalogStatus();loadAt=now()+2000;
     cacheCatalog();
     if(waitingForCatalog){if(nextPlaybackIndex(queueIndex,queue.size())){waitingForCatalog=false;advanceQueued=true;advanceManual=!waitAutomatic;}
      else if(catalog.done){waitingForCatalog=false;playerStatus="列表已播完：A 重播，B 返回";controlsUntil=now()+86400000;}}
    }}catch(const std::exception& e){if(listGeneration==catalogGeneration){diagnostic("catalog",e.what());catalogError=e.what();status=catalogStatus();cacheCatalog();if(waitingForCatalog){waitingForCatalog=false;playerStatus="列表加载失败："+catalogError+"；R 重试，B 返回";controlsUntil=now()+86400000;}}}
    listApi.reset();
   }
   if(!loading&&tab<Category::Favorites&&!catalog.done&&catalogError.empty()&&now()>=loadAt&&
    (tab==Category::All||items.empty()||waitingForCatalog||(playing&&queueTab==tab&&queueIndex+5>=(int)queue.size())||(!playing&&Grid::endRow(scroll.offset,items.size())+1>=Grid::rows(items.size()))))fetch();
   if(imageJob.valid()&&imageJob.wait_for(std::chrono::seconds(0))==std::future_status::ready){auto p=imageJob.get();auto tex=ui.image(p.second);if(tex.id)covers[p.first]=tex;else failedCovers.insert(p.first);pendingCover.clear();}
   if(!playing&&!imageJob.valid())for(int i=Grid::firstRow(scroll.offset)*Grid::columns;i<std::min((int)items.size(),Grid::endRow(scroll.offset,items.size())*Grid::columns);i++){auto v=items[i];if(v.cover.empty())continue;if(covers.count(v.id)||failedCovers.count(v.id))continue;auto bytes=readFile(root+"cache/"+v.id+".jpg");if(bytes.empty())bytes=readFile("romfs:/covers/"+v.id+".jpg");if(!bytes.empty()){auto tex=ui.image(bytes);if(tex.id){covers[v.id]=tex;continue;}}pendingCover=v.id;imageJob=std::async(std::launch::async,[&api,v]{try{auto b=api.request(v.cover);save(root+"cache/"+v.id+".jpg",b);return std::make_pair(v.id,b);}catch(...){return std::make_pair(v.id,std::string());}});break;}
   if(covers.size()>48){std::set<std::string> visible;int first=std::max(0,Grid::firstRow(scroll.offset)-2)*Grid::columns,last=std::min((int)items.size(),(Grid::endRow(scroll.offset,items.size())+2)*Grid::columns);for(int i=first;i<last;i++)visible.insert(items[i].id);for(auto it=covers.begin();it!=covers.end();)if(!visible.count(it->first)){glDeleteTextures(1,&it->second.id);it=covers.erase(it);}else ++it;}
   if(detailPending&&detailJob.wait_for(std::chrono::seconds(0))==std::future_status::ready){
    detailPending=false;
    try{auto u=detailJob.get();if(playing&&loadAfterDetail){
     if(u.empty())failed("无可用播放地址",false);
     else{activeUrl=u;auto resume=std::to_string(resumeAfterDetail);mpv_set_property_string(mp,"start",resume.c_str());
      if(command({"loadfile",activeUrl.c_str(),"replace"})<0)failed("无法提交播放命令",false);else playerStatus="正在加载视频…";}
    }}catch(const NetworkError& e){if(playing)failed(e.what());}
    catch(const std::exception& e){if(playing)failed(e.what(),false);}
    loadAfterDetail=false;
   }
   while(auto* e=mpv_wait_event(mp,0)){
    if(e->event_id==MPV_EVENT_NONE)break;
    if(e->event_id==MPV_EVENT_LOG_MESSAGE){auto* log=(mpv_event_log_message*)e->data;auto message=std::string(log->prefix)+": "+log->text;diagnostic("mpv",message);
     if(playing&&(message.find("TLS ")!=std::string::npos||message.find("sslConnection")!=std::string::npos||message.find("HTTP error")!=std::string::npos))lastPlayerError=redactMediaUrls(message);
    }
    if(e->event_id==MPV_EVENT_COMMAND_REPLY&&e->error<0)diagnostic("command-result",mpv_error_string(e->error));
    if(e->event_id==MPV_EVENT_FILE_LOADED&&playing&&!awaitingStart){diagnostic("play","file loaded");fileLoaded=true;playbackFailed=false;applyOrientation();playerStatus.clear();}
    if(e->event_id==MPV_EVENT_START_FILE&&!detailPending&&!activeUrl.empty()){playbackEntry=((mpv_event_start_file*)e->data)->playlist_entry_id;awaitingStart=false;}
    if(e->event_id==MPV_EVENT_END_FILE&&playing){auto* ef=(mpv_event_end_file*)e->data;if(awaitingStart||ef->playlist_entry_id!=playbackEntry)continue;
     if(ef->reason==MPV_END_FILE_REASON_ERROR)failed(lastPlayerError.empty()?mpv_error_string(ef->error):lastPlayerError);
     else if(ef->reason==MPV_END_FILE_REASON_EOF){fileLoaded=false;playbackFailed=true;if(autoplay){advanceQueued=true;advanceManual=false;playerStatus="正在切换下一条…";}else{playbackFailed=true;playerStatus="播放结束：A 重播，B 返回";controlsUntil=now()+86400000;}}
    }
   }
   if(advanceQueued&&!detailPending){advanceQueued=false;if(playing)advance(!advanceManual);advanceManual=false;}
   if(playing){double p=0,d=0;if(mpv_get_property(mp,"time-pos",MPV_FORMAT_DOUBLE,&p)>=0)position=p;if(mpv_get_property(mp,"duration",MPV_FORMAT_DOUBLE,&d)>=0)duration=d;if(!detailPending&&recovery.due(now())){auto resume=recovery.resume;recovery.cancel();resolve(resume);}}
   if(down&HidNpadButton_Plus){menu=!menu;controlsUntil=now()+6000;}
   if(menu){if(down&HidNpadButton_B)menu=false;if(down&HidNpadButton_A){if(playing)stop();else quitting=true;}}
   else if(playing){if(down&HidNpadButton_B)stop();if(down&HidNpadButton_Y)toggleOrientation();if(down&HidNpadButton_X)toggleAutoplay();if((down&HidNpadButton_R)&&!detailPending){advanceQueued=true;advanceManual=true;}if((down&HidNpadButton_L)&&queueIndex>0&&!detailPending){queueIndex--;waitingForCatalog=false;start(queue[queueIndex]);followActive();}if(down&HidNpadButton_A){if(!detailPending&&!recovery.pending()){if(playbackFailed){start(active);}else if(fileLoaded){paused=!paused;mpv_set_property_string(mp,"pause",paused?"yes":"no");}}controlsUntil=now()+4000;}if((down&HidNpadButton_AnyLeft)&&!activeUrl.empty())seek(-10);if((down&HidNpadButton_AnyRight)&&!activeUrl.empty())seek(10);if(down&(HidNpadButton_AnyUp|HidNpadButton_AnyDown)){volume=std::clamp(volume+((down&HidNpadButton_AnyUp)?5:-5),0.0,100.0);mpv_set_property(mp,"volume",MPV_FORMAT_DOUBLE,&volume);controlsUntil=now()+4000;}}
   else {
    if((down&HidNpadButton_Minus)&&!loading){
     SwkbdConfig keyboard;Result kr=swkbdCreate(&keyboard,0);
     if(R_SUCCEEDED(kr)){
      swkbdConfigMakePresetDefault(&keyboard);swkbdConfigSetHeaderText(&keyboard,"添加视频直链");
      swkbdConfigSetGuideText(&keyboard,"输入 MP4 / M3U8 媒体地址，不能输入抖音网页链接");
      swkbdConfigSetStringLenMax(&keyboard,2047);char address[2048]={};
      kr=swkbdShow(&keyboard,address,sizeof(address));swkbdClose(&keyboard);
      if(R_SUCCEEDED(kr)&&address[0]){
       std::string url=address;
       if(Api::remote(url))try{
        auto imported=load(root+"playlist.json");auto id=Api::stableId(url);
        if(std::none_of(imported.begin(),imported.end(),[&](const Video&v){return v.id==id;}))imported.push_back({id,"","手动添加视频","",url});
        save(root+"playlist.json",encode(imported).dump());switchTab(Category::Hot);
       }catch(const std::exception&e){status=e.what();}
       else status="请输入 http/https 视频直链";
      }
     }
    }
    if(down&HidNpadButton_L){switchTab(tab-1);}if(down&HidNpadButton_R){switchTab(tab+1);}if(down&HidNpadButton_Y){failedCovers.clear();if(tab<2){if(!catalogError.empty()){catalogError.clear();loadAt=0;fetch();}else resetCatalog();}}if(down&HidNpadButton_ZR)scroll.scrollTo(scroll.offset+Grid::viewportHeight*.85,Grid::maxOffset(items.size()));if(down&HidNpadButton_ZL)scroll.scrollTo(scroll.offset-Grid::viewportHeight*.85,Grid::maxOffset(items.size()));if(!items.empty()){if(down&HidNpadButton_AnyLeft)selected=std::max(0,selected-1);if(down&HidNpadButton_AnyRight)selected=std::min((int)items.size()-1,selected+1);if(down&HidNpadButton_AnyUp)selected=std::max(0,selected-Grid::columns);if(down&HidNpadButton_AnyDown)selected=std::min((int)items.size()-1,selected+Grid::columns);if(down&dirs)scroll.scrollTo(Grid::visibleOffset(selected,scroll.offset,items.size()),Grid::maxOffset(items.size()));if(down&HidNpadButton_A)beginSelection(selected);if(down&HidNpadButton_X){auto v=items[selected];auto it=std::find_if(favorites.begin(),favorites.end(),[&](const Video& f){return f.id==v.id;});if(it==favorites.end())favorites.push_back(v);else favorites.erase(it);try{save(root+"favorites.json",encode(favorites).dump());status="收藏已更新";}catch(...){status="收藏保存失败";}if(tab==2){items=favorites;selected=std::max(0,std::min(selected,(int)items.size()-1));scroll.scrollTo(Grid::visibleOffset(selected,scroll.offset,items.size()),Grid::maxOffset(items.size()));}}}}
   SDL_Event event;
   while(SDL_PollEvent(&event)){
    if(event.type==SDL_QUIT)quitting=true;
    if(event.type==SDL_MOUSEWHEEL&&!playing){scroll.scrollTo((scroll.animating?scroll.target:scroll.offset)-event.wheel.y*100,Grid::maxOffset(items.size()));}
    if(event.type==SDL_FINGERDOWN){float x=event.tfinger.x*1280,y=event.tfinger.y*720;barDragging=gridTouch=false;touchMoved=false;
     if(menu){menu=false;continue;}
     if(playing){
      playTouch=true;playSeeking=false;playTouchX=x;playTouchY=y;tapControls=now()<controlsUntil||paused||!playerStatus.empty();
      if(tapControls&&x>=800&&x<980&&y>=602&&y<642){toggleOrientation();playTouch=false;}
      else if(tapControls&&x>=1000&&x<1240&&y>=602&&y<642){toggleAutoplay();playTouch=false;}
      else if(tapControls&&y>640&&duration>0){playSeeking=true;seek(std::clamp((x-40)/1200,0.0f,1.0f)*duration,true);controlsUntil=now()+5000;}
     }
     else if(x>=Grid::barLeft-8&&x<Grid::barLeft+Grid::barWidth+8&&y>=Grid::top&&y<=Grid::top+Grid::trackHeight){barDragging=true;float thumb=Grid::thumbPosition(scroll.offset,items.size()),height=Grid::thumbSize(items.size());barOffset=(y>=thumb&&y<=thumb+height)?y-thumb:height/2;scroll.jump(Grid::barPosition(y,barOffset,items.size()),Grid::maxOffset(items.size()));}
     else if(y>=85&&y<140&&x>=40&&x<1240){switchTab(std::clamp((int)((x-40)/400),0,2));}
     else if(y>=Grid::top&&y<Grid::top+Grid::viewportHeight){touchItem=Grid::hitAt(x,y,scroll.offset,items.size());gridTouch=true;touchX=x;touchY=y;scroll.begin(y,now());if(touchItem>=0)selected=touchItem;}
    }
    if(event.type==SDL_FINGERMOTION){float x=event.tfinger.x*1280,y=event.tfinger.y*720;if(playing){if(playTouch&&playSeeking&&duration>0){seek(std::clamp((x-40)/1200,0.0f,1.0f)*duration,true);controlsUntil=now()+5000;}}else if(barDragging){scroll.jump(Grid::barPosition(y,barOffset,items.size()),Grid::maxOffset(items.size()));}else if(gridTouch){if(std::abs(y-touchY)>12||std::abs(x-touchX)>12)touchMoved=true;if(touchMoved){scroll.move(y,now(),Grid::maxOffset(items.size()));}}}
    if(event.type==SDL_FINGERUP){float x=event.tfinger.x*1280,y=event.tfinger.y*720;
     if(playing&&playTouch&&!playSeeking){auto gesture=playerGesture(x-playTouchX,y-playTouchY);
      if(gesture==PlayerGesture::Next&&!detailPending){advanceQueued=true;advanceManual=true;}
      else if(gesture==PlayerGesture::Previous&&!detailPending&&queueIndex>0){queueIndex--;waitingForCatalog=false;start(queue[queueIndex]);followActive();}
      else if(gesture==PlayerGesture::Tap){
       if(tapControls&&playbackFailed&&!detailPending&&!recovery.pending())start(active);
       else if(tapControls&&fileLoaded){paused=!paused;mpv_set_property_string(mp,"pause",paused?"yes":"no");}
       controlsUntil=now()+5000;
      }
     }playTouch=playSeeking=false;
     if(!playing&&gridTouch&&!touchMoved&&touchItem>=0&&Grid::hitAt(x,y,scroll.offset,items.size())==touchItem){beginSelection(touchItem);}scroll.end(now());gridTouch=barDragging=false;}
   }
   int w,h;SDL_GL_GetDrawableSize(window,&w,&h);glViewport(0,0,w,h);glClearColor(.055,.065,.09,1);glClear(GL_COLOR_BUFFER_BIT);
   if(playing){mpv_opengl_fbo fbo={0,w,h,0};int flip=1;mpv_render_param rp[]={{MPV_RENDER_PARAM_OPENGL_FBO,&fbo},{MPV_RENDER_PARAM_FLIP_Y,&flip},{MPV_RENDER_PARAM_INVALID,nullptr}};if(!activeUrl.empty()){mpv_render_context_render(render,rp);}glViewport(0,0,w,h);if(now()<controlsUntil||paused||!playerStatus.empty()||menu){ui.rect(0,0,1280,60,0,0,0,.7);ui.text(active.title,30,16,1210);ui.rect(0,598,1280,122,0,0,0,.8);ui.text(playerStatus.empty()?(paused?"已暂停":"正在播放"):playerStatus,40,605,730);ui.rect(800,602,180,38,.15,.23,.34);ui.text(portrait?"Y 恢复方向":"Y 横竖切换",812,609,156);ui.rect(1000,602,240,38,autoplay?.12:.16,autoplay?.32:.20,autoplay?.22:.25);ui.text(autoplay?"X 连播：开":"X 连播：关",1012,609,216);ui.rect(40,650,1200,7,.3,.33,.4);if(duration>0)ui.rect(40,650,1200*std::clamp(position/duration,0.0,1.0),7,.15,.68,1);ui.text(clockText(position)+" / "+clockText(duration)+"   音量 "+std::to_string((int)volume)+"%",40,670,670);ui.text("A 暂停  B 返回  L/R 切换  上下滑切换",680,670,560);}}
   else {ui.text("抖音原生播放器",40,28,500);ui.text(std::to_string(items.size())+" 个视频",1030,28,210,.5,.72,1);for(int i=0;i<3;i++){ui.rect(40+i*400,85,384,54,i==tab?.19:.11,i==tab?.34:.13,i==tab?.52:.18);ui.text(Category::names[i],60+i*400,100,340);}ui.clip(Grid::left-3,Grid::top-3,Grid::barLeft-Grid::left-5,Grid::viewportHeight+3);for(int i=Grid::firstRow(scroll.offset)*Grid::columns;i<Grid::endRow(scroll.offset,items.size())*Grid::columns;i++){if(i>=(int)items.size())break;float x=Grid::left+(i%Grid::columns)*Grid::columnPitch,y=Grid::top+(i/Grid::columns)*Grid::rowPitch-scroll.offset;ui.rect(x-3,y-3,Grid::cardWidth+6,Grid::cardHeight+6,i==selected?.2:.09,i==selected?.68:.11,i==selected?1:.16);auto it=covers.find(items[i].id);if(it!=covers.end())ui.cover(x,y,Grid::cardWidth,Grid::coverHeight,it->second);else{ui.rect(x,y,Grid::cardWidth,Grid::coverHeight,.12,.16,.23);ui.text("原生视频",x+22,y+(Grid::coverHeight-28)/2,140);}ui.rect(x,y+Grid::coverHeight,Grid::cardWidth,40,.1,.12,.16);ui.text(items[i].title,x+8,y+Grid::coverHeight+8,Grid::cardWidth-16);}ui.clearClip();ui.rect(Grid::barLeft,Grid::top,Grid::barWidth,Grid::trackHeight,.12,.16,.23);ui.rect(Grid::barLeft,Grid::thumbPosition(scroll.offset,items.size()),Grid::barWidth,Grid::thumbSize(items.size()),.25,.58,.85);if(items.empty())ui.text(loading?"正在加载视频列表…":"复制访客配置后按 Y 加载实时推荐，或切换本地视频",350,340,800);ui.text(status,40,654,1150,.6,.7,.82);ui.text("A 播放  X 收藏  L/R 分类  Y 刷新  - 添加直链  + 菜单",40,686,1200,.7,.8,.94);}
   if(menu){ui.rect(0,0,1280,720,0,0,0,.6);ui.rect(330,250,620,180,.12,.16,.23);ui.text(playing?"播放菜单":"退出应用",365,278,550);ui.text(playing?"A 返回视频列表":"A 退出应用",365,332,550);ui.text("B 取消",365,377,550);}
   SDL_GL_SwapWindow(window);if(playing)mpv_render_context_report_swap(render);
  }
  api.cancel=true;if(listApi)listApi->cancel=true;cacheCatalog();if(listJob.valid())listJob.wait();if(imageJob.valid())imageJob.wait();if(detailJob.valid())detailJob.wait();while(cacheJob.valid()||!cacheQueue.empty())pumpCache(true);for(auto& x:covers)glDeleteTextures(1,&x.second.id);mpv_render_context_free(render);render=nullptr;mpv_terminate_destroy(mp);mp=nullptr;ui.finish();
 }catch(const std::exception& e){fatal=e.what();std::ofstream(root+"error.log")<<fatal<<"\n";result=1;}
 if(render){mpv_render_context_free(render);}
 if(mp){mpv_terminate_destroy(mp);}
 if(font){TTF_CloseFont(font);}
 TTF_Quit();IMG_Quit();if(context)SDL_GL_DeleteContext(context);if(window)SDL_DestroyWindow(window);SDL_Quit();if(result){consoleInit(nullptr);printf("Video Grid startup failed:\n%s\n\nLog: /switch/douyin-native/error.log\nPress B or + to exit.\n",fatal.c_str());padConfigureInput(1,HidNpadStyleSet_NpadStandard);PadState p;padInitializeDefault(&p);while(appletMainLoop()){padUpdate(&p);if(padGetButtonsDown(&p)&(HidNpadButton_B|HidNpadButton_Plus))break;consoleUpdate(nullptr);}consoleExit(nullptr);}
 plExit();curl_global_cleanup();sslExit();socketExit();romfsExit();return result;
}
