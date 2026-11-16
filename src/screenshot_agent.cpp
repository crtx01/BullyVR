#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
extern "C" __attribute__((visibility("default"))) int Agent_OnAttach(JavaVM*,char*,void*){
 void* main=dlopen("libmain.so",RTLD_NOW);auto save=(void(*)(JNIEnv*,jclass))dlsym(main,"Java_org_dolphinemu_dolphinemu_NativeLibrary_SaveScreenShot");
 if(!save)return -1;save(nullptr,nullptr);__android_log_print(ANDROID_LOG_INFO,"BullyRecovery","Game framebuffer screenshot requested");return 0;
}
