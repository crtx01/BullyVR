static JavaVM* recoveryVm=nullptr;
static void bootstrapFiles(){
 const char* names[]={"Dolphin.ini","GFX.ini","BullyVR.ini","WiimoteNew.ini","RB7.ini","RB7E54.ini"};
 mkdir(BULLY_FILES "/Config",0700);mkdir(BULLY_FILES "/GameSettings",0700);mkdir(BULLY_FILES "/GameSettingsVR",0700);
 for(const char* name:names){
  std::string folder=!strcmp(name,"RB7.ini")?"GameSettingsVR":!strcmp(name,"RB7E54.ini")?"GameSettings":"Config";
  std::string path=std::string(BULLY_FILES)+"/"+folder+"/"+name;
  if(access(path.c_str(),F_OK)==0)continue;
  auto bytes=readAsset(std::string("default-")+name);if(bytes.empty())continue;
  std::string content(bytes.begin(),bytes.end());
  if(!strcmp(name,"Dolphin.ini")){
   auto at=content.find("ISOPath0 = ");
   if(at!=content.npos){auto end=content.find('\n',at);content.replace(at,end-at,std::string("ISOPath0 = ")+BULLY_FILES);}
  }
  if(FILE* f=fopen(path.c_str(),"wb")){fwrite(content.data(),1,content.size(),f);fclose(f);}
 }
 // Apply this delivery's bindings once on upgrades as well as fresh installs.
 // Keep the old mapping beside it so user customizations remain recoverable.
 const char* revision=BULLY_FILES "/Config/controls-20261106-afternoon3.ready";
 if(access(revision,F_OK)!=0){
  auto bytes=readAsset("default-WiimoteNew.ini");
  if(!bytes.empty()){
   const char* profile=BULLY_FILES "/Config/WiimoteNew.ini";
   rename(profile,BULLY_FILES "/Config/WiimoteNew.before-afternoon3.ini");
   if(FILE* f=fopen(profile,"wb")){bool okay=fwrite(bytes.data(),1,bytes.size(),f)==bytes.size();fclose(f);if(okay)if(FILE* marker=fopen(revision,"w")){fputs("L3-tabs-controller-tip-aim\n",marker);fclose(marker);}}
  }
  auto vrProfile=readAsset("default-RB7.ini");
  if(!vrProfile.empty()){
   const char* path=BULLY_FILES "/GameSettingsVR/RB7.ini";
   rename(path,BULLY_FILES "/GameSettingsVR/RB7.before-afternoon3.ini");
   if(FILE* f=fopen(path,"wb")){fwrite(vrProfile.data(),1,vrProfile.size(),f);fclose(f);}
  }
  const char* controls=BULLY_FILES "/Config/RecoveryControls.ini";
  if(FILE* f=fopen(controls,"r")){
   char line[256];std::string content;
   while(fgets(line,sizeof(line),f))if(strncmp(line,"Hud2=",5)&&strncmp(line,"Hud3=",5)&&strncmp(line,"Hud4=",5)&&strncmp(line,"Hud5=",5)&&strncmp(line,"TurnSpeed=",10)&&strncmp(line,"TurnSmoothing=",14)&&strncmp(line,"EyeHeight=",10))content+=line;
   fclose(f);content+="Hud2=0\nHud3=0\nHud4=0\nHud5=0\nTurnSpeed=50\nTurnSmoothing=0.14\nEyeHeight=1.60\n";
   if(FILE* out=fopen(controls,"w")){fwrite(content.data(),1,content.size(),out);fclose(out);}
  }
 }
 // One-time centering migration: clear stale experimental HUD offsets from
 // older builds so the first launch of this layout starts with readable,
 // centered mission/instruction text and the game's corner radar placement.
 const char* hudRevision=BULLY_FILES "/Config/hud-preview-20261106-v1.ready";
 if(access(hudRevision,F_OK)!=0){
  const char* controls=BULLY_FILES "/Config/RecoveryControls.ini";
  std::string content;char line[256];
  if(FILE* f=fopen(controls,"r")){while(fgets(line,sizeof(line),f))if(strncmp(line,"Hud0=",5)&&strncmp(line,"Hud1=",5)&&strncmp(line,"Hud2=",5)&&strncmp(line,"Hud3=",5)&&strncmp(line,"Hud4=",5)&&strncmp(line,"Hud5=",5))content+=line;fclose(f);}
  content+="Hud0=0\nHud1=0\nHud2=0\nHud3=0\nHud4=0\nHud5=0\n";
  if(FILE* f=fopen(controls,"w")){fwrite(content.data(),1,content.size(),f);fclose(f);}
  if(FILE* f=fopen(hudRevision,"w")){fputs("centered-hud-preview-v1\n",f);fclose(f);}
 }
 const char* interactionRevision=BULLY_FILES "/Config/interact-grab-20261110-v3.ready";
 if(access(interactionRevision,F_OK)!=0){
  auto bytes=readAsset("default-WiimoteNew.ini");
  if(!bytes.empty()){
   const char* profile=BULLY_FILES "/Config/WiimoteNew.ini";
   rename(profile,BULLY_FILES "/Config/WiimoteNew.before-interact-grab.ini");
   if(FILE* f=fopen(profile,"wb")){bool okay=fwrite(bytes.data(),1,bytes.size(),f)==bytes.size();fclose(f);if(okay)if(FILE* marker=fopen(interactionRevision,"w")){fputs("L2-interact-Y-grab-held\n",marker);fclose(marker);}}
  }
 }
#if BULLY_DEMO
 const char* path=BULLY_FILES "/exploration-start.sav";
 if(access(path,F_OK)!=0){
  AAsset* asset=AAssetManager_open(assets,"recovery/exploration-start.sav",AASSET_MODE_STREAMING);
  if(asset){if(FILE* f=fopen(path,"wb")){char buffer[65536];int n;while((n=AAsset_read(asset,buffer,sizeof(buffer)))>0)fwrite(buffer,1,n,f);fclose(f);}AAsset_close(asset);}
 }
#endif
}
static void bootExploration(){
#if BULLY_DEMO
 static int readyFrames=0;static bool loaded=false;
 // A fresh user has no player ped on the title screen. Wait for emulation
 // frames, not gameplay, so the preview never requires starting a mission.
 if(loaded||!recoveryVm||++readyFrames<90)return;
 loaded=true;
 const char* path=BULLY_FILES "/exploration-start.sav";if(access(path,R_OK)!=0)return;
 JNIEnv* env=nullptr;bool attached=false;
 if(recoveryVm->GetEnv((void**)&env,JNI_VERSION_1_6)!=JNI_OK){if(recoveryVm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return;attached=true;}
 auto load=(void(*)(JNIEnv*,jclass,jstring))dlsym(dlopen("libmain.so",RTLD_NOW),"Java_org_dolphinemu_dolphinemu_NativeLibrary_LoadStateAs");
 if(load){jstring text=env->NewStringUTF(path);load(env,nullptr,text);env->DeleteLocalRef(text);LOG("Exploration preview start requested");}
 if(attached)recoveryVm->DetachCurrentThread();
#endif
}
