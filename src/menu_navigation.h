#pragma once
#include <cmath>
struct MenuNavigation {
 int held=0;double repeatAt=0;
 void reset(){held=0;repeatAt=0;}
 int step(float axis,double time){
  if(!std::isfinite(axis)||std::abs(axis)<.28f){reset();return 0;}
  int direction=axis>.55f?-1:axis<-.55f?1:0;
  if(!direction)return 0;
  if(direction!=held){held=direction;repeatAt=time+.38;return direction;}
  if(time>=repeatAt){repeatAt=time+.16;return direction;}
  return 0;
 }
};
