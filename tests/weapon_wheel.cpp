#include <cassert>
#include <cstdio>
#include <limits>
#include "../src/weapon_wheel.h"
int main(){
 std::array<int,weaponCount> seen{};
 for(int p=0;p<wheel::pageCount();p++){
  auto page=wheel::pageAt(p);assert(page.count>0&&page.count<=8);
  for(int slot=0;slot<page.count;slot++){
   int item=page.items[slot];assert(item>=0&&item<weaponCount);++seen[item];
   assert(weaponGroups[item]==page.group&&wheel::pageFor(item)==p);
   float angle=slot*6.283185307f/page.count;
   assert(wheel::hovered(page,std::sin(angle),std::cos(angle),-1)==item);
   assert(wheel::hovered(page,0,0,item)==item);
  }
  assert(wheel::hovered(page,0,0,-1)==page.items[0]);
  assert(wheel::hovered(page,std::numeric_limits<float>::quiet_NaN(),1,-1)==page.items[0]);
 }
 for(auto count:seen)assert(count==1);
 assert(wheel::wrapPage(-1)==wheel::pageCount()-1);
 assert(wheel::pageAt(wheel::pageCount()).items[0]==0);
 for(int id:{305,307,396,437}){bool found=false;for(int i=0;i<weaponCount;i++)if(weaponIds[i]==id){found=weaponGroups[i]==0;
  assert(!wheel::canEquip(i,0,10));
  if(id!=437){assert(wheel::ammoId(i)==(id==307?308:316));assert(!wheel::canEquip(i,1,0));assert(wheel::canEquip(i,1,1));}
  else{assert(wheel::ammoId(i)==-1);assert(wheel::canEquip(i,1,0));}
 }assert(found);}
 for(int i=0;i<weaponCount;i++)for(int j=i+1;j<weaponCount;j++)assert(weaponIds[i]!=weaponIds[j]);
 printf("PASS: %d items reachable exactly once; %d category pages; cardinal selection, deadzone, wrap, invalid input, launchers and skateboard\n",weaponCount,wheel::pageCount());
}
