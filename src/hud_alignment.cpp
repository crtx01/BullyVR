#include <jni.h>
#include <dlfcn.h>
#include <cstdint>
extern "C" __attribute__((visibility("default"))) int Agent_OnAttach(JavaVM*,char*,void*){
 auto h=dlopen("libmain.so",RTLD_NOW);Dl_info i{};if(!h||!dladdr(dlsym(h,"Java_org_dolphinemu_dolphinemu_NativeLibrary_RequestOpenXRRecenter"),&i))return -1;
 *(float*)((uintptr_t)i.dli_fbase+0x1741600)=.18f;return 0;
}
