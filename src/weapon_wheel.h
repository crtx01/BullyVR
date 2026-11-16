#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include "weapon_catalog.h"

namespace wheel {
inline constexpr int slotsPerPage=8,groupCount=4;
inline constexpr const char* groupNames[]={"Equipamento","Arremessaveis","Corpo a corpo","Objetos"};
constexpr int ammoId(int item){
 if(item<0||item>=weaponCount)return -1;
 return weaponIds[item]==307?308:(weaponIds[item]==305||weaponIds[item]==396)?316:-1;
}
constexpr bool canEquip(int item,int owned,int ammo){return item==0||(item>0&&item<weaponCount&&owned>0&&(ammoId(item)<0||ammo>0));}
struct Page {std::array<int,slotsPerPage> items{};int count=0,group=0,number=0,pages=0;};
constexpr int groupSize(int group){int n=0;for(int i=0;i<weaponCount;i++)n+=weaponGroups[i]==group;return n;}
constexpr int pagesIn(int group){return (groupSize(group)+slotsPerPage-1)/slotsPerPage;}
constexpr int pageCount(){int n=0;for(int g=0;g<groupCount;g++)n+=pagesIn(g);return n;}
constexpr int wrapPage(int p){return (p%pageCount()+pageCount())%pageCount();}
constexpr Page pageAt(int p){
 p=wrapPage(p);Page out;
 for(int g=0;g<groupCount;g++){
  int pages=pagesIn(g);if(p>=pages){p-=pages;continue;}
  out.group=g;out.number=p;out.pages=pages;int rank=0;
  for(int i=0;i<weaponCount;i++)if(weaponGroups[i]==g){if(rank/slotsPerPage==p)out.items[out.count++]=i;++rank;}
  break;
 }return out;
}
constexpr int pageFor(int item){
 item=std::clamp(item,0,weaponCount-1);int p=0,rank=0;
 for(int g=0;g<weaponGroups[item];g++)p+=pagesIn(g);
 for(int i=0;i<item;i++)rank+=weaponGroups[i]==weaponGroups[item];
 return p+rank/slotsPerPage;
}
inline int hovered(const Page& page,float x,float y,int previous){
 if(!page.count)return 0;
 bool present=false;for(int i=0;i<page.count;i++)present|=page.items[i]==previous;
 if(!std::isfinite(x)||!std::isfinite(y)||std::hypot(x,y)<.38f)return present?previous:page.items[0];
 float a=std::atan2(x,y);if(a<0)a+=6.283185307f;
 return page.items[int(std::floor(a/6.283185307f*page.count+.5f))%page.count];
}
}
