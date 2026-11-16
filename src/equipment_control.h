#pragma once
#include <algorithm>
#include <cmath>

// Grips are inventory controls only. A long right hold must never first
// equip another weapon, and overlapping grips must not send Wii +/-.
struct EquipmentButtons {
 bool left=false,right=false,chord=false,longSent=false,armed=false;
 double leftAt=0,rightAt=0;
 int step(bool l,bool r,double time,bool allowed){
  if(!allowed){left=l;right=r;chord=false;longSent=false;armed=false;return 0;}
  if(!armed){if(!l&&!r)armed=true;left=l;right=r;return 0;}
  if(l&&!left)leftAt=time;
  if(r&&!right){rightAt=time;longSent=false;}
  if(l&&r)chord=true;
  int action=0;
  if(r&&!l&&!chord&&!longSent&&time-rightAt>=.60){action=2;longSent=true;}
  if(!chord){
   if(!l&&left&&time-leftAt>=.035)action=-1;
   if(!r&&right&&!longSent&&time-rightAt>=.035)action=1;
  }
  if(!l&&!r){chord=false;longSent=false;}
  left=l;right=r;return action;
 }
};

template<class Available>
int nextOwnedEquipment(int current,int direction,int count,Available available){
 if(count<=0)return 0;
 current=std::clamp(current,0,count-1);
 for(int distance=1;distance<=count;distance++){
  int item=(current+direction*distance+count*2)%count;
  if(item==0||available(item))return item;
 }
 return 0;
}

// Native ranged attacks use the Wii B press AND release. Keep the input
// released across equipment changes and menu closure until the user lets go.
struct TriggerRelease {
 bool armed=false,held=false;
 int item=-2;
 struct State{bool down=false,released=false;};
 State step(int equipped,bool pressed,bool allowed){
  if(!allowed||equipped!=item){item=equipped;armed=false;held=false;}
  if(!allowed)return {};
  if(!pressed){bool release=armed&&held;armed=true;held=false;return {false,release};}
  if(!armed)return {};
  held=true;return {true,false};
 }
};
