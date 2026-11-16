#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <cassert>
#include <cstdio>
#define LOG(...) ((void)0)
struct V{float x=0,y=0,z=0;};
static V operator+(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
static V operator*(V a,float b){return {a.x*b,a.y*b,a.z*b};}
static float length(V a){return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);}
static V norm(V a){float n=length(a);return n>0?a*(1/n):V{};}
static std::unordered_map<uint32_t,uint32_t> ram;
static uint32_t gameRead(uint32_t a){return ram[a];}
static void gameWrite(uint32_t a,uint32_t v){ram[a]=v;}
static bool gameAddress(uint32_t a){return a>=0x80000000&&a<0x81800000;}
static float gameFloat(uint32_t a){union {uint32_t u;float f;}v{gameRead(a)};return v.f;}
static void gameFloatWrite(uint32_t a,float f){union {float f;uint32_t u;}v{f};gameWrite(a,v.u);}
static std::atomic<float> playerEyeHeight{1.6f},submissionFov{112.f};
#include "../src/native_camera.h"
int main(){
 constexpr uint32_t manager=0x806247b0,player=0x80700000,transform=0x80710000,cam=0x80720000,rw=0x80730000,frame=0x80740000;
 gameWrite(0x80624204,player);gameWrite(player+0x18,transform);
 gameWrite(manager+0x258,cam);gameWrite(cam,0x80500000);
 gameWrite(manager+0x80,rw);gameWrite(rw+0xac,frame);gameFloatWrite(frame+0x44,1);
 gameWrite(manager+0x84,1);gameWrite(manager+0x1a0,0x01000012);
 gameWrite(cam+0xf0,23);gameFloatWrite(cam+0x9c,9);
 assert(freezeNativeCamera(true));gameWrite(manager+0x84,4);
 // Director reuses its zero path counters and adds unrelated flags.
 gameWrite(manager+0x1a0,0x00008034);gameFloatWrite(cam+0x80,99);
 assert(!freezeNativeCamera(false,true));
 assert(gameRead(cam+0xf0)==0); // must not restore pre-VR path count 23
 assert(gameFloat(cam+0x80)==99); // authored director target survives
 assert(gameFloat(cam+0x9c)==9); // unchanged VR position restored
 assert(gameRead(manager+0x1a0)==0x01008034);
 // Ordinary release restores the previous camera while preserving live flags.
 gameWrite(manager+0x84,1);assert(freezeNativeCamera(true));gameWrite(manager+0x84,4);
 gameWrite(manager+0x1a0,0x00004056);
 assert(!freezeNativeCamera(false));assert(gameRead(manager+0x28c)==1);
 assert(gameRead(manager+0x1a0)==0x01004056);
 // Another native camera has taken over: no stale camera-type request.
 gameWrite(manager+0x84,1);assert(freezeNativeCamera(true));
 gameWrite(manager+0x84,7);gameWrite(manager+0x28c,7);
 assert(!freezeNativeCamera(false));assert(gameRead(manager+0x28c)==7);
 puts("PASS: director path state, authored target, live flags and native camera handoff");
}
