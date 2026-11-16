// One-shot local photo staging. User save is backed up before invoking.
#include <jni.h>
#include <dlfcn.h>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <cstring>
#include "../upstream/Source/Core/Common/Functional.h"
extern "C" __attribute__((visibility("default"))) int Agent_OnAttach(JavaVM*,char* options,void*){
 float x=0,y=0,z=0;
 if(!options||sscanf(options,"%f:%f:%f",&x,&y,&z)!=3||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||std::abs(x)>2000||std::abs(y)>2000||z<0||z>100)return -1;
 void* main=dlopen("libmain.so",RTLD_NOW);Dl_info info{};
 if(!main||!dladdr(dlsym(main,"Java_org_dolphinemu_dolphinemu_NativeLibrary_RequestOpenXRRecenter"),&info))return -2;
 uintptr_t base=(uintptr_t)info.dli_fbase;
 [base,x,y,z]{
  auto memory=((void*(*)(void*))(base+0x6b3a84))((void*)(base+0xdec9c8));
  auto read=(uint32_t(*)(void*,uint32_t))(base+0x5bb354);
  auto write=(void(*)(void*,uint32_t,uint32_t))(base+0x5bb688);
  if(read(memory,0x80000000)!=0x52423745)return;
  uint32_t ped=read(memory,0x80624204);
  if(ped<0x80000000||ped>0x817fd000)return;
  uint32_t vt=read(memory,ped);if(vt<0x80500000||vt>=0x80614000)return;
  // Verified 0x800ffcfc queues a world transition here. Writing the ped's
  // RenderWare matrix alone is undone by the next physics step.
  constexpr uint32_t world=0x8061d584;
  float values[]={x,y,z};
  for(int i=0;i<3;i++){
   uint32_t bits;memcpy(&bits,values+i,4);
   write(memory,bits,world+0x870+i*4);write(memory,bits,world+0x8cc+i*4);
   write(memory,read(memory,ped+4+i*4),world+0x8dc+i*4);
  }
  write(memory,0,world+0x880);
  write(memory,read(memory,0x806eadac),world+0x884);
  write(memory,read(memory,world+0x888)&0x00ffffff,world+0x888);
  write(memory,(read(memory,world+0x86c)&0x00ffffff)|0x01000000,world+0x86c);
  write(memory,(read(memory,world+0x864)&0x00ffffff)|0x01000000,world+0x864);
 }();
 return 0;
}
