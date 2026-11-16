#include "../src/equipment_control.h"
#include <cassert>
#include <array>
#include <cstdio>
int main(){
 EquipmentButtons buttons;
 assert(buttons.step(false,false,0,true)==0);
 assert(buttons.step(false,true,1,true)==0);
 assert(buttons.step(false,false,1.1,true)==1);
 buttons.step(false,true,2,true);
 assert(buttons.step(false,true,2.61,true)==2);
 assert(buttons.step(false,true,2.9,true)==0);
 assert(buttons.step(false,false,3,true)==0);
 buttons.step(true,false,4,true);
 assert(buttons.step(false,false,4.1,true)==-1);
 buttons.step(true,true,5,true);buttons.step(false,true,5.3,true);
 assert(buttons.step(false,true,6,true)==0);
 assert(buttons.step(false,false,6.1,true)==0);
 buttons.step(false,true,7,false);
 assert(buttons.step(false,true,8,true)==0);
 assert(buttons.step(false,false,8.2,true)==0);
 std::array<bool,5> owned{true,true,false,true,true};
 auto available=[&](int i){return owned[i];};
 assert(nextOwnedEquipment(4,1,5,available)==0);
 assert(nextOwnedEquipment(0,-1,5,available)==4);
 assert(nextOwnedEquipment(1,1,5,available)==3);
 TriggerRelease trigger;
 assert(!trigger.step(303,true,true).down); // already held when equipping
 trigger.step(303,false,true);
 assert(trigger.step(303,true,true).down);
 assert(trigger.step(303,true,true).down);
 assert(trigger.step(303,false,true).released);
 assert(!trigger.step(303,false,true).released);
 trigger.step(303,true,false);
 assert(!trigger.step(303,true,true).down); // menu cannot leave it held
 trigger.step(303,false,true);
 assert(trigger.step(303,true,true).down);
 puts("PASS: grip cycle, long hold without intermediate equip, chord suppression, fists wrap, trigger release and menu/equip cancellation");
}
