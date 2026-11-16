// Native scripted camera 4: position path +0x9c, node count +0xf0, target +0x80.
static bool freezeNativeCamera(bool enabled,bool cinematic=false){
 constexpr uint32_t manager=0x806247b0;
 static bool owned=false;static uint32_t previousType=1,cam=0,allow=0,camVtable=0;
 static V forward{};static std::array<uint32_t,16> saved{},controlled{};
 constexpr uint32_t fields[]={4,0x7c,0x80,0x84,0x88,0x9c,0xa0,0xa4,0xf0,0xf4,0xf8,0xfc,0x100,0x104,0x10,0xc};
 auto request=[&](uint32_t type){gameWrite(manager+0x28c,type);gameWrite(manager+0x290,0);gameWrite(manager+0x294,0x01000000);gameWrite(manager+0x298,0);gameWrite(manager+0x29c,0);};
 auto release=[&]{if(owned){if(gameRead(cam)==camVtable){
  // Restore only values that still belong to us. Preserve paths already
  // authored by the cutscene director, while removing stale VR eye positions.
  // A director transition can reuse this camera object. Its zero counters and
  // path flags may equal our values, but belong to the new shot: do not restore
  // stale pre-VR path state over them. Only release pose/FOV values still ours.
  for(int i=0;i<16;i++){
   bool pose=fields[i]==0x80||fields[i]==0x84||fields[i]==0x88||fields[i]==0x9c||fields[i]==0xa0||fields[i]==0xa4||fields[i]==0x104;
   if((!cinematic||pose)&&gameRead(cam+fields[i])==controlled[i])gameWrite(cam+fields[i],saved[i]);
  }
  if(!cinematic&&gameRead(manager+0x258)==cam&&gameRead(manager+0x84)==4)request(previousType);
 }
 // Only bit 24 was borrowed for camera ownership. Preserve all other live
 // manager flags, including changes made by the mission during the fade.
 uint32_t live=gameRead(manager+0x1a0);
 gameWrite(manager+0x1a0,(live&~0x01000000u)|(cinematic?0x01000000u:(allow&0x01000000u)));
 owned=false;LOG("Scripted VR camera released");}};
 uint32_t player=gameRead(0x80624204),nextCam=gameRead(manager+0x258);
 if(!enabled||!gameAddress(player)||!gameAddress(nextCam)){release();return false;}
 if(owned&&nextCam!=cam)release();
 uint32_t transform=gameRead(player+0x18),position=gameAddress(transform)?transform+0x30:player+4;
 V root{gameFloat(position),gameFloat(position+4),gameFloat(position+8)};
 if(!std::isfinite(root.x)||!std::isfinite(root.y)||!std::isfinite(root.z))return owned;
 // Native visibility follows the viewer. StableView cancels this rotation in
 // the OpenXR rig, leaving exactly one head rotation in the rendered result.
 // Keep the Wii view basis fixed. OpenXR owns all head rotation and stick
 // turn; feeding the headset back into this camera creates a second clock.
 if(!owned){
  uint32_t rw=gameRead(manager+0x80),frame=gameRead(rw+0xac);if(!gameAddress(frame))return false;
  forward=norm(V{gameFloat(frame+0x40),gameFloat(frame+0x44),0});if(length(forward)<.9f)return false;
  previousType=gameRead(manager+0x84);allow=gameRead(manager+0x1a0);if(previousType==4){previousType=1;allow|=0x01000000u;}cam=nextCam;camVtable=gameRead(cam);
  for(int i=0;i<16;i++)saved[i]=gameRead(cam+fields[i]);
  gameWrite(manager+0x1b0,gameRead(manager+0x1b0)&0x00ffffffu);
  owned=true;LOG("Native scripted camera 4 acquired; auto-orbit disabled");
 }
 // Rotate at the eyes instead of orbiting four metres around Jimmy. An orbit
 // makes head rotation translate the CPU camera and invalidates near visibility.
 V pos=root+V{0,0,playerEyeHeight.load()},target=pos+forward*4.f;
 gameWrite(cam+4,1);gameWrite(cam+0x7c,1);gameWrite(cam+0xf0,0);gameWrite(cam+0xf4,0);gameWrite(cam+0xf8,0);gameWrite(cam+0xfc,0);
 gameFloatWrite(cam+0x9c,pos.x);gameFloatWrite(cam+0xa0,pos.y);gameFloatWrite(cam+0xa4,pos.z);
 gameFloatWrite(cam+0x80,target.x);gameFloatWrite(cam+0x84,target.y);gameFloatWrite(cam+0x88,target.z);gameFloatWrite(cam+0x104,submissionFov.load());
 // Allow the action tree to perform the initial switch before suppressing auto-orbit.
 if(gameRead(manager+0x84)!=4){gameWrite(manager+0x1a0,allow|0x01000000u);request(4);}
 else gameWrite(manager+0x1a0,allow&0x00ffffffu);
 for(int i=0;i<16;i++)controlled[i]=gameRead(cam+fields[i]);
 return true;
}
