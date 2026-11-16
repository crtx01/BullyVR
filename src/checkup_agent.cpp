// Read-only diagnostics for the exact recovered executable.
#include <jni.h>
#include <dlfcn.h>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
extern "C" __attribute__((visibility("default"))) int Agent_OnAttach(JavaVM*,char*,void*){
 auto h=dlopen("libmain.so",RTLD_NOW);Dl_info i{};
 if(!h||!dladdr(dlsym(h,"Java_org_dolphinemu_dolphinemu_NativeLibrary_RequestOpenXRRecenter"),&i))return -1;
 uintptr_t b=(uintptr_t)i.dli_fbase;
 void* m=((void*(*)(void*))(b+0x6b3a84))((void*)(b+0xdec9c8));
 auto read=(uint32_t(*)(void*,uint32_t))(b+0x5bb354);
 auto byte=[b](uintptr_t at){return *(uint8_t*)(b+at);};
 uint32_t ped=read(m,0x80624204),vt=0;
 if(ped>=0x80000000&&ped<0x81800000)vt=read(m,ped);
 FILE* f=fopen("/data/user/0/org.dolphinemu.dolphinemu.quest.bully.recovery/files/checkup-native.txt","w");if(!f)return -2;
 fprintf(f,"id=%08x player=%08x vtable=%08x running=%u vr=%u vrmenu=%u firstPerson=%u camera=%u\n",read(m,0x80000000),ped,vt,byte(0x173915c),byte(0x170b4ee),byte(0x170b4ef),byte(0xde4ef0),read(m,0x80624834));
 auto paused=(jboolean(*)(JNIEnv*,jclass))dlsym(h,"Java_org_dolphinemu_dolphinemu_NativeLibrary_IsRunningAndUnpaused");
 if(paused)fprintf(f,"coreRunningUnpaused=%d\n",int(paused(nullptr,nullptr)));
 for(uint32_t a:{0x806eab14u,0x806ecbd8u,0x806ec1e8u,0x80001800u,0x8061dde8u,0x8061ddf4u})fprintf(f,"ram[%08x]=%08x\n",a,read(m,a));
 fclose(f);return 0;
}
