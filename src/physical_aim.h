// RB7E54: redirect only Jimmy's native projectile factory and target query.
static void updatePhysicalAim(const Snapshot& input,bool enabled,bool blockLock,bool visibility){
 physicalAimDisplayValid.store(false);
 if(!installContactBridge())return;
 uint32_t payload=contactMailbox-contactMailboxOffset,ray=payload+physical_rayOffset;
 static bool flushPending=false;
 constexpr uint32_t sites[]={0x801ea2c0,0x801a8944,0x8014d7ac,0x80160b40,0x801684a0};
 constexpr uint32_t original[]={0x542b073e,0x9421fff0,0x9421ffc0,0x542b073e,0x9421ffe0};
 const unsigned offsets[]={physical_projectileOffset,physical_targetOffset,physical_frustumOffset,physical_camera_updateOffset,physical_player_visibilityOffset};
 bool hooks=true;
 for(int i=0;i<5;i++){
  uint32_t current=gameRead(sites[i]),destination=payload+offsets[i];
  uint32_t jump=0x48000000u|((destination-sites[i])&0x03fffffcu);
  if(current==jump)continue;
  int32_t displacement=int32_t(current<<6)>>6;
  uint32_t oldDestination=sites[i]+displacement;
  bool previousOwn=(current&0xfc000003u)==0x48000000u&&oldDestination>=0x80001800&&oldDestination<0x80003000;
  if(current!=original[i]&&!previousOwn){hooks=false;continue;}
  gameWrite(sites[i],jump);flushPending=true;
 }
 if(flushPending&&nativeRequest(6,gameRead(0x80624204),0,{}))flushPending=false;
 uint32_t camera=payload+physical_cameraOffset;
 gameFloatWrite(camera+4,playerEyeHeight.load());gameWrite(camera,hooks&&visibility?1:0);
 uint32_t vis=payload+physical_visibilityOffset;
 gameWrite(vis,0);
 if(visibility){V center=pedPosition(gameRead(0x80624204))+V{0,0,playerEyeHeight.load()};for(int i=0;i<3;i++)gameFloatWrite(vis+4+i*4,(&center.x)[i]);gameWrite(vis,1);}
 gameWrite(ray,0);gameWrite(ray+60,hooks&&blockLock?1:0);
 if(!hooks||!enabled||!input.active||!input.head.valid||!input.c[1].aim.valid)return;
 uint32_t rw=gameRead(0x806247b0+0x80),frame=gameRead(rw+0xac);
 if(!gameAddress(frame))return;
 V right=norm({gameFloat(frame+0x20),gameFloat(frame+0x24),0});
 V forward=norm({gameFloat(frame+0x40),gameFloat(frame+0x44),0});
 float angle=rigYaw.load(),c=std::cos(angle),s=std::sin(angle);
 auto world=[&](V v){return right*(c*v.x-s*v.z)+forward*(-s*v.x-c*v.z)+V{0,0,v.y};};
 const auto& hand=input.c[1].aim;
 V localDirection=norm(rotate(hand.q,{0,0,-1}));
 V direction=norm(world(localDirection));
 const auto& grip=input.c[1].grip.valid?input.c[1].grip:hand;
 V relative{grip.p[0]-trackingHomeX.load(),grip.p[1]-trackingHomeY.load(),grip.p[2]-trackingHomeZ.load()};
 int item=weaponCurrent.load();float muzzle=weaponIds[item]==307?.51f:weaponIds[item]==305?.34f:weaponIds[item]==396?.51f:.08f;
 V origin=pedPosition(gameRead(0x80624204))+V{0,0,playerEyeHeight.load()}+world(relative)+direction*muzzle;
 if(!finitePosition(origin)||!finitePosition(direction)||length(direction)<.95f)return;
 // Charged native attacks may spawn after the release animation begins.
 // Retain the last held ray during that short interval instead of following
 // the controller after the user has already lowered it.
 static bool held=false;static int previousItem=-2;static double releaseUntil=0;
 static V lastOrigin{},lastDirection{},lastUiOrigin{},lastUiDirection{};static uint32_t shotSerial=0;
 bool pressed=input.c[1].trigger||input.c[1].tv>.25f;
 uint32_t serial=gameRead(ray+28);
 if(previousItem!=item){held=false;releaseUntil=0;previousItem=item;}
 if(held&&!pressed)releaseUntil=now()+.45;
 V uiOrigin{grip.p[0],grip.p[1],grip.p[2]};
 if(pressed){releaseUntil=0;lastOrigin=origin;lastDirection=direction;lastUiOrigin=uiOrigin;lastUiDirection=localDirection;}
 else if(now()<releaseUntil&&serial==shotSerial){origin=lastOrigin;direction=lastDirection;uiOrigin=lastUiOrigin;localDirection=lastUiDirection;}
 else {releaseUntil=0;lastOrigin=origin;lastDirection=direction;lastUiOrigin=uiOrigin;lastUiDirection=localDirection;}
 uiOrigin=uiOrigin+localDirection*muzzle;
 for(int i=0;i<3;i++){physicalAimDisplayOrigin[i].store((&uiOrigin.x)[i]);physicalAimDisplayDirection[i].store((&localDirection.x)[i]);}
 physicalAimDisplayValid.store(true);
 held=pressed;shotSerial=serial;
 for(int i=0;i<3;i++){gameFloatWrite(ray+4+i*4,(&origin.x)[i]);gameFloatWrite(ray+16+i*4,(&direction.x)[i]);}
 gameWrite(ray,1);
}
