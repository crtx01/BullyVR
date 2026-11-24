#include "app_profile.h"
// Local recovery layer for the user's 6955095 BullyVR build.
// The pinned offsets are verified against the original instruction bytes before use.
#include <jni.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <pthread.h>
#include <elf.h>
#include <sys/stat.h>
#include <cmath>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <array>
#include <tuple>
#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <GLES3/gl3.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "locomotion.h"
#include "aim_mode.h"
#include "menu_navigation.h"
#include "equipment_control.h"
#include "hud_textures.h"
#include "hud_panel.h"
#include "weapon_textures.h"
#include "player_clothing.h"
#include <strings.h>

#define LOG(...) __android_log_print(ANDROID_LOG_INFO,"BullyRecovery",__VA_ARGS__)
static uintptr_t base;
static AAssetManager* assets;
template<class T> static T& mem(uintptr_t off){return *reinterpret_cast<T*>(base+off);}
template<class F> static F fn(uintptr_t off){return reinterpret_cast<F>(base+off);}
struct V {float x{},y{},z{}; V operator+(V b)const{return{x+b.x,y+b.y,z+b.z};} V operator-(V b)const{return{x-b.x,y-b.y,z-b.z};} V operator*(float s)const{return{x*s,y*s,z*s};}};
static float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
static V cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
static float length(V a){return std::sqrt(dot(a,a));}
static V norm(V a){return a*(1/std::max(length(a),1e-6f));}
struct Q {float x,y,z,w;};
static V rotate(Q q,V v){V u{q.x,q.y,q.z};return v+cross(u,cross(u,v)+v*q.w)*2;}
static float yaw(Q q){return std::atan2(2*(q.w*q.y+q.x*q.z),1-2*(q.y*q.y+q.x*q.x));}
static double now(){timespec t{};clock_gettime(CLOCK_MONOTONIC,&t);return t.tv_sec+t.tv_nsec*1e-9;}
struct Pose {bool valid;float p[3];Q q;};
struct Velocity {bool lv,av;float linear[3],angular[3];};
struct Hit {bool valid;float u,v,d;};
struct Controller {bool connected,primary,secondary,menu,trigger,squeeze,stick;float tv,sv,sf,x,y;Pose aim,grip;Velocity vel;Hit hit;};
struct Snapshot {Controller c[2];Pose head;bool active;uint64_t generation;};
static_assert(sizeof(Controller)==136);static_assert(sizeof(Snapshot)==320);
static bool testInput(Snapshot&);
static Snapshot snapshot(){Snapshot s{};if(testInput(s))return s;auto* m=&mem<pthread_mutex_t>(0xdec9e0);pthread_mutex_lock(m);memcpy(&s,&mem<Snapshot>(0xdd7f28),sizeof(s));pthread_mutex_unlock(m);return s;}
#include "test_input.h"
static bool writeMem(uintptr_t address,const void* data,size_t n){
 const size_t page=sysconf(_SC_PAGESIZE);auto first=address&~(page-1);auto end=(address+n+page-1)&~(page-1);
 // Preserve segment permissions. Android refuses adding execute permission to RELRO data.
 int protection=PROT_READ;
 auto* elf=(Elf64_Ehdr*)base;auto* segments=(Elf64_Phdr*)(base+elf->e_phoff);
 for(int i=0;i<elf->e_phnum;i++)if(segments[i].p_type==PT_LOAD&&address>=base+segments[i].p_vaddr&&address<base+segments[i].p_vaddr+segments[i].p_memsz){
  if(segments[i].p_flags&PF_X)protection|=PROT_EXEC;
  if(segments[i].p_flags&PF_W)protection|=PROT_WRITE;
 }
 for(int i=0;i<elf->e_phnum;i++)if(segments[i].p_type==PT_GNU_RELRO&&address>=base+segments[i].p_vaddr&&address<base+segments[i].p_vaddr+segments[i].p_memsz)protection&=~PROT_WRITE;
 if(mprotect((void*)first,end-first,protection|PROT_WRITE))return false;
 memcpy((void*)address,data,n);__builtin___clear_cache((char*)address,(char*)(address+n));
 return mprotect((void*)first,end-first,protection)==0;
}
static void jump(void* p,void* target){uint32_t words[4]={0x58000051,0xd61f0220,0,0};memcpy(words+2,&target,8);memcpy(p,words,16);}
template<class F> static bool hook(uintptr_t off,F replacement,F& original,const char* expected){
 uint8_t bytes[16];for(int i=0;i<16;i++){unsigned x;sscanf(expected+i*2,"%2x",&x);bytes[i]=x;}
 if(memcmp((void*)(base+off),bytes,16)){LOG("Refused hook: incompatible offset %zx",off);return false;}
 void* trampoline=mmap(nullptr,4096,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);if(trampoline==MAP_FAILED)return false;
 memcpy(trampoline,bytes,16);jump((char*)trampoline+16,(void*)(base+off+16));
 mprotect(trampoline,4096,PROT_READ|PROT_EXEC);original=(F)trampoline;
 uint8_t branch[16];jump(branch,(void*)replacement);return writeMem(base+off,branch,16);
}
static std::vector<uint8_t> readAsset(const std::string& name){
 auto* a=AAssetManager_open(assets, ("recovery/"+name).c_str(),AASSET_MODE_BUFFER);
 if(!a){
  // Recovered Windows filenames retain mixed case; Android ZIP lookup is exact.
  static std::unordered_map<std::string,std::string> assetCase;static bool indexed=false;
  auto lower=[](std::string value){for(char& c:value)if(c>='A'&&c<='Z')c+=32;return value;};
  if(!indexed){indexed=true;if(auto* dir=AAssetManager_openDir(assets,"recovery")){while(const char* file=AAssetDir_getNextFileName(dir))assetCase.emplace(lower(file),file);AAssetDir_close(dir);}}
  auto found=assetCase.find(lower(name));if(found!=assetCase.end())a=AAssetManager_open(assets,("recovery/"+found->second).c_str(),AASSET_MODE_BUFFER);
  if(!a){LOG("Missing recovery asset: %s",name.c_str());return{};}
  LOG("Resolved asset case: %s -> %s",name.c_str(),found->second.c_str());
 }
 std::vector<uint8_t> b(AAsset_getLength(a));AAsset_read(a,b.data(),b.size());AAsset_close(a);return b;
}
#include "tutorial_disc_patch.h"
struct Vertex {V p;float u,v,r,g,b,a;};
struct Part {std::string texture;std::vector<Vertex> v;};
struct Mesh {std::vector<Part> parts;V lo{100,100,100},hi{-100,-100,-100};};
static std::unordered_map<std::string,Mesh> meshes;
static void loadMesh(const char* name){
 auto b=readAsset(std::string(name)+".mesh");if(b.size()<4)return;size_t at=4;uint32_t count;memcpy(&count,b.data(),4);Mesh mesh;
 if(count>100)return;
 for(uint32_t i=0;i<count;i++){
  if(at+68>b.size())return;Part p;p.texture=std::string((char*)b.data()+at,strnlen((char*)b.data()+at,64));at+=64;
  uint32_t n;memcpy(&n,b.data()+at,4);at+=4;if(n>100000||at+n*sizeof(Vertex)>b.size())return;
  p.v.resize(n);memcpy(p.v.data(),b.data()+at,n*sizeof(Vertex));at+=n*sizeof(Vertex);
  for(auto& v:p.v){mesh.lo.x=std::min(mesh.lo.x,v.p.x);mesh.lo.y=std::min(mesh.lo.y,v.p.y);mesh.lo.z=std::min(mesh.lo.z,v.p.z);mesh.hi.x=std::max(mesh.hi.x,v.p.x);mesh.hi.y=std::max(mesh.hi.y,v.p.y);mesh.hi.z=std::max(mesh.hi.z,v.p.z);}
  mesh.parts.push_back(std::move(p));
 }meshes.emplace(name,std::move(mesh));
}
struct Texture {virtual ~Texture()=default;};
struct TextureConfig {uint32_t width,height,levels=1,layers=1,samples=1,format=0,flags=0;uint8_t type=0;};
static std::unordered_map<std::string,Texture*> textures;
static Texture* texture(const std::string& name){
 if(auto it=textures.find(name);it!=textures.end())return it->second;
 auto b=readAsset(name+".rgba");if(b.size()<8)return nullptr;TextureConfig c;memcpy(&c.width,b.data(),8);
 if(c.width>4096||c.height>4096||b.size()!=8+size_t(c.width)*c.height*4)return nullptr;
 auto* gfx=*mem<void**>(0xdd14d8);if(!gfx)return nullptr;auto** vt=*(void***)gfx;
 using Create=std::unique_ptr<Texture>(*)(void*,const TextureConfig&,std::string_view);
 auto t=((Create)vt[16])(gfx,c,"Bully Recovery model");if(!t)return nullptr;
 auto* raw=t.release();auto** tv=*(void***)raw;
 using Load=void(*)(void*,uint32_t,uint32_t,uint32_t,uint32_t,const uint8_t*,size_t,uint32_t);
 ((Load)tv[4])(raw,0,c.width,c.height,c.width,b.data()+8,b.size()-8,0);
 textures[name]=raw;LOG("Texture %s %ux%u",name.c_str(),c.width,c.height);return raw;
}
static std::atomic<float> rigYaw{0};
// Calibrate the root-to-eye offset once. OpenXR already applies subsequent
// head translations; following live HMD height here would double crouching.
static std::atomic<float> playerEyeHeight{1.60f};
static std::atomic<bool> cameraActive{false},wheelOpen{false};
static std::atomic<bool> helpOpen{false};
static std::atomic<bool> freeAim{false};
static std::atomic<float> slingChargeProgress{0};
// Shared display ray: the reticle and native projectile factory use one held
// ray, including the short release-animation latch.
static std::atomic<bool> physicalAimDisplayValid{false};
static std::atomic<float> physicalAimDisplayOrigin[3]{};
static std::atomic<float> physicalAimDisplayDirection[3]{};
static std::atomic<double> equipmentNoticeUntil{0};
static std::atomic<bool> nativeCameraEnabled{true};
static std::atomic<bool> transparentHudBacking{true};
static std::atomic<bool> cinema{false},gameReady{false},minigameActive{false};
// Frontend pages submit orthographic graphics without the gameplay world.
// Detect that presentation separately from the player's still-valid ped.
static std::atomic<double> lastWorldDrawAt{0};
static std::atomic<bool> originalMenu{false};
static std::atomic<float> eyeAnchorX{0},eyeAnchorY{0},eyeAnchorZ{-4};
static std::atomic<bool> eyeAnchorValid{false};
static std::atomic<float> nativeViewHeading{0};
// Only physical stick turn goes back into the native camera. rigYaw also
// includes native-camera compensation, so feeding it back creates a spin loop.
static std::atomic<float> cameraUserTurn{0};
static std::atomic<bool> nativeViewValid{false};
static std::atomic<float> submissionFov{112.f};
// Read and request equipment through the game's own weapon state, never a HUD icon.
static bool gameAddress(uint32_t a){return (a>=0x80000000u&&a<=0x817ffffcu)||(a>=0x90000000u&&a<=0x93fffffcu);}
static uint32_t gameRead(uint32_t a){if(!gameAddress(a))return 0;auto m=fn<void*(*)(void*)>(0x6b3a84)((void*)(base+0xdec9c8));return fn<uint32_t(*)(void*,uint32_t)>(0x5bb354)(m,a);}
static void gameWrite(uint32_t a,uint32_t v){if(!gameAddress(a))return;auto m=fn<void*(*)(void*)>(0x6b3a84)((void*)(base+0xdec9c8));fn<void(*)(void*,uint32_t,uint32_t)>(0x5bb688)(m,v,a);}
static float gameFloat(uint32_t a){auto u=gameRead(a);float v;memcpy(&v,&u,4);return v;}
static void gameFloatWrite(uint32_t a,float v){uint32_t u;memcpy(&u,&v,4);gameWrite(a,u);}
static void visibilityForVR(){
 constexpr uint32_t camera=0x806247b0;
 // Exact equivalent of CameraSetNearPlane, verified in this disc's PPC code.
 uint32_t rw=gameRead(camera+0x80);
 if(gameAddress(rw)){
  float near=gameFloat(rw+0xa0);
  if(std::isfinite(near)&&near>.051f&&near<30.f){gameFloatWrite(rw+0xa0,.05f);static bool logged=false;if(!logged){LOG("VR near clip %.3f -> 0.05",near);logged=true;}}
 }
 // SetPopulationFastCulling(false): SDA r13=0x806ed380, bit 7 of flags.
 uint32_t population=gameRead(0x806eb928);
 if(gameAddress(population)){
  uint32_t flags=gameRead(population+0x789c);
  if(flags&0x80000000u){gameWrite(population+0x789c,flags&0x7fffffffu);LOG("Native population fast culling disabled");}
  // AreaGetPopulationCullDistance reads +0x782c. The native population
  // update restores these from +0x7844..4c every frame (PPC 0x801b1804).
  // Extend both the active radii and their source profile, including rear
  // retention, instead of only widening the GPU projection.
  constexpr float distances[]={220.f,120.f,200.f};
  for(int i=0;i<3;i++)for(uint32_t field:{0x782cu,0x7844u,0x785cu}){
   float value=gameFloat(population+field+i*4);
   if(std::isfinite(value)&&value>1.f&&value<distances[i])gameFloatWrite(population+field+i*4,distances[i]);
  }
 }
 // Keep a realistic native FOV: 160 degrees also shrinks the engine's
 // projected-size/LOD estimates. Nearby VR retention is handled separately.
 float fov=gameFloat(camera+0x90);
 if(std::isfinite(fov)&&fov>5.f&&std::abs(fov-submissionFov.load())>.1f){gameFloatWrite(camera+0x90,submissionFov.load());gameWrite(camera+0x15c,0);}
}
#include "vr_settings.h"
#include "native_camera.h"
#include "travel.h"
#include "weapon_catalog.h"
#include "weapon_wheel.h"
static std::atomic<int> actualWeapon{-1};
static std::atomic<bool> equipmentKnown{false};
static std::atomic<int> pendingWeapon{-2};
static std::atomic<int> queuedWeapon{-1};
static std::atomic<double> pendingUntil{0},equipFailedUntil{0};
static std::atomic<bool> focusRequest{false},focusWaiting{false};
static std::atomic<double> focusDeadline{0};
static std::atomic<int> focusWeapon{-2};
static bool requestWeapon(int index);
static std::atomic<int> diagnosticMission{-1};
static std::atomic<bool> grantArsenal{false};
static std::atomic<int> arsenalProgress{-1};
static std::atomic<uint64_t> vrFrameCount{0};
static std::atomic<double> largestFrameGap{0};
#include "../upstream/Source/Core/Common/Functional.h"
static std::atomic<int> weaponCurrent{0};
static std::atomic<float> trackingHomeX{0},trackingHomeY{1.60f},trackingHomeZ{0};
static std::atomic<unsigned> nativeClock{8*60+30};

static std::atomic<int> effectMode{0};
static std::atomic<bool> skyboxDetection{true};
static std::atomic<int> skyMode{1};
static std::atomic<float> gameProjectionX{1.3f},gameProjectionY{1.7f},gameProjectionOffsetX{0},gameProjectionOffsetY{0};
static std::atomic<void*> rigManager{nullptr};
static std::atomic<bool> captureGameFrame{false};
static std::atomic<int> recordGameFrames{0};
static std::atomic<unsigned> effectSkips{0};
static std::atomic<unsigned> effectLogs{0};
// Only called by the local diagnostic agent, outside the render loop.
extern "C" __attribute__((visibility("default"))) void BullyRecovery_Diagnostic(int mode){
 if(mode==91){
  if(FILE* f=fopen(BULLY_FILES "/runtime-status.json","w")){
   fprintf(f,"{\"build\":\"20261110-charge-gesture-r4\",\"frames\":%llu,\"max_gap\":%.6f,\"weapon\":%d,\"arsenal_progress\":%d,\"rig_yaw\":%.6f,\"native_heading\":%.6f,\"free_aim\":%s,\"settings\":%s,\"quick_menu\":%s}",
    (unsigned long long)vrFrameCount.load(),largestFrameGap.exchange(0),actualWeapon.load(),arsenalProgress.load(),rigYaw.load(),nativeViewHeading.load(),freeAim?"true":"false",settingsOpen?"true":"false",travelOpen?"true":"false");fclose(f);
  }return;
 }
 if(mode==50){grantArsenal=true;return;}
 if(mode==250||mode==251){submissionFov=mode==250?112.f:160.f;return;}
 if(mode>=300&&mode<300+travelCount){diagnosticTravel=mode-300;return;}
 if(mode==60){diagnosticMission=5;return;}
 if(mode==61){diagnosticMission=10;return;}
 if(mode==90){captureGameFrame=true;return;}
 if(mode==92||mode==93){recordGameFrames=mode==92?1:2;return;}
 if(mode==10||mode==11){skyboxDetection=mode==10;LOG("Skybox detection=%d",skyboxDetection.load());return;}
 if(mode>=12&&mode<=14){skyMode=mode==12?2:mode==13?1:0;return;}
 if(mode==20||mode==21){travelOpen=mode==20;mem<uint8_t>(0x170b4ef)=0;return;}
 if(mode>=22&&mode<=24){mem<int>(0x17415f8)=mode==22?2:mode==23?3:1;mem<int>(0x17415fc)=0;mem<uint8_t>(0x170b4ef)=1;return;}
 if(mode==30||mode==31){wheelOpen=mode==30;return;}
 if(mode==40||mode==41){helpOpen=mode==40;return;}
 if(mode==70||mode==71){transparentHudBacking=mode==71;return;}
 if(mode==80||mode==81){nativeCameraEnabled=mode==81;return;}
 if(mode>=100&&mode<100+weaponCount){requestWeapon(mode-100);return;}
 effectLogs=0;effectMode=mode;LOG("EFB diagnostic mode %d, skipped=%u",mode,effectSkips.load());
}
#include "npc_contacts.h"
#include "demo_policy.h"
#include "physical_aim.h"
static std::array<std::atomic<int>,weaponCount> weaponStock;
static std::array<std::atomic<int>,weaponCount> weaponAmmo;
static bool requestWeapon(int index){
 if(index<0||index>=weaponCount||!equipmentKnown||!wheel::canEquip(index,weaponStock[index],weaponAmmo[index])){equipFailedUntil=now()+3.;return false;}
 if(pendingWeapon!=-2&&index!=0){queuedWeapon=index;return true;}
 if(index==0)queuedWeapon=-1;
 pendingUntil=now()+5.;pendingWeapon=weaponIds[index];
 LOG("Queued verified inventory selection %d",weaponIds[index]);return true;
}
static void readWeaponStock(uint32_t player){
 std::array<int,weaponCount> next{};next[0]=1;int rockets=0,potatoes=0;
 uint32_t inventory=gameRead(player+0x1e0),pool=gameRead(0x806ebaac);
 if(!gameAddress(inventory)||!gameAddress(pool))return;
 uint32_t data=gameRead(pool),flags=gameRead(pool+4),cap=gameRead(pool+8),stride=gameRead(pool+12);
 if(!gameAddress(data)||!gameAddress(flags)||cap>512||stride!=0x28)return;
 for(uint32_t i=0;i<cap;i++){
  if((gameRead(flags+(i&~3u))>>(24-(i&3)*8))&0x80)continue;
  uint32_t entry=data+i*stride;if(gameRead(entry+0x24)!=inventory)continue;
  uint32_t packed=gameRead(entry);int count=int(int16_t(packed>>16)),id=int(int16_t(packed&65535));
  if(count<=0)continue;
  if(id==308)rockets+=count;else if(id==316)potatoes+=count;
  for(int k=1;k<weaponCount;k++)if(weaponIds[k]==id)next[k]+=count;
 }
 for(int k=0;k<weaponCount;k++){weaponStock[k]=next[k];weaponAmmo[k]=wheel::ammoId(k)==308?rockets:wheel::ammoId(k)==316?potatoes:-1;}
}
#include "unified_menu.h"
static bool retireLegacyArsenal(){
 static double checkAt=0;if(now()<checkAt)return false;checkAt=now()+1.;
 constexpr uint32_t values[]={0x0005012d,0x0001012f,0x00010132,0x00050135,0x000c0138,0x001e0140,0x000c0141,0x00010145,0x00010148,0x0005015d,0x0005018a};
 for(uint32_t p=0x80002200;p<0x80002f80;p+=8){
  if(gameRead(p)!=0x4a000000||gameRead(p+4)!=0x92b79780)continue;
  bool match=true;for(unsigned i=0;i<11;i++)if(gameRead(p+8+i*8)!=(0x14000000u+i*0x28)||gameRead(p+12+i*8)!=values[i]){match=false;break;}
  if(!match)continue;
  for(unsigned i=0;i<12;i++){gameWrite(p+i*8,0xe0000000);gameWrite(p+i*8+4,0x80008000);}
  LOG("Retired legacy inventory overwrite; migrating arsenal through native item loader");return true;
 }
 return false;
}
static void updateGameState(){
 // 0x173915c is a transient stereo render heuristic, NOT core/game state.
 // It can be false in valid gameplay (e.g. after opening the map).
 uint32_t activePlayer=gameRead(0x80624204);
 bool ready=gameRead(0x80000000)==0x52423745u&&validPed(activePlayer);
 gameReady=ready;if(!ready){equipmentKnown=false;eyeAnchorValid=false;return;}
 updateDemoPolicy();
 uint32_t clock=gameRead(0x806ead68);unsigned hour=clock>>24,minute=(clock>>16)&255;
 if(hour<24&&minute<60)nativeClock=hour*60+minute;
 if(diagnosticMission>=0&&nativeRequest(7,activePlayer,0,{},diagnosticMission.load()))diagnosticMission=-1;
 uint32_t minigame=gameRead(0x806ec1e8);minigameActive=gameAddress(minigame)&&(gameRead(minigame+8)&0xff000000u);
 double lastWorld=lastWorldDrawAt.load();
 // Streaming/compiling a weapon can pause world submissions for several
 // frames. It must not release and reacquire the native camera each time.
 // Brief gaps in world submissions happen while the Wii streams interiors
 // and during mission/cutscene fades. Do not mistake them for the pause menu
 // and repeatedly release/reacquire the VR camera during those transitions.
 originalMenu=lastWorld>0&&now()-lastWorld>10.0;
 bool cinematic=originalMenu||minigameActive||(gameRead(0x806eab14)&0xffu)||(gameRead(0x806ecbd8)&0xff000000u);
 if(cinema.exchange(cinematic)!=cinematic)LOG("Original director camera=%d",cinematic);
 processTravel(activePlayer,cinematic);
 if(diagnosticTravel>=0&&!travelInProgress){travelRequest=diagnosticTravel.exchange(-1);}
 bool manual=freeAim.load();
 // PlayerIgnoreTargeting writes byte +0x20 of input module 5 (PPC
 // 0x803de968). Preserve the script's value outside ranged-weapon gameplay.
 static uint32_t ignoredModule=0,ignoredWord=0;
 uint32_t targetingModule=gameRead(0x8063119c);
 bool suppressLock=!cinematic&&actualWeapon!=-1&&
  !weaponMelee[weaponCurrent.load()]&&weaponIds[weaponCurrent.load()]!=437;
 if(suppressLock&&gameAddress(targetingModule)&&gameRead(targetingModule)==0x805b2838){
  if(ignoredModule!=targetingModule){ignoredModule=targetingModule;ignoredWord=gameRead(targetingModule+0x20);}
  gameWrite(targetingModule+0x20,gameRead(targetingModule+0x20)|0x01000000u);
 }else if(ignoredModule){
  if(gameRead(ignoredModule)==0x805b2838)gameWrite(ignoredModule+0x20,(gameRead(ignoredModule+0x20)&0x00ffffffu)|(ignoredWord&0xff000000u));
  ignoredModule=0;
 }
 auto vrInput=snapshot();
 static bool eyeHeightCalibrated=loadVrSettings();
 if(!eyeHeightCalibrated&&vrInput.head.valid&&std::isfinite(vrInput.head.p[1])&&vrInput.head.p[1]>.90f&&vrInput.head.p[1]<2.25f){
  playerEyeHeight=std::clamp(vrInput.head.p[1],1.20f,2.05f);eyeHeightCalibrated=true;
 }
 // Director flags can drop briefly between adjacent shots/fades. Let the
 // native camera finish the transition before taking camera 4 again.
 static double directorReleasedAt=0;
 if(cinematic)directorReleasedAt=now()+.75;
 bool control=nativeCameraEnabled&&mem<uint8_t>(0xde4ef0)&&!cinematic&&!travelInProgress&&now()>=directorReleasedAt;
 bool owned=freezeNativeCamera(control,cinematic);
 eyeAnchorValid=false;
 if(control||manual){
  visibilityForVR();
  uint32_t player=gameRead(0x80624204),transform=gameRead(player+0x18);
  uint32_t pos=gameAddress(transform)?transform+0x30:player+4;
  uint32_t rw=gameRead(0x806247b0+0x80),frame=gameRead(rw+0xac);
  if(gameAddress(frame)){
   V eye{gameFloat(pos),gameFloat(pos+4),gameFloat(pos+8)+playerEyeHeight.load()};
   V cam{gameFloat(frame+0x50),gameFloat(frame+0x54),gameFloat(frame+0x58)};
   V forward{gameFloat(frame+0x40),gameFloat(frame+0x44),gameFloat(frame+0x48)};
   // The camera-4 PPC hook follows the root during the actual camera update.
   // A second 30 Hz root-minus-old-camera correction adds movement twice.
   V d=eye-cam;float x=0,y=0,z=0;
   if(std::isfinite(forward.x)&&std::isfinite(forward.y)){nativeViewHeading=std::atan2(forward.x,forward.y);nativeViewValid=true;}
   if(std::isfinite(x)&&std::isfinite(y)&&std::isfinite(z)&&length(d)<20){eyeAnchorX=x;eyeAnchorY=y;eyeAnchorZ=z;eyeAnchorValid=true;}
  }
 }
 uint32_t player=gameRead(0x80624204),component=gameRead(player+0x1ec);
 if(gameAddress(player)&&gameAddress(component)){
  int id=int(int16_t(gameRead(component+0x110)&65535));
  if(id==427)id=301;else if(id==304)id=349;
  if(!equipmentKnown||actualWeapon!=id){LOG("Verified native equipment: %d",id);equipmentNoticeUntil=now()+2.5;}
  actualWeapon=id;equipmentKnown=true;
  int index=0;for(int i=0;i<weaponCount;i++)if(weaponIds[i]==id)index=i;weaponCurrent=index;
 }else equipmentKnown=false;
 updatePhysicalAim(vrInput,!cinematic&&!travelOpen&&!helpOpen&&!travelInProgress&&suppressLock,suppressLock&&!cinematic,control&&!cinematic);
 // The native launchers auto-unequip without their separate ammunition items.
 // Grant ammunition first when migrating the user's "Todas as armas" cheat.
 static constexpr int arsenal[]={308,316,301,303,306,309,312,320,321,325,328,349,394,326,300,305,307,396,437};
 static constexpr int arsenalCount=sizeof(arsenal)/sizeof(*arsenal);
 static int migrate=-1,migrationFailures=0;static unsigned migrationSerial=0;static double migrationDeadline=0,migrationRetryAt=0;
 static bool arsenalWasEnabled=false;
 bool arsenalEnabled=mem<int>(0x1741624)!=0;
 if(retireLegacyArsenal()||(arsenalEnabled&&!arsenalWasEnabled)||grantArsenal.exchange(false)){migrate=0;migrationSerial=0;}
 arsenalProgress=migrate>=0&&migrate<arsenalCount?migrate:-1;
 arsenalWasEnabled=arsenalEnabled;
 if(validPed(player))readWeaponStock(player);
 // Native cheat-list regeneration can discard an in-flight bridge request.
 // Retry that item instead of waiting forever for an unreachable serial.
 if(migrationSerial&&now()>migrationDeadline){migrationSerial=0;installContactBridge();}
 if(migrationSerial&&contactMailbox&&gameRead(contactMailbox+48)==migrationSerial){
  migrationSerial=0;
  if(int32_t(gameRead(contactMailbox+36))>0){++migrate;migrationFailures=0;}
  else if(++migrationFailures>=6){LOG("Arsenal item could not load: %d",arsenal[migrate]);++migrate;migrationFailures=0;}
  else migrationRetryAt=now()+.25;
 }
 if(migrate>=0&&migrate<arsenalCount&&!cinema&&pendingWeapon==-2&&!focusWaiting&&!migrationSerial&&now()>=migrationRetryAt&&nativeRequest(4,player,0,{},arsenal[migrate])){migrationSerial=contactSerial;migrationDeadline=now()+2.;}
 static unsigned ackSerial=0;
 if(contactMailbox){unsigned serial=gameRead(contactMailbox+48);if(serial&&serial!=ackSerial){ackSerial=serial;LOG("Native action completed serial=%u result=%d",serial,int32_t(gameRead(contactMailbox+36)));}}
 static int sentWeapon=-2;static unsigned sentSerial=0;static double equipRetryAt=0;
 if(pendingWeapon==-2&&queuedWeapon>=0){int next=queuedWeapon.exchange(-1);requestWeapon(next);}
 int target=pendingWeapon.load();
 if(target==-2){sentWeapon=-2;}
 else if(target==actualWeapon){pendingWeapon=-2;sentWeapon=-2;wheelOpen=false;equipFailedUntil=0;}
 else if(cinema||now()>pendingUntil){equipFailedUntil=now()+4.;pendingWeapon=-2;sentWeapon=-2;}
 else if((sentWeapon!=target||now()>equipRetryAt)&&nativeRequest(2,player,0,{},target)){sentWeapon=target;sentSerial=contactSerial;equipRetryAt=now()+.5;}
 else if(sentWeapon==target&&contactMailbox&&gameRead(contactMailbox+48)==sentSerial&&int32_t(gameRead(contactMailbox+36))<0){equipFailedUntil=now()+4.;pendingWeapon=-2;sentWeapon=-2;}
 // Face once before the native lock button reaches the game. PedFaceHeading
 // uses the opposite sign to atan2(camera.forward.x,camera.forward.y).
 // Repeating this during punches/grapples would fight their native animations.
 static unsigned focusSerial=0;
 if(focusWaiting){
  if(cinematic||actualWeapon!=focusWeapon||now()>focusDeadline){focusWaiting=false;focusRequest=false;focusSerial=0;}
  else if(focusRequest&&pendingWeapon==-2&&nativeViewValid){
   auto input=snapshot();
   float heading=nativeViewHeading.load()+rigYaw.load()-(input.head.valid?yaw(input.head.q):0.f);
   if(std::isfinite(heading)&&nativeRequest(5,player,-std::remainder(heading,2*locomotion::pi),{})){
    focusSerial=contactSerial;focusRequest=false;
   }
  }else if(focusSerial&&contactMailbox&&gameRead(contactMailbox+48)==focusSerial){focusWaiting=false;focusSerial=0;}
 }
 int index=weaponCurrent.load();float objectLength=index==13?.65f:index==12?.32f:index==2||index==3?.18f:index>0?.12f:0;
 updateNpcContacts(snapshot(),rigYaw.load(),(owned||manual)&&!cinema&&!mem<uint8_t>(0x170b4ef)&&!wheelOpen&&!helpOpen,objectLength);
 static double statusAt=0;if(now()-statusAt>5.){statusAt=now();LOG("Runtime camera=%u cinema=%d eye=%.2f,%.2f,%.2f equipment=%d pending=%d",gameRead(0x806247b0+0x84),cinema.load(),eyeAnchorX.load(),eyeAnchorY.load(),eyeAnchorZ.load(),actualWeapon.load(),pendingWeapon.load());}
}
#include "bootstrap.h"
static void* gameStateWorker(void*){
 auto running=(jboolean(*)(JNIEnv*,jclass))dlsym(dlopen("libmain.so",RTLD_NOW),"Java_org_dolphinemu_dolphinemu_NativeLibrary_IsRunningAndUnpaused");
 static std::atomic<bool> updatePending{false};
 while(true){
  usleep(33333);
  if(!running||!running(nullptr,nullptr)){gameReady=false;cameraActive=false;updatePending=false;continue;}
  bootExploration();
  // A long load/fade can stall the emulated CPU. Keep at most one pending
  // update instead of accumulating camera/equipment writes to run on resume.
  if(!updatePending.exchange(true))fn<void(*)(void*,Common::MoveOnlyFunction<void()>)>(0x518bb4)((void*)(base+0xdec9c8),[]{updateGameState();updatePending=false;});
 }
 return nullptr;
}
#include "frame_capture.h"
static const HudTexture* hudTexture(uint64_t hash){
 auto i=std::lower_bound(std::begin(hudTextures),std::end(hudTextures),hash,[](const HudTexture& a,uint64_t b){return a.hash<b;});
 return i!=std::end(hudTextures)&&i->hash==hash?i:nullptr;
}
static void observeHudTexture(const char* name);
using Draw=void(*)(void*,void*,void*,void*,uint64_t,uint64_t,uint32_t,void*);
static Draw originalDraw;
using Batch=void(*)(void*,uint32_t,uint32_t,int32_t);
static Batch originalBatch;
static thread_local bool hudBatch=false;
static thread_local bool effectBatch=false;
static thread_local bool hudAssetBatch=false;
static thread_local bool hudTextBatch=false;
static thread_local bool radarBatch=false;
static thread_local int currentHudGroup=1,batchHudGroup=1;
static thread_local float messageCenter=0.f;
static thread_local bool messagePanelMeasured=false;
static thread_local bool skyBatch=false;
static uint64_t gameFrame=0,hudDepthFrame=~uint64_t(0);
static bool worldDrawnSinceHud=false;
static void batchDraw(void* batch,uint32_t index,uint32_t count,int32_t vertex){
 static bool logged=false;if(!logged){LOG("Batch hook active");logged=true;}
 if(effectBatch){static bool loggedEffect=false;if(!loggedEffect){LOG("Actual effect GPU batch skipped");loggedEffect=true;}return;}
 if(hudBatch&&worldDrawnSinceHud){
  worldDrawnSinceHud=false;
  hudDepthFrame=gameFrame;
  GLboolean scissor=glIsEnabled(GL_SCISSOR_TEST),depthMask;GLfloat depthClear;
  glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);glGetFloatv(GL_DEPTH_CLEAR_VALUE,&depthClear);
  glDisable(GL_SCISSOR_TEST);glDepthMask(GL_TRUE);glClearDepthf(0.f);glClear(GL_DEPTH_BUFFER_BIT);
  glDepthMask(depthMask);glClearDepthf(depthClear);if(scissor)glEnable(GL_SCISSOR_TEST);
 }
 originalBatch(batch,index,count,vertex);
}
static void draw(void* batch,void* pixel,void* geom,void* custom,uint64_t data,uint64_t size,uint32_t primitive,void* pipeline){
 bool ortho=*(int*)(mem<uint8_t*>(0xdd02c0)+0x4098)!=0;
 float route=*(float*)((uint8_t*)geom+0x134);
 bool game=mem<uint8_t>(0x170b4ee)&&gameReady;
 auto* cache=*mem<void**>(0xdd1520);const HudTexture* asset=nullptr;uint64_t textureHash=0;bool gameText=false,largeCopy=false,isSky=false,smallPlain=true,heldTexture=false;
 auto* bp=mem<uint32_t*>(0xdd0e28);uint32_t used=0;
 for(unsigned stage=0;stage<=((bp[0]>>10)&15);stage++){uint32_t order=bp[0x28+stage/2]>>((stage&1)*12);if(order&64)used|=1u<<(order&7);}
 for(unsigned stage=0;stage<8;stage++)if(game&&cache&&(used&(1u<<stage))){
  uint64_t hash=fn<uint64_t(*)(void*,uint32_t)>(0x846528)(cache,stage);if(!textureHash)textureHash=hash;
  if(equipmentKnown&&weaponCurrent>0){auto entry=std::lower_bound(std::begin(weaponTextures),std::end(weaponTextures),hash,[](const WeaponTexture& a,uint64_t b){return a.hash<b;});while(entry!=std::end(weaponTextures)&&entry->hash==hash){heldTexture|=entry->index==weaponCurrent.load();++entry;}}
  if(auto* ui=hudTexture(hash)){asset=ui;observeHudTexture(ui->name);}
  gameText|=hash==0x3b75d1edb5f3e59dULL||hash==0x641cfc23afa8fb0eULL||hash==0x49e5f5f8ac72c9eeULL;
  isSky|=hash==0x67d77e18544d37f7ULL||hash==0x765b3bc4103f6f8aULL||hash==0xa7b48c01e7585df2ULL;
  if(ortho){
   uint32_t width=0,height=0;bool efb=fn<bool(*)(void*,uint32_t,uint32_t*,uint32_t*)>(0x8465ec)(cache,stage,&width,&height);
   if(!efb){auto* entry=*(uint8_t**)((uint8_t*)cache+0x50+stage*16);if(entry){auto* name=entry+0xa0;const char* str=(name[0]&1)?*(const char**)(name+16):(const char*)(name+1);if(str)sscanf(str,"tex1_%ux%u",&width,&height);}}
   largeCopy|=width>=320&&height>=240;
   smallPlain&=width>0&&height>0&&width<=8&&height<=8;
  }
 }
 bool background=game&&!cinema&&ortho&&largeCopy&&!asset&&!gameText&&!mem<uint8_t>(0x170b4ef);
 skyBatch=game&&isSky;
 // Tiny solid-color images are shared by HUD and scenery materials. A texture
 // match alone must never move perspective world geometry onto the HUD plane.
 if(!ortho){asset=nullptr;gameText=false;}
 if(game&&!ortho&&!asset&&!skyBatch){
  lastWorldDrawAt=now();
  currentHudGroup=1;
  messagePanelMeasured=false;
  worldDrawnSinceHud=true;
  auto* p=(float*)(mem<uint8_t*>(0xdd02c0)+0x4080);
  if(std::isfinite(p[0])&&std::isfinite(p[2])&&p[0]>.1f&&p[0]<10&&p[2]>.1f&&p[2]<10){gameProjectionX=p[0];gameProjectionY=p[2];gameProjectionOffsetX=p[1];gameProjectionOffsetY=p[3];}
 }
 if(asset||gameText){route=-2.f;*(float*)((uint8_t*)geom+0x134)=route;}
 if(effectMode.load()==4&&effectLogs.fetch_add(1)<1200){
  auto* vs=fn<uint8_t*(*)(void*)>(0x6b3b64)((void*)(base+0xdec9c8));
  LOG("Draw hash=%016llx ortho=%d route=%.0f xyz=%.2f,%.2f,%.2f indices=%zu",(unsigned long long)textureHash,ortho,route,*(float*)(vs+0x2c),*(float*)(vs+0x3c),*(float*)(vs+0x4c),(*(uintptr_t*)((uint8_t*)batch+0x1f8)-*(uintptr_t*)((uint8_t*)batch+0x200))/2);
 }
 if(ortho&&effectMode.load()==3&&effectLogs.fetch_add(1)<90){
  auto* vp=(float*)(mem<uint8_t*>(0xdd02c0)+0x4068);
  auto* bp=mem<uint8_t*>(0xdd0e28);
  uint32_t w=0,h=0;auto* cache=*mem<void**>(0xdd1520);
  bool copied=cache&&fn<bool(*)(void*,uint32_t,uint32_t*,uint32_t*)>(0x8465ec)(cache,0,&w,&h);
  LOG("Ortho route=%.0f vp=%.0f,%.0f xy=%.0f,%.0f blend=%x tex=%u,%u efb=%d indices=%zu",route,vp[0],vp[1],vp[3],vp[4],*(uint32_t*)(bp+0x104),w,h,copied,(*(uintptr_t*)((uint8_t*)batch+0x1f8)-*(uintptr_t*)((uint8_t*)batch+0x200))/2);
 }
 if(ortho&&effectMode==6&&effectLogs.fetch_add(1)<300){
  auto* format=fn<uint8_t*(*)()>(0x87b11c)();unsigned stride=format?*(unsigned*)(format+8):0;
  auto* begin=*(uint8_t**)((uint8_t*)batch+0x10);auto* end=*(uint8_t**)((uint8_t*)batch+8);size_t bytes=end>=begin?size_t(end-begin):0;
  float loX=99999,loY=99999,hiX=-99999,hiY=-99999;uint32_t color=0;
  if(begin&&stride>=12&&stride<=188&&bytes<1024*1024){for(size_t n=0;n+stride<=bytes;n+=stride){float x=*(float*)(begin+n),y=*(float*)(begin+n+4);if(std::isfinite(x)&&std::isfinite(y)){loX=std::min(loX,x);loY=std::min(loY,y);hiX=std::max(hiX,x);hiY=std::max(hiY,y);}}if(stride>=16)color=*(uint32_t*)(begin+12);}
  auto* ps=(int*)pixel;
  LOG("Panel hash=%016llx used=%x asset=%s text=%d stride=%u bytes=%zu bounds=%.1f,%.1f..%.1f,%.1f vc=%08x colors=%d,%d,%d,%d;%d,%d,%d,%d;%d,%d,%d,%d;%d,%d,%d,%d blend=%x",(unsigned long long)textureHash,used,asset?asset->name:"-",gameText,stride,bytes,loX,loY,hiX,hiY,color,ps[0],ps[1],ps[2],ps[3],ps[4],ps[5],ps[6],ps[7],ps[8],ps[9],ps[10],ps[11],ps[12],ps[13],ps[14],ps[15],bp[0x41]);
 }
 bool jimmy=textureHash==0xd11cb3da0146b9f8ULL||textureHash==0x0f5ae8c61b4425b7ULL||textureHash==0x3efc8187ec12cf04ULL||textureHash==0xe2b4c90699271165ULL||textureHash==0x24c028be33bf1056ULL||textureHash==0x750fb83c889c21a7ULL||textureHash==0xf925b5cc6044cae1ULL||textureHash==0xfaa78341fcdb36f1ULL||textureHash==0x52d9c10a18519b40ULL||textureHash==0xd79c0e35a60f2740ULL||textureHash==0xb270ab45a82a4831ULL;
 size_t drawIndices=(*(uintptr_t*)((uint8_t*)batch+0x1f8)-*(uintptr_t*)((uint8_t*)batch+0x200))/2;
 // Skin and uniform textures are shared by other students. Restrict the
 // first-person body filter to geometry beside the player, never every NPC
 // that happens to use the same texture.
 jimmy|=std::binary_search(std::begin(playerClothingHashes),std::end(playerClothingHashes),textureHash);
 bool localPlayerBody=false;
 if(jimmy&&game&&!ortho&&!cinema&&eyeAnchorValid){
  auto* vs=fn<uint8_t*(*)(void*)>(0x6b3b64)((void*)(base+0xdec9c8));
  V d{*(float*)(vs+0x2c)-eyeAnchorX.load(),*(float*)(vs+0x3c)-eyeAnchorY.load(),*(float*)(vs+0x4c)-eyeAnchorZ.load()};
   localPlayerBody=std::hypot(d.x,d.z)<.90f&&std::abs(d.y)<2.3f;
 }
 bool nativeHeldObject=false;
 if(game&&!ortho&&!cinema&&eyeAnchorValid&&weaponCurrent>0&&actualWeapon!=437){
  auto item=std::lower_bound(std::begin(weaponTextures),std::end(weaponTextures),textureHash,[](const WeaponTexture& a,uint64_t b){return a.hash<b;});
  while(item!=std::end(weaponTextures)&&item->hash==textureHash&&item->index!=weaponCurrent.load())++item;
  if(heldTexture||(item!=std::end(weaponTextures)&&item->hash==textureHash&&item->index==weaponCurrent.load())){
   auto* vs=fn<uint8_t*(*)(void*)>(0x6b3b64)((void*)(base+0xdec9c8));
   V delta{*(float*)(vs+0x2c)-eyeAnchorX.load(),*(float*)(vs+0x3c)-eyeAnchorY.load(),*(float*)(vs+0x4c)-eyeAnchorZ.load()};
   nativeHeldObject=length(delta)<2.1f;
   if(nativeHeldObject){static int hidden=-1;if(hidden!=weaponCurrent){hidden=weaponCurrent;LOG("Original held mesh hidden for VR weapon=%d",hidden);}}
  }
 }
 bool backingTest=effectMode==5&&game&&!cinema&&!mem<uint8_t>(0x170b4ef)&&ortho&&!textureHash&&!used&&drawIndices<=5;
 bool flatHudBacking=false,compactHudBacking=false;
 if(transparentHudBacking&&game&&!cinema&&!originalMenu&&!minigameActive&&!mem<uint8_t>(0x170b4ef)&&ortho&&(!used||smallPlain)&&!asset&&!gameText){
  auto* format=fn<uint8_t*(*)()>(0x87b11c)();unsigned stride=format?*(unsigned*)(format+8):0;
  auto* begin=*(uint8_t**)((uint8_t*)batch+0x10);auto* end=*(uint8_t**)((uint8_t*)batch+8);
  if(begin&&stride>=16&&stride<=64&&end-begin==ptrdiff_t(stride*4)){
   HudQuadSample panel[4];for(int i=0;i<4;i++){auto* at=begin+i*stride;panel[i]={*(float*)at,*(float*)(at+4),*(uint32_t*)(at+12)};}
   auto* projection=(float*)(mem<uint8_t*>(0xdd02c0)+0x4080);
   flatHudBacking=isDarkHudPanel(panel,4,projection[0],projection[2]);
   float loX=99999,loY=99999,hiX=-99999,hiY=-99999;bool black=true;
   for(int v=0;v<4;v++){
    auto* vertex=begin+v*stride;float x=*(float*)vertex,y=*(float*)(vertex+4);uint32_t color=*(uint32_t*)(vertex+12);unsigned alpha=color>>24;
    black&=std::isfinite(x)&&std::isfinite(y)&&(color&0xffffffu)==0&&alpha>0&&alpha<200;
    loX=std::min(loX,x);hiX=std::max(hiX,x);loY=std::min(loY,y);hiY=std::max(hiY,y);
   }
   float width=hiX-loX,height=hiY-loY;unsigned alpha=*(uint32_t*)(begin+12)>>24;
   flatHudBacking|=black&&width>3.8f&&height>2.1f;
   // Tutorial/help messages use a smaller 4-vertex translucent charcoal
   // panel.  Remove only that uniform backing; the following text batches
   // remain untouched and keep the information readable.
   compactHudBacking=(flatHudBacking&&width<1.6f&&height<.8f)||(black&&alpha>=32&&alpha<=120&&width>1.15f&&width<1.95f&&height>.25f&&height<.75f);
   if(compactHudBacking){
    currentHudGroup=2;
    auto* projection=(float*)(mem<uint8_t*>(0xdd02c0)+0x4080);
    float center=projection[0]*(loX+hiX)*.5f+projection[1];
    if(std::isfinite(center)&&std::abs(center)<1.5f){messageCenter=center;messagePanelMeasured=true;}
   }
   if(flatHudBacking||compactHudBacking){static bool logged=false;if(!logged){LOG("Transparent HUD: removed backing alpha=%u size=%.2fx%.2f",alpha,width,height);logged=true;}}
  }
 }
 // particle.txd/coronastar is a flat-screen lens flare, not a world light.
 // Reprojecting these quads into VR leaves bright points floating in view.
 bool flatLensFlare=game&&!cinema&&ortho&&textureHash==0x0be6f5f3d9400393ULL;
 bool nativeReticle=game&&!cinema&&equipmentKnown&&actualWeapon!=-1&&ortho&&asset&&(!strcmp(asset->name,"HUD_xhair")||!strcmp(asset->name,"HUD_center_xhair"));
 bool skip=nativeReticle||flatLensFlare||flatHudBacking||compactHudBacking||nativeHeldObject||backingTest||(game&&!cinema&&mem<uint8_t>(0xde4ef0)&&!ortho&&localPlayerBody)||background||(game&&ortho&&route==0.f&&effectMode.load()==1);
 if(skip){unsigned n=++effectSkips;if(n==1||n==120)LOG("Suppressed incompatible stereo EFB effect (%u)",n);}
 // Map tiles, circular mask, frame, icons and text must share the exact same plane.
 hudBatch=game&&!cinema&&ortho&&!background;hudAssetBatch=asset;
 radarBatch=false;
 if(asset){std::string_view n(asset->name);radarBatch=n.find("radar")!=n.npos||n.find("Radar")!=n.npos||n.find("Rdr")!=n.npos||n.find("map")!=n.npos||n.find("Map")!=n.npos;}
 bool crosshair=false;
 if(ortho&&asset){
  std::string_view n(asset->name);
  if(n=="HUD_xhair"||n=="HUD_center_xhair"||n=="HUD_Scope"||n=="HUD_Scope_tar")crosshair=true;
  else if(radarBatch||n.starts_with("Bar_")||n.starts_with("HUDIcon")||n.starts_with("HUDicon")||n=="HUD_clock"||n=="HUD_ArmHour"||n=="HUD_ArmMinute")currentHudGroup=1;
  else if(n=="HUD_winmid"||n=="HUD_winright")currentHudGroup=2;
  else if(n.starts_with("Wii_")||n.starts_with("Button_"))currentHudGroup=3;
 }
 batchHudGroup=crosshair?0:currentHudGroup;
 if(gameText&&batchHudGroup>=2&&!messagePanelMeasured){
  auto* format=fn<uint8_t*(*)()>(0x87b11c)();unsigned stride=format?*(unsigned*)(format+8):0;
  auto* begin=*(uint8_t**)((uint8_t*)batch+0x10);auto* end=*(uint8_t**)((uint8_t*)batch+8);
  if(begin&&stride>=12&&stride<=188&&end>begin&&end-begin<=ptrdiff_t(stride*1024)){
   float lo=1e9f,hi=-1e9f;for(auto* p=begin;p+stride<=end;p+=stride){float x=*(float*)p;if(std::isfinite(x)){lo=std::min(lo,x);hi=std::max(hi,x);}}
   auto* projection=(float*)(mem<uint8_t*>(0xdd02c0)+0x4080);float center=projection[0]*(lo+hi)*.5f+projection[1];
   if(std::isfinite(center)&&std::abs(center)<2.f)messageCenter=center;
  }
 }
 hudTextBatch=gameText;
 effectBatch=skip;
 originalDraw(batch,pixel,geom,custom,data,size,primitive,pipeline);hudBatch=false;hudAssetBatch=false;hudTextBatch=false;skyBatch=false;effectBatch=false;
}
static std::atomic<bool> weaponObserved{false};
static void observeHudTexture(const char* name){
 if(equipmentKnown)return;
 if(mem<uint8_t>(0x170b4ef)||wheelOpen)return;
 for(int k=0;k<weaponCount;k++)if(!strcasecmp(name,weaponIcons[k])){
  int previous=weaponCurrent.exchange(k);weaponObserved=true;
  if(previous!=k)LOG("Game equipped: %s",weaponNames[k]);

  return;
 }
}
using Commit=void(*)(void*);static Commit originalCommit;
static void restoreHudOffsets(){
 static bool loaded=false;if(loaded||!gameReady)return;loaded=true;
 FILE* file=fopen(BULLY_FILES "/Config/BullyVR.ini","r");if(!file)return;
 struct Setting{const char* name;uintptr_t offset;};
 constexpr Setting settings[]={{"HudRadarX",0x1741600},{"HudRadarY",0x1741604},{"HudMissionX",0x1741608},{"HudMissionY",0x174160c},{"HudInstructionsX",0x1741610},{"HudInstructionsY",0x1741614}};
 char line[160],key[80];float value;
 while(fgets(line,sizeof(line),file))if(sscanf(line," %79[^ =] = %f",key,&value)==2&&std::isfinite(value))for(const auto& setting:settings)if(strcmp(key,setting.name)==0)mem<float>(setting.offset)=std::clamp(value,-3.f,3.f);
 fclose(file);loadVrSettings();
}
static void commit(void* manager){
 ++vrFrameCount;static double previousCommit=0;double frameNow=now();if(previousCommit&&frameNow-previousCommit>largestFrameGap)largestFrameGap=frameNow-previousCommit;previousCommit=frameNow;
 ++gameFrame;publishTestInput();
 rigManager=manager;
 auto* p=(uint8_t*)manager;
 // The pinned commit function also maintains the original VR input/settings.
 // Discard only its animated anchor target, never the whole commit function.
 if(gameReady&&!cinema&&nativeCameraEnabled){p[0xe0]=0;p[0xf0]=0;}
 originalCommit(manager);
 if(gameReady&&!cinema&&nativeCameraEnabled){p[0xe0]=0;p[0xf0]=1;float zero[3]={};memcpy(p+0x104,zero,12);memcpy(p+0xf4,zero,12);}
 restoreHudOffsets();auto s=snapshot();
 V home{*(float*)(p+0xb38),*(float*)(p+0xb3c),*(float*)(p+0xb40)};
 if(std::isfinite(home.x)&&std::isfinite(home.y)&&std::isfinite(home.z)){trackingHomeX=home.x;trackingHomeY=home.y;trackingHomeZ=home.z;}
 mem<uint8_t*>(0xdd0e38)[0x198]=skyboxDetection?1:0;
 // Our root/eye anchor remains valid even when the native head mesh is culled.
 // Depending on its texture anchor disabled controls and reset the view mid-game.
 bool active=mem<uint8_t>(0x170b4ee)&&mem<uint8_t>(0xde4ef0)&&eyeAnchorValid&&s.active&&s.head.valid;
 active=active&&!cinema&&gameReady;
 cameraActive=active;static double last=0;double t=now();float dt=last?std::clamp(float(t-last),0.f,.05f):0;last=t;
 static locomotion::Turn turn;
 if(!active){if(cinema||!gameReady){
  turn={};cameraUserTurn=0;float identity[9]={1,0,0,0,1,0,0,0,1};memcpy(p+0x158,identity,sizeof(identity));p[0x17c]=1;
  float zero[3]={};memcpy(p+0x104,zero,12);memcpy(p+0xf4,zero,12);p[0xf0]=0;rigYaw=0;
 }return;}
 if(eyeAnchorValid){float anchor[3]={eyeAnchorX.load(),eyeAnchorY.load(),eyeAnchorZ.load()};memcpy(p+0x104,anchor,12);memcpy(p+0xf4,anchor,12);p[0xf0]=1;*(int*)(p+0x100)=0;}
 bool turning=!mem<uint8_t>(0x170b4ef)&&!wheelOpen&&!helpOpen&&!settingsOpen&&!travelOpen&&!s.c[0].squeeze&&!s.c[0].trigger&&!s.c[0].secondary&&s.c[1].connected;
 float stickTurn=turn.step(s.c[1].x,dt,turning,vrTurnSpeed.load(),vrTurnSmoothing.load());cameraUserTurn=stickTurn;
 float userTurn=stickTurn;
 rigYaw=userTurn;
 const float c=std::cos(userTurn),sn=std::sin(userTurn);float rot[9]={c,0,-sn,0,1,0,sn,0,c};
 memcpy(p+0x158,rot,sizeof(rot));p[0x17c]=1;
}
using Input=uint64_t(*)(void*);static Input originalInput;
#include "charged_punch.h"
static uint64_t input(void* device){
 publishTestInput();
 // Do not expose the fist combo during the few frames between a weapon request
 // and the native inventory/model state catching up.
 int selected=weaponCurrent.load();
 bool selectedArmed=(selected>0&&selected<weaponCount)||
  (pendingWeapon.load()!=-2&&pendingWeapon.load()!=-1)||queuedWeapon.load()>=0||
  (equipmentKnown.load()&&actualWeapon.load()!=-1);
 mem<uint8_t>(0xde4688)=!selectedArmed;
 auto result=originalInput(device);auto* s=(Snapshot*)((uint8_t*)device+0x40);
 Snapshot simulated;if(testInput(simulated))*s=simulated;
 bool menu=mem<uint8_t>(0x170b4ef);auto& l=s->c[0];auto& r=s->c[1];
 static EquipmentButtons equipmentButtons;static TriggerRelease weaponTrigger;
 if(unifiedMenuInput(*s)){
  equipmentButtons.step(false,false,now(),false);weaponTrigger.step(actualWeapon.load(),false,false);
  *((uint8_t*)device+0x212)=*((uint8_t*)device+0x213)=0;return result;
 }
 bool open=false;
 if(menu||cinema){open=false;helpOpen=false;pendingWeapon=-2;}
 bool equipmentAllowed=gameReady&&!menu&&!helpOpen&&!cinema;
 int equipmentAction=equipmentButtons.step(l.squeeze,r.squeeze,now(),equipmentAllowed);
 if(equipmentAction){
  int desired=queuedWeapon.load();
  if(desired<0){desired=weaponCurrent.load();if(pendingWeapon!=-2)for(int i=0;i<weaponCount;i++)if(weaponIds[i]==pendingWeapon)desired=i;}
  if(equipmentAction==2)requestWeapon(0);
  else requestWeapon(nextOwnedEquipment(desired,equipmentAction,weaponCount,[](int item){return wheel::canEquip(item,weaponStock[item],weaponAmmo[item]);}));
 }
 // Never leak physical inventory grips into native skateboard, jump or attack
 // bindings, including frames where the first-person anchor is reacquiring.
 if(gameReady&&!cinema){
  // L2 holds native C: focus and interaction selector. Y sends Z: grab.
  l.squeeze=false;l.sv=0.f;
  r.squeeze=false;r.sv=0.f;l.sf=r.sf=0.f;
 }
 wheelOpen=open;
 selectedArmed=selectedArmed||(pendingWeapon.load()!=-2&&pendingWeapon.load()!=-1)||queuedWeapon.load()>=0;
 bool weaponIntent=selectedArmed;
 bool weaponReady=equipmentKnown.load()&&actualWeapon.load()!=-1;
 if(weaponIntent&&!weaponReady){r.trigger=false;r.tv=0.f;}
 bool ranged=actualWeapon!=-1&&!weaponMelee[weaponCurrent.load()]&&actualWeapon!=437;
 // originalInput generates motion/combination pulses before this hook reads
 // the equipped item. Clear those cached pulses as well as disabling combos.
 if(selectedArmed||ranged){*((uint8_t*)device+0x212)=0;*((uint8_t*)device+0x213)=0;}
 auto trigger=weaponTrigger.step(actualWeapon.load(),r.trigger||r.tv>.25f,gameReady&&!menu&&!helpOpen&&!cinema&&ranged);
 if(ranged){r.trigger=trigger.down;r.tv=trigger.down?1.f:0.f;}
 static double slingHeldAt=0;
 bool sling=(actualWeapon==303||actualWeapon==306)&&trigger.down;
 if(sling){if(slingHeldAt==0)slingHeldAt=now();slingChargeProgress=std::clamp(float((now()-slingHeldAt)/1.5),0.f,1.f);}
 else {slingHeldAt=0;slingChargeProgress=0;}
 static ChargedPunch heavyPunch;
 float handForward[2]{};bool tracked[2]{};
 if(s->head.valid){
  V forward=rotate(s->head.q,{0,0,-1});
  for(int h=0;h<2;h++){
   auto& hand=s->c[h];V delta{hand.grip.p[0]-s->head.p[0],hand.grip.p[1]-s->head.p[1],hand.grip.p[2]-s->head.p[2]};
   tracked[h]=hand.connected&&hand.grip.valid&&length(delta)<1.1f&&delta.y>-.65f;
   handForward[h]=dot(delta,forward);
  }
 }
 auto heavy=heavyPunch.step(handForward,tracked,now(),gameReady&&!menu&&!helpOpen&&!cinema&&s->head.valid&&!selectedArmed&&actualWeapon==-1);
 if(heavy.attack){r.trigger=true;r.tv=1.f;}
 for(int h=0;h<2;h++)if(heavy.suppress[h]||heavy.punch[h])*((uint8_t*)device+0x212+h)=heavy.punch[h]?1:0;
 static AimMode aimMode;
 float eyeDistance=length(V{r.grip.p[0]-s->head.p[0],r.grip.p[1]-s->head.p[1],r.grip.p[2]-s->head.p[2]});
 bool eyeSight=weaponCurrent==3&&r.grip.valid&&s->head.valid&&eyeDistance<.27f;
 bool canFreeAim=actualWeapon.load()!=-1&&!weaponMelee[weaponCurrent.load()]&&weaponIds[weaponCurrent.load()]!=437;
 bool wantsFreeAim=cameraActive&&!menu&&!open&&!settingsOpen&&canFreeAim&&(r.trigger||r.tv>.25f||eyeSight);
 auto aimState=aimMode.step(wantsFreeAim,now(),cameraActive&&!menu&&!open&&!settingsOpen&&!cinema&&canFreeAim);
 if(freeAim.exchange(aimState.manual)!=aimState.manual)LOG("RT free aim=%d",aimState.manual);
 // Ranged targeting is suppressed in the native targeting module; keep L2
 // intact so doors and held interactions still work while a weapon is equipped.
 // Aim is a VR sight and a native projectile redirect. Never send the old
 // Up/Down free-aim commands: they switch to camera 13 and detach the view.
 static locomotion::Walk walk;static double lastMove=0;
 double movementTime=now();float movementDt=lastMove?float(movementTime-lastMove):0.f;lastMove=movementTime;
 // Retain the native Wii throw/strike animations and physics. Fast grip release while
 // aiming activates the native use action for throwable items; an ordinary grip is melee.
 static bool heldRightGrip=false,heldLeftGrip=false,oldThrowStick=false;static double throwUntil=0,grappleThrowUntil=0,grappleCooldown=0;
 bool rightGrip=r.squeeze,leftGrip=l.squeeze;
 int equipped=weaponCurrent.load();bool throwable=equipped==1||equipped==4||equipped==5||equipped==6||equipped==8||equipped==10||equipped==11;
 auto speed=[](const Controller& c){return c.vel.lv?std::sqrt(c.vel.linear[0]*c.vel.linear[0]+c.vel.linear[1]*c.vel.linear[1]+c.vel.linear[2]*c.vel.linear[2]):0.f;};
 bool throwStick=l.secondary&&r.y>.65f&&equipped==0;
 if(cameraActive&&!menu&&!open&&(l.secondary||wantsFreeAim)){
  V leftVelocity{l.vel.linear[0],l.vel.linear[1],l.vel.linear[2]};
  V headForward=s->head.valid?rotate(s->head.q,{0,0,-1}):V{0,0,-1};
  bool physicalPush=l.secondary&&l.vel.lv&&dot(leftVelocity,headForward)>1.f;
  if(equipped==0&&movementTime>grappleCooldown&&((throwStick&&!oldThrowStick)||physicalPush)){grappleThrowUntil=movementTime+.16;grappleCooldown=movementTime+.5;}
  if(heldRightGrip&&!rightGrip&&throwable&&speed(r)>1.2f)throwUntil=movementTime+.14;
  if(heldLeftGrip&&!leftGrip&&speed(l)>1.2f)grappleThrowUntil=movementTime+.16;
 }else throwUntil=grappleThrowUntil=0;
 oldThrowStick=throwStick;if(throwStick)r.y=0;
 heldRightGrip=rightGrip;heldLeftGrip=leftGrip;
 // Aim/use and melee remain distinct; never swallow the original fist-combo pulses.
 if(l.secondary&&throwable&&equipmentKnown){*((uint8_t*)device+0x213)=0;}
 if(movementTime<throwUntil){r.trigger=true;r.tv=1.f;}
 if(movementTime<grappleThrowUntil){l.squeeze=true;l.sv=1.f;*((uint8_t*)device+0x212)=1;}
 if(cameraActive&&!menu&&!open&&canFreeAim&&r.aim.valid){
  // Native attacks must not be cancelled by the old virtual-screen IR bounds.
  // The real ray is applied to the final native projectile, independently.
  r.hit={true,0.f,0.f,2.f};
  // IMUIR also reads the shared OpenXR snapshot independently of this device.
  auto* mutex=&mem<pthread_mutex_t>(0xdec9e0);pthread_mutex_lock(mutex);
  auto& shared=mem<Snapshot>(0xdd7f28);shared.c[1].hit=r.hit;s->generation=++shared.generation;
  pthread_mutex_unlock(mutex);
 }
 if(cameraActive){
  auto v=walk.step({l.x,l.y},s->head.valid?yaw(s->head.q):0.f,rigYaw.load(),movementDt,
                   !menu&&!open&&!helpOpen&&s->active&&l.connected&&s->head.valid);
  // Wii A is run.  Keep the native button visible to the game and scale only
  // the locomotion vector so sprinting works with the VR left stick too.
  if(r.primary){v.x=std::clamp(v.x*1.45f,-1.f,1.f);v.y=std::clamp(v.y*1.45f,-1.f,1.f);}
  if(actualWeapon==437){
   // A skateboard uses lateral steering and forward pushing, rather than
   // the walking vector. Head turns must not become steering or throttle.
   auto skate=locomotion::deadzone({l.x,l.y},.18f);l.x=skate.x;l.y=skate.y;walk.reset();
  }else {l.x=v.x;l.y=v.y;}
 }else walk.reset();
 if(open){l.x=l.y=r.x=r.y=0;l.primary=l.secondary=r.primary=r.secondary=l.trigger=r.trigger=l.squeeze=r.squeeze=false;l.tv=r.tv=l.sv=r.sv=0;}
 // Physical R3 was consumed above. Only the synthetic aim-entry pulse reaches
 // D-pad Up; X remains available for the game's context-sensitive use action.
 // Inventory requests execute on the emulated CPU through the game's item loader.
 l.sf=r.sf=0.f;
 if(pendingWeapon!=-2&&!open&&!menu&&!cinema){
  l.trigger=r.trigger=l.squeeze=r.squeeze=l.primary=r.primary=false;
  l.tv=r.tv=l.sv=r.sv=0;l.x=l.y=r.x=r.y=0;
  *((uint8_t*)device+0x212)=0;*((uint8_t*)device+0x213)=0;
 }
 // Circular mini-game inputs must remain circular and bypass locomotion.
 // The right stick can operate the dial while the left stick remains available.
 if(minigameActive&&std::hypot(r.x,r.y)>.18f){l.x=r.x;l.y=r.y;r.x=r.y=0;}

 return result;
}
using Render=void(*)(void*,uint64_t,uint64_t);static Render originalRender;
using Geometry=void(*)(float*,int);static Geometry originalGeometry;
static void publishTestEyeViews(){
 auto* xr=*mem<uint8_t**>(0xdcfc88);if(!xr)return;
 static bool simulating=false;static float savedHome[3]{};
 Snapshot simulated;
 if(!testInput(simulated)){
  if(simulating){memcpy(xr+0xb38,savedHome,sizeof(savedHome));simulating=false;}
  return;
 }
 // A synthetic head at 1.6 m needs the same neutral home as a real tracked
 // head. Otherwise the test adds 1.6 m to the camera and to shot origins.
 if(!simulating){memcpy(savedHome,xr+0xb38,sizeof(savedHome));memcpy(xr+0xb38,simulated.head.p,sizeof(savedHome));simulating=true;}
 // The overlay hook runs after the world has already rendered. Inject into
 // both live and frame-locked eye views before world projection is generated.
 for(unsigned start:{0x668u,0x718u,0xac0u})for(int eye=0;eye<2;eye++){
  auto* view=(float*)(xr+start+eye*0x2c);memcpy(view,&simulated.head.q,16);
  V offset=rotate(simulated.head.q,{eye?.032f:-.032f,0,0});
  for(int axis=0;axis<3;axis++)view[4+axis]=simulated.head.p[axis]+(&offset.x)[axis];
 }
}
static void geometry(float* constants,int primitive){
 publishTestEyeViews();
 // Camera-anchor rotation is changed outside the original texture detector.
 // Refresh cached stereo rows once per game frame after those changes.
 static uint64_t projectionFrame=~uint64_t(0);
 if(projectionFrame!=gameFrame){
  *(uint8_t*)((uint8_t*)constants+0x168)=1;
  *(uint8_t*)((uint8_t*)constants+0x210)=1;
  projectionFrame=gameFrame;
 }
 originalGeometry(constants,primitive);
 if(!mem<uint8_t>(0x170b4ee))return;
 // The director and original menus already compose a complete native image.
 // Bypass stereo reprojection for that composition; routing every world draw
 // onto the HUD screen also reprojects its EFB copy a second time.
 if(!gameReady||cinema){constants[3]=0.f;constants[0x41]=1.f;constants[0x42]=constants[0x43]=0.f;*(uint8_t*)((uint8_t*)constants+0x130)=1;return;}
 if(skyBatch&&skyMode){constants[2]=0.f;if(skyMode==2)constants[3]=0.f;*(uint8_t*)((uint8_t*)constants+0x130)=1;return;}
 // Keep the game's HUD composition on one head-locked plane, with its native positions.
 constants[0x41]=1.f;constants[0x42]=0.f;constants[0x43]=0.f;
 if(constants[3]<-.5f&&constants[3]>-3.f)constants[3]=-2.f;
 if(hudBatch){
  float scale=std::clamp(mem<float>(0xde4f04),.4f,2.f),distance=std::clamp(mem<float>(0xde4ed8),.75f,6.f);
  // Size, placement and depth are shared by every draw in a logical HUD group.
  if(batchHudGroup>=2)scale*=std::clamp(mem<float>(0xde4f08),.5f,2.f);
  // Original GS derives every ortho layer from ScreenDistance, not HudDistance.
  float upm=constants[0x2e]/std::max(mem<float>(0xde4edc),.01f);if(!std::isfinite(upm)||upm<.001f)upm=1.f;
  constants[0x2d]=upm*scale;constants[0x2c]=upm*scale*1.7777778f;constants[0x2e]=upm*distance;
  float ox=0.f,oy=0.f;
  // Keep all radar components together; text has its own mission/instruction adjustment.
  if(batchHudGroup==1){
   ox=-.65f+std::clamp(mem<float>(0x1741600),-3.f,3.f);
   oy=std::clamp(mem<float>(0x1741604),-3.f,3.f);
  }else if(batchHudGroup==2){
   ox=mem<float>(0x1741608)+messageCenter*constants[0x2c];oy=mem<float>(0x174160c);
  }else if(batchHudGroup==3){
   ox=messageCenter*constants[0x2c]+mem<float>(0x1741610);oy=mem<float>(0x1741614);
  }else{
   // Aim reticles are camera-centred overlays; never inherit mission/radar offsets.
   ox=oy=0.f;
  }
  for(int eye=0;eye<2;eye++){constants[0x32+eye*8]-=ox;constants[0x36+eye*8]-=oy;}
  *(uint8_t*)((uint8_t*)constants+0x130)=1; // Uniform upload must include these adjustments.
 }
}
static bool modelDrawn=false;static uint32_t currentEye=0;
static void render(void* ui,uint64_t vr,uint64_t eye){
 modelDrawn=false;currentEye=eye;publishTestInput();publishTestEyeViews();Snapshot simulated;
 // The original overlay also gates hands/menus on its transient texture
 // heuristic (0x808474). Streaming can clear it during valid gameplay.
 // Use the verified player/game state for this overlay call only.
 uint8_t previousWorldHint=mem<uint8_t>(0x173915c);
 if(gameReady&&!cinema)mem<uint8_t>(0x173915c)=1;
 auto* xr=*mem<uint8_t**>(0xdcfc88);
 if(xr&&testInput(simulated)){
  auto* view=(float*)(xr+(xr[0xb18]?0xac0:0x718)+(eye&1)*0x2c);
  float saved[7];memcpy(saved,view,sizeof(saved));memcpy(view,&simulated.head.q,16);
  V offset=rotate(simulated.head.q,{eye?.032f:-.032f,0,0});
  for(int axis=0;axis<3;axis++)view[4+axis]=simulated.head.p[axis]+(&offset.x)[axis];
  originalRender(ui,vr,eye);captureFramebuffer(eye==0);memcpy(view,saved,sizeof(saved));
 }else {originalRender(ui,vr,eye);captureFramebuffer(eye==0);}
 mem<uint8_t>(0x173915c)=previousWorldHint;
}
using Capsule=void(*)(float,float,float,float,float,float,float,float,ImDrawList*,void*,uint32_t,uint32_t);static Capsule originalCapsule;
using Project=bool(*)(float,float,float,void*,ImVec2*);
// Author the grip point in model coordinates rather than centering every object.
// Wii models use Z up and -Y forward; OpenXR uses Y up and -Z forward.
static V weaponPoint(V p,const Mesh& mesh,int weapon,const Snapshot& s){
 V mid=(mesh.lo+mesh.hi)*.5f,q;
 if(weapon==2||weapon==3){
  // HANDLE is the authored socket, measured in the recovered model's skeleton.
  V grip{-.11942430f,.51287103f,-.29034689f};p=p-grip;
  // The recovered model's X axis is the band span.  The old path scaled Y,
  // which pushed the elastic into depth and made it pull to the wrong side.
  // Use the tracked support hand to set a stable two-hand span while keeping
  // the handle socket fixed in the firing hand.
  // Only the elastic extends when RT is held. The wooden fork keeps its
  // authored size; the recovered mesh was baked in its fully drawn pose.
  if(p.y<-.08f){float pull=.08f+.27f*std::clamp(s.c[1].tv,0.f,1.f);p.y=-.08f+(p.y+.08f)*(pull/.65f);}
  q={p.x,p.z,-p.y-.025f};
 }else if(weapon==1||weapon==10||weapon==11){p=p-V{mid.x,mid.y,mesh.lo.z+.04f};q={p.x,p.z+.015f,-p.y-.09f};}
 else if(weapon==7){p=p-mid;q={p.x,p.z+.025f,-p.y-.045f};}
 else if(weaponIds[weapon]==305){
  // Rotate the Spudg body 180 degrees around local up at its hand socket;
  // its source mesh points the barrel opposite the Touch grip direction.
  p=p-V{.10f,.05f,.10f};q={-p.x,-p.y,p.z-.045f};
 }
 else if(weaponIds[weapon]==396){p=p-V{mid.x,.16f,1.07f};q={p.x,p.z,-p.y-.08f};}
 else if(weaponIds[weapon]==307){p=p-V{0,.03f,-.04f};q={p.x,p.z,-p.y-.08f};}
 else if(weapon==12){p=p-V{mid.x,mid.y,mesh.hi.z-.08f};q={p.x,p.z+.04f,-p.y-.06f};}
 else if(weapon==13){p=p-V{mid.x,mid.y,mesh.lo.z+.10f};q={p.x,p.z,-p.y-.04f};}
 else if(weapon==6){p=p-V{mesh.lo.x+.09f,mid.y,mid.z};q={-p.y,p.z,-p.x-.035f};}
 else if(weapon==9){p=p-mid;q={p.x-.055f,p.z+.035f,-p.y-.055f};}
 else if(weapon>=14&&weaponMelee[weapon]){p=p-V{mid.x,mid.y,mesh.lo.z+.10f};q={p.x,p.z,-p.y-.04f};}
 else{p=p-mid;q={p.x,p.z,-p.y-.045f};}
 const auto& hand=s.c[1];Q orientation=hand.aim.valid&&weapon!=13?hand.aim.q:hand.grip.q;
 return V{hand.grip.p[0],hand.grip.p[1],hand.grip.p[2]}+rotate(orientation,q);
}
#include "hands.h"
#include "wristwatch.h"
#include "avatar_render.h"
#include "hud_style.h"
#include "unified_menu_render.h"
static void drawControls(ImDrawList* dl,void* ctx){
#if BULLY_DEMO
 if(demoComplete)centeredText(dl,{*(float*)((uint8_t*)ctx+8)*.5f,*(float*)((uint8_t*)ctx+12)*.12f},22.f,wheelStyle::gold,"DEMO CONCLUIDA - Explore livremente a escola");
#endif
 if(!helpOpen)return;int first=dl->VtxBuffer.Size;float w=*(float*)((uint8_t*)ctx+8),h=*(float*)((uint8_t*)ctx+12),iw=std::min(w*.80f,h*1.35f),ih=iw*.65f;
 dl->AddRectFilled({(w-iw)/2,(h-ih)/2},{(w+iw)/2,(h+ih)/2},wheelStyle::ink,10.f);
 const char* lines[]={"BULLY VR - QUEST",tr("AE: andar | AD: virar","Left stick: walk | Right stick: turn"),tr("A: correr / skate | B: pular / ollie","A: run / skate | B: jump / ollie"),tr("X: pausa | Y: agarrar (segure)","X: pause | Y: grab (hold)"),tr("L2: segure foco / seletor | AD: opcoes","L2: hold focus / selector | Right stick: options"),tr("RT: combo de socos / mira livre e usar arma","RT: punch combo / free aim and weapon use"),tr("Grips: trocar | Segure grip direito: punhos","Grips: cycle | Hold right grip: fists"),tr("L3: menu VR com abas | R3: controles","L3: tabbed VR menu | R3: controls"),tr("Mova as maos para socar / resistir a agarrar","Move hands to punch / resist grabs")};
 for(int i=0;i<9;i++)centeredText(dl,{w*.5f,(h-ih)/2+ih*.09f+i*ih*.095f},std::min(w,h)*.022f,i?wheelStyle::paper:wheelStyle::gold,lines[i]);
 centeredText(dl,{w*.5f,(h+ih)/2+25},22.f,IM_COL32_WHITE,tr("R3: fechar controles e voltar ao jogo","R3: close controls and return to game"));
 for(int i=first;i<dl->VtxBuffer.Size;i++){
  auto& p=dl->VtxBuffer[i].pos;p.x=w*.5f+(p.x-w*.5f)*.68f;p.y=h*.5f+(p.y-h*.5f)*.68f;
 }
}
static void drawAim(ImDrawList* dl,void* ctx,const Snapshot& s){
 if(!freeAim||travelOpen||settingsOpen||helpOpen||!physicalAimDisplayValid.load())return;
 // Render the original game's reticle on the same physical ray supplied to
 // Wii IR. It must never inherit the radar/instruction panel's offsets.
 auto* eye=*(float**)ctx;if(!eye)return;
 V origin{physicalAimDisplayOrigin[0].load(),physicalAimDisplayOrigin[1].load(),physicalAimDisplayOrigin[2].load()};
 V direction{physicalAimDisplayDirection[0].load(),physicalAimDisplayDirection[1].load(),physicalAimDisplayDirection[2].load()};
 V world=origin+direction*30.f;
 Q inverse{-eye[0],-eye[1],-eye[2],eye[3]};V point=rotate(inverse,world-V{eye[4],eye[5],eye[6]});
 if(point.z>=-.1f)return;
 float left=std::tan(eye[7]),right=std::tan(eye[8]),up=std::tan(eye[9]),down=std::tan(eye[10]);
 float nx=(2*point.x/-point.z-(right+left))/(right-left),ny=(2*point.y/-point.z-(up+down))/(up-down);
 if(!std::isfinite(nx)||!std::isfinite(ny)||std::abs(nx)>1||std::abs(ny)>1)return;
 float w=*(float*)((uint8_t*)ctx+8),h=*(float*)((uint8_t*)ctx+12),radius=std::min(w,h)*.016f;
 ImVec2 at{w*(nx+1)*.5f,h*(1-ny)*.5f};
 if(auto* reticle=texture("HUD_xhair"))dl->AddImage(ImTextureRef((ImTextureID)(uintptr_t)reticle),{at.x-radius,at.y-radius},{at.x+radius,at.y+radius});
 // Reuse the game's meter border beside the VR reticle. Progress follows the
 // held input; the Wii still computes projectile power itself (not this UI).
 if((actualWeapon==303||actualWeapon==306)&&slingChargeProgress>0){
  float width=radius*4.f,height=std::max(5.f,radius*.30f),y=at.y+radius*1.6f;
  ImVec2 a{at.x-width*.5f,y},b{at.x+width*.5f,y+height};
  dl->AddRectFilled(a,b,IM_COL32(18,10,10,230));
  dl->AddRectFilled({a.x+1,a.y+1},{a.x+1+(width-2)*slingChargeProgress.load(),b.y-1},IM_COL32(220,38,35,255));
  if(auto* border=texture("Bar_outline"))dl->AddImage(ImTextureRef((ImTextureID)(uintptr_t)border),a,b);
 }
}
static void drawEquipmentNotice(ImDrawList* dl,void* ctx){
 if(now()>equipmentNoticeUntil||travelOpen||helpOpen||!equipmentKnown)return;
 int item=weaponCurrent.load();float w=*(float*)((uint8_t*)ctx+8),h=*(float*)((uint8_t*)ctx+12),u=std::min(w,h),size=u*.052f;
 ImVec2 center{w*.5f,h*.65f};
 if(weaponIcons[item][0])if(auto* icon=texture(weaponIcons[item]))dl->AddImage(ImTextureRef((ImTextureID)(uintptr_t)icon),{center.x-size,center.y-size},{center.x+size,center.y+size});
 centeredText(dl,{center.x,center.y+size*1.2f},u*.024f,wheelStyle::gold,weaponNames[item]);
}
static void drawWristTime(ImDrawList* dl,void* ctx,const Snapshot& s){
 if(travelOpen||helpOpen||!s.c[0].grip.valid||!s.head.valid)return;
 const auto& wrist=s.c[0].grip;V hand{wrist.p[0],wrist.p[1],wrist.p[2]};
 if(length(hand-V{s.head.p[0],s.head.p[1],s.head.p[2]})>.85f)return;
 auto* eye=*(float**)ctx;if(!eye)return;
 Q inverse{-eye[0],-eye[1],-eye[2],eye[3]};
 V p=rotate(inverse,hand+rotate(wrist.q,{0,.055f,.01f})-V{eye[4],eye[5],eye[6]});
 if(p.z>-.08f)return;
 float left=std::tan(eye[7]),right=std::tan(eye[8]),up=std::tan(eye[9]),down=std::tan(eye[10]);
 float nx=(2*p.x/-p.z-(right+left))/(right-left),ny=(2*p.y/-p.z-(up+down))/(up-down);
 if(!std::isfinite(nx)||!std::isfinite(ny)||std::abs(nx)>.9f||std::abs(ny)>.9f)return;
 float w=*(float*)((uint8_t*)ctx+8),h=*(float*)((uint8_t*)ctx+12),font=std::min(w,h)*.018f;
 ImVec2 at{w*(nx+1)*.5f,h*(1-ny)*.5f};unsigned time=nativeClock.load();char label[12];snprintf(label,sizeof(label),"%02u:%02u",time/60,time%60);
 dl->AddRectFilled({at.x-font*1.8f,at.y-font*.7f},{at.x+font*1.8f,at.y+font*.7f},IM_COL32(10,12,14,215),3);
 auto* f=menuFont(dl);auto extent=f->CalcTextSizeA(font,1000,0,label);dl->AddText(f,font,{at.x-extent.x*.5f,at.y-extent.y*.5f},wheelStyle::paper,label);
}
static void capsule(float x,float y,float z,float x2,float y2,float z2,float r,float r2,ImDrawList* dl,void* ctx,uint32_t c,uint32_t c2){
 if(!mem<uint8_t>(0x170b4ee)){originalCapsule(x,y,z,x2,y2,z2,r,r2,dl,ctx,c,c2);return;}
 if(cinema)return;
 if(modelDrawn)return;modelDrawn=true;auto s=snapshot();if(!s.head.valid)return;
 if(auto* avatar=renderAvatar(ctx,s)){
  float w=*(float*)((uint8_t*)ctx+8),h=*(float*)((uint8_t*)ctx+12);
  dl->AddImage(ImTextureRef((ImTextureID)(uintptr_t)avatar),{0,0},{w,h},{0,1},{1,0});
  drawAim(dl,ctx,s);drawWristTime(dl,ctx,s);drawEquipmentNotice(dl,ctx);drawControls(dl,ctx);drawTravel(dl,ctx);return;
 }
 drawAim(dl,ctx,s);drawWristTime(dl,ctx,s);drawEquipmentNotice(dl,ctx);drawControls(dl,ctx);drawTravel(dl,ctx);
}
static void menuPatches(){
 // Center the menu; the font uses a readable 1.5x..1.75x scale below.
 float pos[2]={.50f,.50f},size[2]={.82f,.88f};
 writeMem(base+0x208958,pos,8);writeMem(base+0x207fd8,size,8);
 auto label=[](uintptr_t off,const char* expected,const char* replacement){if(strcmp((char*)(base+off),expected)==0&&strlen(replacement)<=strlen(expected))writeMem(base+off,replacement,strlen(replacement)+1);};
 label(0x18190b,"Radar X","HUD X");label(0x187660,"Radar Y","HUD Y");
 label(0x193acd,"Radar tamanho","HUD tamanho");label(0x193adb,"Dinheiro/info tamanho","Textos tamanho");
 label(0x15e601,"R3  SALVAR E FECHAR","L3  SALVAR E FECHAR");
 label(0x1c9fa8,"ANALOGICO ESQUERDO: SELECIONAR E AJUSTAR","AE: SELECIONAR / AJUSTAR");
 auto patch=[](uintptr_t off,uint32_t expected,uint32_t value){if(mem<uint32_t>(off)==expected)writeMem(base+off,&value,4);else LOG("Skipped mismatched menu patch %zx",off);};
 auto movz=[](unsigned imm,unsigned shift=0){return 0x52800008u|((imm&65535)<<5)|((shift/16)<<21);};
 auto movk=[](unsigned imm,unsigned shift=0){return 0x72800008u|((imm&65535)<<5)|((shift/16)<<21);};
 patch(0x80a878,mem<uint32_t>(0x80a878),movz(0x4480,16)); // Height / 1024.
 patch(0x80a880,0x529999a8,movz(0));patch(0x80a884,0x72a80388,movk(0x3fe0,16));
 patch(0x80a88c,0x52933348,movz(0));patch(0x80a894,0x72a7fb28,movk(0x3fc0,16));
 uint32_t a=mem<uint32_t>(0x80a90c),b=mem<uint32_t>(0x80a92c);
 if(((a>>10)&4095)==7){a=(a&~(4095u<<10))|(5u<<10);writeMem(base+0x80a90c,&a,4);}
 if(((b>>10)&4095)==7){b=(b&~(4095u<<10))|(5u<<10);writeMem(base+0x80a92c,&b,4);}
 // HUD positioning: +/-3 units on both grouped panels.
 patch(0x91b670,0x528cccc8,0x52800008); // mov w8,0x6666 -> 0
 patch(0x91b674,0x72b7dcc8,0x72b80808); // overwritten below using generated movk
 // Native immediates are checked by their full instruction word.
 auto fmov=[](uintptr_t at,float from,float to){
  auto encode=[](float v){uint32_t bits;memcpy(&bits,&v,4);uint32_t imm=((bits>>19)&0x3f)|((~(bits>>30)&1)<<6)|((bits>>24)&0x80);return 0x1e201000u|(imm<<13);};
  uint32_t instruction=mem<uint32_t>(at),expected=encode(from),replacement=encode(to);
  // Preserve destination register (s1 or s2).
  if((instruction&~31u)==expected){replacement|=instruction&31u;writeMem(base+at,&replacement,4);}
 };
 // Radar lower bound is constructed in w8; replace both words atomically verified.
 uint32_t lower[]={0x52800008u,0x72b80808u};
 if(mem<uint32_t>(0x91b670)==0x52800008u){writeMem(base+0x91b670,lower,8);}
 fmov(0x91b67c,.25f,3.f);fmov(0x91b6d0,-.25f,-3.f);fmov(0x91b6f4,.25f,3.f);
 fmov(0x91b75c,-1.f,-3.f);fmov(0x91b76c,1.f,3.f);
 // Move the original R3 menu to L3 (left menu is reserved by the Quest shell).
 uint32_t menuKey=mem<uint32_t>(0x919534);
 if(((menuKey>>10)&4095)==0x256||((menuKey>>10)&4095)==0x1ce){uint32_t disabled=0x2a1f03f5;writeMem(base+0x919534,&disabled,4);}
 // Bypass the old texture-by-texture HUD relocation; preserve the game's layout.
 uint32_t skipHud=0x14000000u|(((0x8889b4u-0x888688u)/4)&0x3ffffffu);
 writeMem(base+0x888688,&skipHud,4);
}
extern "C" JNIEXPORT void JNICALL Java_org_dolphinemu_dolphinemu_recovery_Recovery_start(JNIEnv* env,jclass,jobject manager){
 assets=AAssetManager_fromJava(env,manager);patchTutorialDisc();void* main=dlopen("libmain.so",RTLD_NOW);if(!main){LOG("libmain not found");return;}
 env->GetJavaVM(&recoveryVm);bootstrapFiles();
 void* symbol=dlsym(main,"Java_org_dolphinemu_dolphinemu_NativeLibrary_RequestOpenXRRecenter");Dl_info info{};
 if(!symbol||!dladdr(symbol,&info)){LOG("Cannot resolve main base");return;}base=(uintptr_t)info.dli_fbase;
 if(mem<uint32_t>(0x918450)!=0x6db82beb){LOG("Unsupported original build");return;}
 // Dolphin upstream PR #14899: avoid Adreno's miscompiled literal TEV zero.
 // The original table entry is confirmed before substituting its zero-valued uniform.
 static const char tevZero[]="konstLookup[8].rgb";
 auto table=mem<const char*>(0xdace50);
 if(table&&strcmp(table,"int3(0,0,0)")==0){const char* replacement=tevZero;bool ok=writeMem(base+0xdace50,&replacement,sizeof(replacement));LOG("Adreno TEV zero workaround applied=%d verified=%d",ok,mem<const char*>(0xdace50)==replacement);}
 loadHands();for(int k=1;k<weaponCount;k++)loadMesh(weaponModels[k]);menuPatches();
 bool a=hook<Commit>(0x918450,commit,originalCommit,"eb2bb86de923016dfd7b02a9fc6f03a9");
 bool b=hook<Input>(0x739ef8,input,originalInput,"fd7bbaa9fc6f01a9fa6702a9f85f03a9");
 bool c=hook<Render>(0x80820c,render,originalRender,"ef3bb66ded33016deb2b026de923036d");
 bool d=hook<Capsule>(0x80c424,capsule,originalCapsule,"ffc303d1ef3b076ded33086deb2b096d");
 bool e=hook<Geometry>(0x7f514c,geometry,originalGeometry,"ff4306d1ec8300fdeb2b116de923126d");
 bool f=hook<Draw>(0x88c018,draw,originalDraw,"ff8301d1fd7b01a9fa6702a9f85f03a9");
 bool g=hook<Batch>(0x88a04c,batchDraw,originalBatch,"fd7bbda9f50b00f9f44f02a9fd030091");
 LOG("Recovery ready: menu, analog turn, wheel, Jimmy meshes=%zu hooks=%d%d%d%d%d%d%d",meshes.size(),a,b,c,d,e,f,g);
 pthread_t gameWorker;if(pthread_create(&gameWorker,nullptr,gameStateWorker,nullptr)==0)pthread_detach(gameWorker);
}
