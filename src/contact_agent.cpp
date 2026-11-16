#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <cmath>
#include <array>
#include <atomic>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include "../upstream/Source/Core/Common/Functional.h"
#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"BullyContacts",__VA_ARGS__)
static uintptr_t base;
template<class F>static F fn(uintptr_t o){return (F)(base+o);}
struct V{float x{},y{},z{};V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};}V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};}V operator*(float s)const{return{x*s,y*s,z*s};}};
static float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}static float length(V a){return std::sqrt(dot(a,a));}static V norm(V a){return a*(1/std::max(length(a),1e-6f));}
struct Q{float x,y,z,w;};struct Pose{bool valid;float p[3];Q q;};struct Velocity{bool lv,av;float linear[3],angular[3];};struct Hit{bool valid;float u,v,d;};struct Controller{bool connected,primary,secondary,menu,trigger,squeeze,stick;float tv,sv,sf,x,y;Pose aim,grip;Velocity vel;Hit hit;};struct Snapshot{Controller c[2];Pose head;bool active;uint64_t generation;};
static double now(){timespec t{};clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
static bool gameAddress(uint32_t a){return (a>=0x80000000&&a<=0x817ffffc)||(a>=0x90000000&&a<=0x93fffffc);}
static uint32_t gameRead(uint32_t a){if(!gameAddress(a))return 0;void*m=fn<void*(*)(void*)>(0x6b3a84)((void*)(base+0xdec9c8));return fn<uint32_t(*)(void*,uint32_t)>(0x5bb354)(m,a);}
static void gameWrite(uint32_t a,uint32_t v){if(!gameAddress(a))return;void*m=fn<void*(*)(void*)>(0x6b3a84)((void*)(base+0xdec9c8));fn<void(*)(void*,uint32_t,uint32_t)>(0x5bb688)(m,v,a);}
static float gameFloat(uint32_t a){uint32_t u=gameRead(a);float v;memcpy(&v,&u,4);return v;}static void gameFloatWrite(uint32_t a,float v){uint32_t u;memcpy(&u,&v,4);gameWrite(a,u);}
static V cross(V a,V b){return{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static V rotate(Q q,V v){V u{q.x,q.y,q.z};return v+cross(u,cross(u,v)+v*q.w)*2;}
#include "npc_contacts.h"
static std::atomic<int> request{-1};
static void tick(){
 if(gameRead(0x80000000)!=0x52423745)return;
 installContactBridge();
 Snapshot input{};auto* mutex=(pthread_mutex_t*)(base+0xdec9e0);pthread_mutex_lock(mutex);memcpy(&input,(void*)(base+0xdd7f28),sizeof(input));pthread_mutex_unlock(mutex);
 bool cinema=(gameRead(0x806eab14)&255)||(gameRead(0x806ecbd8)&0xff000000u);
 updateNpcContacts(input,0,!cinema&&*(uint8_t*)(base+0x173915c));
 int mode=request.exchange(-1);
 if(mode<0)return;
 uint32_t player=gameRead(0x80624204);if(!validPed(player))return;
 if(mode>=300){unsigned command=mode>=2000?4:mode>=1000?3:2;int item=mode>=2000?mode-2000:mode>=1000?mode-1000:mode;bool ok=nativeRequest(command,player,0,{},item);LOG("Equipment native request %d queued=%d",mode,ok);return;}
 uint32_t pool=gameRead(0x806eba48),objects=gameRead(pool),flags=gameRead(pool+4),cap=gameRead(pool+8),stride=gameRead(pool+12);
 V origin=pedPosition(player);float closest=1000;uint32_t selected=0;
 for(uint32_t i=1;i<std::min(cap,128u);i++){
  if((gameRead(flags+(i&~3u))>>(24-(i&3)*8))&0x80)continue;
  uint32_t ped=objects+i*stride;if(!validPed(ped)||gameRead(ped+0x155c)||!(gameFloat(ped+0x1e64)>0))continue;
  float d=length(pedPosition(ped)-origin);if(d<closest){selected=ped;closest=d;}
 }
 if(selected){
 uint32_t rw=gameRead(0x806247b0+0x80),frame=gameRead(rw+0xac);
 V forward=norm(V{gameFloat(frame+0x40),gameFloat(frame+0x44),0}),right=norm(V{gameFloat(frame+0x20),gameFloat(frame+0x24),0});
 V relative=pedPosition(selected)+V{0,0,1.f}-(origin+V{0,0,1.60f});
 LOG("NPC relative headset-space %.3f %.3f %.3f",dot(right,relative),relative.z,-dot(forward,relative));
 LOG("Closest NPC %08x distance=%.2f health=%.2f collision=%u",selected,closest,gameFloat(selected+0x1e64),gameRead(selected+0x64));if(mode==1)LOG("Reaction queued=%d",nativeRequest(1,selected,0,{1,0,0}));}
}
static void* worker(void*){for(int i=0;i<60000;i++){usleep(33333);fn<void(*)(void*,Common::MoveOnlyFunction<void()>)>(0x518bb4)((void*)(base+0xdec9c8),[]{tick();});}return nullptr;}
extern "C" __attribute__((visibility("default"))) int Agent_OnAttach(JavaVM*,char* options,void*){
 static bool started=false;if(!base){void*m=dlopen("libmain.so",RTLD_NOW);Dl_info d{};dladdr(dlsym(m,"Java_org_dolphinemu_dolphinemu_NativeLibrary_RequestOpenXRRecenter"),&d);base=(uintptr_t)d.dli_fbase;}
 int mode=0;if(options)sscanf(options,"%d",&mode);
 if(mode==-3){void*m=dlopen("libmain.so",RTLD_NOW);auto resume=(void(*)(JNIEnv*,jclass))dlsym(m,"Java_org_dolphinemu_dolphinemu_NativeLibrary_UnPauseEmulation");if(resume)resume(nullptr,nullptr);LOG("Resume invoked");}
 request=mode;
 if(!started){started=true;pthread_t thread;pthread_create(&thread,nullptr,worker,nullptr);pthread_detach(thread);}return 0;
}
