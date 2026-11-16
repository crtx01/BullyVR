#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
struct HudQuadSample{float x,y;uint32_t color;};
inline bool isDarkHudPanel(const HudQuadSample* vertices,unsigned count,float px,float py){
 if(count<4||count>6||!std::isfinite(px)||!std::isfinite(py))return false;
 float lx=1e9f,ly=1e9f,hx=-1e9f,hy=-1e9f;uint32_t first=vertices[0].color;
 for(unsigned i=0;i<count;i++){
  const auto& v=vertices[i];if(!std::isfinite(v.x)||!std::isfinite(v.y)||(v.color>>24)==0)return false;
  for(unsigned shift:{0u,8u,16u})if(((v.color>>shift)&255)>48)return false;
  for(unsigned shift:{0u,8u,16u,24u})if(std::abs(int((v.color>>shift)&255)-int((first>>shift)&255))>3)return false;
  lx=std::min(lx,v.x);hx=std::max(hx,v.x);ly=std::min(ly,v.y);hy=std::max(hy,v.y);
 }
 return std::abs((hx-lx)*px)>.30f&&std::abs((hy-ly)*py)>.08f;
}
