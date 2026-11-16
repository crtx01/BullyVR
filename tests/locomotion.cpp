#include "../src/locomotion.h"
#include <cassert>
#include <cstdio>
using namespace locomotion;
static bool near(float a,float b){return std::abs(a-b)<.0001f;}
int main(){
 assert(near(nativeHeading(0,-pi/2,0),pi/2)); // turn head right -> native right
 assert(near(nativeHeading(0,pi/2,0),-pi/2)); // turn head left -> native left
 assert(near(nativeHeading(.4f,.3f,.2f),.3f));
 auto forward=relativeToView({0,1},0,pi/2);
 assert(near(forward.x,1)&&near(forward.y,0));
 auto headLeft=relativeToView({0,1},pi/2,0);
 assert(near(headLeft.x,-1)&&near(headLeft.y,0));
 auto cancel=relativeToView({0,1},pi/2,pi/2);
 assert(near(cancel.x,0)&&near(cancel.y,1));
 for(int degrees=-180;degrees<=180;degrees+=15){
  float a=degrees*pi/180;
  auto f=relativeToView({0,1},0,a),r=relativeToView({1,0},0,a);
  assert(near(std::hypot(f.x,f.y),1)&&near(f.x*r.x+f.y*r.y,0));
 }
 assert(deadzone({.1f,0}).x==0);
 auto diagonal=deadzone({1,1});assert(near(std::hypot(diagonal.x,diagonal.y),1));
 Walk a,b;
 for(int i=0;i<72;i++)a.step({0,1},0,0,1.f/72,true);
 for(int i=0;i<144;i++)b.step({0,1},0,0,1.f/144,true);
 assert(near(a.velocity.y,b.velocity.y)&&a.velocity.y>.99f);
 float moving=a.velocity.y;
 a.step({},0,0,1.f/72,true);assert(a.velocity.y>0&&a.velocity.y<moving);
 for(int i=0;i<160;i++)a.step({},0,0,1.f/72,true);
 assert(a.velocity.x==0&&a.velocity.y==0);
 a.step({1,0},0,0,.02f,true);a.step({},0,0,.02f,false);assert(a.velocity.x==0);
 Turn t;for(int i=0;i<72;i++)t.step(1,1.f/72,true);
 assert(t.angle>.58f&&t.angle<.64f);
 float angle=t.angle;t.step(0,.02f,true);assert(t.angle==angle&&t.rate==0);
 t.step(1,.02f,false);assert(t.angle==angle);
 StableView stable;float start=.4f;
 stable.step(start,0,true);
 for(int i=0;i<500;i++){
  float camera=start+i*.025f,user=.3f;
  float rig=stable.step(std::remainder(camera,2*pi),user,true);
  // The actual uploaded row-major rig matrix rotates the viewer right.
  // Native heading + rig heading must remain invariant under native orbit.
  assert(std::abs(std::remainder(camera+rig-(start+user),2*pi))<.0001f);
 }
 auto before=stable.step(1.f,.2f,false);assert(std::isfinite(before));
 stable.reset();assert(near(stable.step(2.f,.3f,true),.3f));
 std::puts("PASS: directions, head/rig composition, deadzone, diagonal speed, frame-rate independence, braking and turn stop");
}
