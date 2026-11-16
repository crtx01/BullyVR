#pragma once
#include <algorithm>
#include <cmath>

namespace locomotion {
constexpr float pi=3.14159265358979323846f;
// OpenXR is right-handed, with positive yaw looking left. RenderWare's
// heading atan2(forward.x, forward.y) grows to the right.
inline float nativeHeading(float base,float xrYaw,float rightTurn){return std::remainder(base-xrYaw+rightTurn,2*pi);}
struct Stick {float x=0,y=0;};
inline Stick deadzone(Stick v,float radius=.15f){
 float m=std::hypot(v.x,v.y);
 if(!std::isfinite(m)||m<=radius)return {};
 float speed=std::clamp((m-radius)/(1-radius),0.f,1.f);
 speed=speed*(.45f+.55f*speed);
 return {v.x/m*speed,v.y/m*speed};
}
// OpenXR yaw is positive to the left. Our rig turn is positive to the right.
// Convert the viewer's right/forward axes into the game's right/forward axes once.
inline Stick relativeToView(Stick v,float headYaw,float turnYaw){
 float a=headYaw-turnYaw,c=std::cos(a),s=std::sin(a);
 return {c*v.x-s*v.y,s*v.x+c*v.y};
}
struct Walk {
 Stick velocity{};
 void reset(){velocity={};}
 Stick step(Stick raw,float headYaw,float turnYaw,float dt,bool enabled){
  if(!enabled){reset();return {};}
  Stick target=relativeToView(deadzone(raw),headYaw,turnYaw);
 // A longer response time removes the small Quest tracking spikes that made
 // the old walk feel like it was snapping between directions.
 float tau=std::hypot(target.x,target.y)<.001f?.16f:.12f;
  float alpha=-std::expm1(-std::clamp(dt,0.f,.05f)/tau);
  velocity.x+=(target.x-velocity.x)*alpha;velocity.y+=(target.y-velocity.y)*alpha;
  if(std::hypot(velocity.x,velocity.y)<.002f&&target.x==0&&target.y==0)reset();
  return velocity;
 }
};
struct Turn {
 float angle=0,rate=0;
 float step(float raw,float dt,bool enabled,float degreesPerSecond=40.f,float smoothing=.14f){
  if(!enabled){rate=0;return angle;}
 float target=deadzone({raw,0},.20f).x*(std::clamp(degreesPerSecond,15.f,120.f)*pi/180.f);
 float alpha=-std::expm1(-std::clamp(dt,0.f,.05f)/std::clamp(smoothing,.03f,.4f));
  rate+=(target-rate)*alpha;
  if(std::abs(raw)<.20f)rate=0; // Stop immediately when released; no rotation drift.
  angle=std::remainder(angle+rate*std::clamp(dt,0.f,.05f),2*pi);
  return angle;
 }
};
// The Wii camera orbits independently when Jimmy changes direction. Cancel
// that orbit so a held stick keeps the same heading in the physical room.
struct StableView {
 bool initialized=false;float previous=0,compensation=0;
 void reset(){initialized=false;previous=compensation=0;}
 float step(float gameYaw,float userTurn,bool valid){
  if(!valid||!std::isfinite(gameYaw))return std::remainder(userTurn+compensation,2*pi);
  if(!initialized){previous=gameYaw;initialized=true;}
  // The uploaded rig matrix is R_y(-angle): its positive angle turns the
  // rendered viewer right, just like nativeHeading. Cancel with the opposite
  // delta; adding it doubles camera orbit and makes the world swim.
  compensation=std::remainder(compensation-std::remainder(gameYaw-previous,2*pi),2*pi);
  previous=gameYaw;return std::remainder(userTurn+compensation,2*pi);
 }
};
}
