#pragma once
// The native weapon needs a released trigger frame to throw/fire before its
// manual camera is dismissed. Exiting on that same frame cancels the attack.
struct AimMode {
 bool held=false,active=false;double enterUntil=0,releaseAt=0,exitUntil=0;
 struct State {bool manual=false,enter=false,exit=false;};
 State step(bool desired,double time,bool allowed){
  if(!allowed){*this={};return {};}
  if(desired&&!held){if(!active)enterUntil=time+.12;active=true;releaseAt=exitUntil=0;}
  if(!desired&&held)releaseAt=time+.30;
  held=desired;
  if(!desired&&releaseAt>0&&time>=releaseAt&&exitUntil==0)exitUntil=time+.12;
  if(exitUntil>0&&time>=exitUntil){active=false;releaseAt=exitUntil=0;}
  return {active,desired&&time<enterUntil,!desired&&exitUntil>time};
 }
};
