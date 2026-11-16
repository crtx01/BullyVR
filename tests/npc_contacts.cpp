#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <unordered_map>
#include <cstdio>
#define LOG(...) ((void)0)
struct V{float x{},y{},z{};V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};}V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};}V operator*(float s)const{return{x*s,y*s,z*s};}};
float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}float length(V a){return std::sqrt(dot(a,a));}V norm(V a){return a*(1/std::max(length(a),1e-6f));}
struct Q{float x=0,y=0,z=0,w=1;};struct Pose{bool valid=true;float p[3]{};Q q;};struct Controller{bool connected=true;Pose grip,aim;};struct Snapshot{Controller c[2];Pose head;bool active=true;};V rotate(Q,V v){return v;}
std::unordered_map<uint32_t,uint32_t> ram;double timestamp=0;
double now(){return timestamp;}bool gameAddress(uint32_t a){return a>=0x80000000&&a<=0x93fffffc;}
uint32_t gameRead(uint32_t a){auto i=ram.find(a);return i==ram.end()?0:i->second;}void gameWrite(uint32_t a,uint32_t v){ram[a]=v;}float gameFloat(uint32_t a){auto u=gameRead(a);float f;memcpy(&f,&u,4);return f;}void gameFloatWrite(uint32_t a,float f){uint32_t u;memcpy(&u,&f,4);gameWrite(a,u);}
#include "../src/npc_contacts.h"
int main(){
 assert(pointSegmentDistance({0,0,0},{-1,0,0},{1,0,0})==0);
 assert(std::abs(pointSegmentDistance({0,1,0},{-1,0,0},{1,0,0})-1)<.0001f);
 assert(pointSegmentDistance({0,0,0},{1,0,0},{1,0,0})==1);
 constexpr uint32_t player=0x81500000,npc=player+0x2120,critical=player+0x4240,pool=0x80600000,flags=0x80601000,frame=0x80603000,rw=0x80602000;
 gameWrite(0x80624204,player);gameWrite(0x806eba48,pool);gameWrite(pool,player);gameWrite(pool+4,flags);gameWrite(pool+8,3);gameWrite(pool+12,0x2120);
 for(auto p:{player,npc,critical}){gameWrite(p,0x80550000);gameFloatWrite(p+0x1e64,75);gameFloatWrite(p+8,.60f);}
 gameWrite(critical+0x155c,1);gameWrite(0x80550044,0x80180a2c);
 gameWrite(0x806247b0+0x80,rw);gameWrite(rw+0xac,frame);gameFloatWrite(frame+0x20,1);gameFloatWrite(frame+0x44,1);
 // Player position remains at the origin; NPCs are 60cm ahead.
 gameFloatWrite(player+8,0);gameWrite(0x80001800,0xd01f1bb2);gameWrite(0x800021f0,0x00d0c0de);gameWrite(0x800021f4,0x00d0c0de);gameWrite(0x800021f8,0xf0000000);
 Snapshot s;s.head.p[1]=1.60f;for(auto& c:s.c){c.grip.p[1]=1.f;c.grip.p[2]=-.1f;}
 auto tick=[&](float z,bool enabled=true,float object=0){timestamp+=1./30;s.c[1].grip.p[2]=z;updateNpcContacts(s,0,enabled,object);};
 tick(-.1f);tick(-.1f);tick(-.5f);
 assert(contactMailbox&&gameRead(contactMailbox)==1);assert(gameRead(contactMailbox+4)==npc);assert(gameFloat(contactMailbox+12)==0);
 gameWrite(contactMailbox,0);tick(-.1f);tick(-.5f);assert(gameRead(contactMailbox)==0); // cooldown
 for(int i=0;i<35;i++)tick(-.5f);tick(-.1f);assert(gameRead(contactMailbox)==1);
 gameWrite(contactMailbox,0);gameWrite(npc+0x155c,1);for(int i=0;i<35;i++)tick(-.1f);tick(-.5f);assert(gameRead(contactMailbox)==0); // mission protection
 gameWrite(npc+0x155c,0);tick(-.1f,false);tick(-.5f,false);assert(gameRead(contactMailbox)==0);
 // An object tip reaches the NPC while the hand remains well outside the capsule.
 handsInitialized=false;contactHistory={};tick(-.01f,true,.5f);tick(-.01f,true,.5f);tick(-.10f,true,.5f);assert(gameRead(contactMailbox)==1);
 puts("PASS: swept contact, zero damage, cooldown, mission protection, disabled tracking and held-object tip");
}
