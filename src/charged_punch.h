#pragma once
#include <algorithm>
#include <cmath>

// A deliberate wind-up behind the shoulder maps to the Wii's B + motion
// heavy attack. Ordinary forward punches retain the original motion detector.
struct ChargedPunch {
 struct Hand {bool ready=false,winding=false;float last=0,back=0;double began=0,lastAt=0;};
 Hand hands[2]{};double heavyUntil=0,cooldownUntil=0;int firedHand=-1;
 struct Result {bool attack=false;bool suppress[2]{};bool punch[2]{};};
 Result step(const float forward[2],const bool tracked[2],double time,bool enabled){
  if(!enabled){*this={};return {};}
  Result out;
  for(int h=0;h<2;h++){
   auto& hand=hands[h];float f=forward[h];double dt=time-hand.lastAt;
   if(!tracked[h]||!std::isfinite(f)){hand={};continue;}
   if(!hand.ready||dt<=0||dt>.2){hand={true,false,f,f,0,time};continue;}
   float speed=(f-hand.last)/float(dt);hand.last=f;hand.lastAt=time;
   if(!hand.winding&&time>=cooldownUntil&&f<-.08f){hand.winding=true;hand.back=f;hand.began=time;}
   if(hand.winding){
    hand.back=std::min(hand.back,f);out.suppress[h]=true;
    if(time-hand.began>1.5){hand.winding=false;cooldownUntil=time+.25;}
    else if(time-hand.began>=.14&&f-hand.back>.30f&&speed>1.35f){
     firedHand=h;heavyUntil=time+.14;cooldownUntil=time+.55;hand.winding=false;
    }else out.attack=true;
   }
  }
  if(time<heavyUntil&&firedHand>=0){out.attack=true;out.punch[firedHand]=true;}
  return out;
 }
};
