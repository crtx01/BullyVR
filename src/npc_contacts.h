// Bounded contact tests and native reactions. All access runs on Dolphin CPU thread.
#include "contact_bridge_data.h"
static uint32_t contactMailbox=0;
static unsigned contactSerial=0;
static bool validPed(uint32_t ped){
 if(!gameAddress(ped)||!gameAddress(ped+0x211c))return false;
 uint32_t vtable=gameRead(ped);return vtable>=0x80500000&&vtable<0x80614000;
}
static bool installContactBridge(){
 // Rebuilding cheats can replace our C0 header with the end marker while
 // leaving its old payload and mailbox intact. Magic alone is not sufficient.
 if(contactMailbox&&gameRead(contactMailbox+52)==0x42565243&&
    gameRead(contactMailbox-contactMailboxOffset-8)==0xc0000000u&&
    gameRead(contactMailbox-contactMailboxOffset-4)==sizeof(contactBridgeWords)/8)return true;
 contactMailbox=0;
 if(gameRead(0x80001800)-0xd01f1badu>5)return false;
 uint32_t start=0;for(uint32_t p=0x80001810;p<0x80002ff0;p+=4)if(gameRead(p)==0x00d0c0de&&gameRead(p+4)==0x00d0c0de){start=p+8;break;}
 if(!start)return false;
 // Only append at the verified end of this small, fixed game-specific codelist.
 uint32_t end=0,firstOwn=0;bool onlyOwnTail=true;for(uint32_t p=start;p<0x80002ff0;p+=8){
  if(gameRead(p)==0xc0000000u){uint32_t lines=gameRead(p+4);if(lines>400||p+8+lines*8>0x80002ff8)return false;
   bool own=lines*8>=500&&lines*8<=3000&&gameRead(p+8)==0x9421fe00&&(gameRead(p+8+lines*8-8)==0x42565243||gameRead(p+8+lines*8-4)==0x42565243);
   if(own&&!firstOwn)firstOwn=p;
   if(firstOwn&&!own)onlyOwnTail=false;
   if(lines*8==sizeof(contactBridgeWords)){
    bool matches=true;for(unsigned i=0;i<contactMailboxOffset/4;i++)if(gameRead(p+8+i*4)!=contactBridgeWords[i]){matches=false;break;}
    if(matches&&gameRead(p+8+contactMailboxOffset+52)==0x42565243){contactMailbox=p+8+contactMailboxOffset;contactSerial=gameRead(contactMailbox+32);gameWrite(contactMailbox,0);LOG("Native contact bridge reused mailbox=%08x",contactMailbox);return true;}
   }
   p+=lines*8;continue;}
  if(gameRead(p)==0xf0000000u&&gameRead(p+4)==0){end=p;break;}
  if(firstOwn)onlyOwnTail=false;
 }
 // Old test bridges are always appended after the user's original cheat list.
 // Compact only a tail consisting entirely of our verified bridge blocks.
 if(end&&firstOwn&&onlyOwnTail)end=firstOwn;
 constexpr uint32_t size=sizeof(contactBridgeWords);
 if(!end||end+8+size+8>0x80002ff8)return false;
 for(unsigned i=0;i<size/4;i++)gameWrite(end+8+i*4,contactBridgeWords[i]);
 gameWrite(end+8+size,0xf0000000);gameWrite(end+12+size,0);
 gameWrite(end+4,size/8);gameWrite(end,0xc0000000);
 contactMailbox=end+8+contactMailboxOffset;
 // Dolphin's own Gecko HLE resets the JIT cache for its first five frames.
 gameWrite(0x80001800,0xd01f1bad);
 LOG("Native contact bridge installed mailbox=%08x bytes=%u",contactMailbox,size);
 return true;
}
static bool nativeRequest(unsigned command,uint32_t ped,float damage,V direction,int weapon=-1){
 if(command==1){uint32_t callback=gameRead(gameRead(ped)+0x44);if(callback<0x80004000||callback>=0x80500000)return false;}
 if(!validPed(ped)||!installContactBridge()||gameRead(contactMailbox)!=0)return false;
 gameWrite(contactMailbox+4,ped);gameWrite(contactMailbox+8,gameRead(ped));
 gameFloatWrite(contactMailbox+12,damage);gameFloatWrite(contactMailbox+16,direction.x);gameFloatWrite(contactMailbox+20,direction.y);gameFloatWrite(contactMailbox+24,direction.z);
 gameWrite(contactMailbox+28,uint32_t(weapon));gameWrite(contactMailbox+32,++contactSerial);gameWrite(contactMailbox+36,0);gameWrite(contactMailbox,command);
 return true;
}
static V pedPosition(uint32_t ped){uint32_t transform=gameRead(ped+0x18);uint32_t p=gameAddress(transform)?transform+0x30:ped+4;return {gameFloat(p),gameFloat(p+4),gameFloat(p+8)};}
static bool finitePosition(V p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::abs(p.x)<10000&&std::abs(p.y)<10000&&std::abs(p.z)<10000;}
static float pointSegmentDistance(V p,V a,V b){V ab=b-a;float t=std::clamp(dot(p-a,ab)/std::max(dot(ab,ab),.000001f),0.f,1.f);return length(p-(a+ab*t));}
struct ContactHistory {uint32_t ped=0,vtable=0;double last=0;};
static std::array<ContactHistory,32> contactHistory;
static std::array<V,4> previousHands;static bool handsInitialized=false;static double contactTime=0;
static void updateNpcContacts(const Snapshot& input,float turn,bool enabled,float objectLength=0.f){
 double t=now();float dt=float(t-contactTime);contactTime=t;
 if(!enabled||!input.active||!input.head.valid||dt<=0||dt>.2f){handsInitialized=false;return;}
 uint32_t player=gameRead(0x80624204),pool=gameRead(0x806eba48);
 if(!validPed(player)||!gameAddress(pool)){handsInitialized=false;return;}
 uint32_t objects=gameRead(pool),flags=gameRead(pool+4),capacity=gameRead(pool+8),stride=gameRead(pool+12);
 if(!gameAddress(objects)||!gameAddress(flags)||capacity>128||stride!=0x2120)return;
 uint32_t rw=gameRead(0x806247b0+0x80),frame=gameRead(rw+0xac);if(!gameAddress(frame))return;
 V forward=norm(V{gameFloat(frame+0x40),gameFloat(frame+0x44),0}),right=norm(V{gameFloat(frame+0x20),gameFloat(frame+0x24),0});
 if(length(forward)<.9f||length(right)<.9f)return;
 V eye=pedPosition(player)+V{0,0,1.60f};if(!finitePosition(eye))return;
 float c=std::cos(turn),sn=std::sin(turn);V points[4];bool tracked[4]{};
 for(int h=0;h<2;h++){
  const auto& hand=input.c[h];tracked[h]=hand.connected&&hand.grip.valid;
  V relative{hand.grip.p[0]-input.head.p[0],hand.grip.p[1]-input.head.p[1],hand.grip.p[2]-input.head.p[2]};
  points[h]=eye+right*(c*relative.x-sn*relative.z)+forward*(-sn*relative.x-c*relative.z)+V{0,0,relative.y};
  tracked[h]&=finitePosition(points[h])&&length(relative)<2.5f;
 }
 for(int h=0;h<2;h++){
  points[h+2]=points[h];tracked[h+2]=tracked[h]&&h==1&&objectLength>.01f;
  if(tracked[h+2]){
   V local=rotate(input.c[h].aim.valid?input.c[h].aim.q:input.c[h].grip.q,{0,0,-objectLength});
   points[h+2]=points[h]+right*(c*local.x-sn*local.z)+forward*(-sn*local.x-c*local.z)+V{0,0,local.y};
  }
 }
 if(!handsInitialized){for(int h=0;h<4;h++)previousHands[h]=points[h];handsInitialized=true;return;}
 bool sent=false;
 for(uint32_t i=0;i<capacity&&!sent;i++){
  uint32_t packed=gameRead(flags+(i&~3u));if((packed>>(24-(i&3)*8))&0x80)continue;
  uint32_t ped=objects+i*stride;if(ped==player||!validPed(ped))continue;
  // Mission-critical and dead peds retain native behavior.
  if(gameRead(ped+0x155c)||!(gameFloat(ped+0x1e64)>0))continue;
  V center=pedPosition(ped)+V{0,0,.95f};if(!finitePosition(center)||length(center-eye)>3.f)continue;
  auto& history=contactHistory[i%contactHistory.size()];uint32_t vtable=gameRead(ped);
  if(history.ped==ped&&history.vtable==vtable&&t-history.last<.9)continue;
  for(int h=0;h<4;h++)if(tracked[h]){
   float travel=length(points[h]-previousHands[h]);if(travel<.002f||travel>.8f)continue;
   V nearest{center.x,center.y,std::clamp(points[h].z,center.z-.65f,center.z+.55f)};
   if(pointSegmentDistance(nearest,previousHands[h],points[h])>.34f)continue;
   float speed=travel/dt;
   // The original engine chooses a hit reaction; a light touch never subtracts health.
   if(nativeRequest(1,ped,0.f,norm(points[h]-previousHands[h]))){history={ped,vtable,t};sent=true;LOG("VR hand contact ped=%08x probe=%d speed=%.2f serial=%u",ped,h,speed,contactSerial);break;}
  }
 }
 for(int h=0;h<4;h++)previousHands[h]=points[h];
}
