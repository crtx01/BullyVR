// Recovery-owned settings: no edits to the original menu's private ABI.
static std::atomic<float> vrTurnSpeed{50.f},vrTurnSmoothing{.14f};
static std::atomic<bool> settingsOpen{false};
static std::atomic<int> settingsRow{0};
static std::atomic<int> vrLanguage{0}; // 0 Portuguese, 1 English.
static const char* tr(const char* pt,const char* en){return vrLanguage?en:pt;}
static constexpr const char* settingsPath=BULLY_FILES "/Config/RecoveryControls.ini";
static void saveVrSettings(){
 if(FILE* f=fopen(settingsPath,"w")){
  fprintf(f,"TurnSpeed=%.2f\nTurnSmoothing=%.3f\nEyeHeight=%.3f\nLanguage=%d\n",vrTurnSpeed.load(),vrTurnSmoothing.load(),playerEyeHeight.load(),vrLanguage.load());
  constexpr uintptr_t offsets[]={0x1741600,0x1741604,0x1741608,0x174160c,0x1741610,0x1741614,0xde4f04};
  for(int i=0;i<7;i++)fprintf(f,"Hud%d=%.3f\n",i,mem<float>(offsets[i]));fclose(f);
 }
}
static bool loadVrSettings(){
 FILE* f=fopen(settingsPath,"r");if(!f)return false;
 char key[64];float v;bool height=false;
 while(fscanf(f," %63[^=]=%f",key,&v)==2){
  if(!std::isfinite(v))continue;
  int hud=-1;if(sscanf(key,"Hud%d",&hud)==1&&hud>=0&&hud<7){constexpr uintptr_t offsets[]={0x1741600,0x1741604,0x1741608,0x174160c,0x1741610,0x1741614,0xde4f04};mem<float>(offsets[hud])=std::clamp(v,hud==6?.4f:-3.f,hud==6?2.f:3.f);}
  if(!strcmp(key,"TurnSpeed"))vrTurnSpeed=std::clamp(v,15.f,120.f);
  if(!strcmp(key,"Language"))vrLanguage=v>=.5f?1:0;
  if(!strcmp(key,"TurnSmoothing"))vrTurnSmoothing=std::clamp(v,.03f,.4f);
  if(!strcmp(key,"EyeHeight")){playerEyeHeight=std::clamp(v,1.2f,2.05f);height=true;}
 }
 fclose(f);return height;
}
static void resetMessageOffsets(){
 for(auto off:{0x1741608,0x174160c,0x1741610,0x1741614})mem<float>(off)=0.f;
}
