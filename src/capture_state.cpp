#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
extern "C" __attribute__((visibility("default"))) int Agent_OnAttach(JavaVM* vm,char*,void*) {
 JNIEnv* env=nullptr;if(vm->GetEnv((void**)&env,JNI_VERSION_1_6)!=JNI_OK)return -1;
 void* h=dlopen("libmain.so",RTLD_NOW);if(!h)return -2;
 using Save=void(*)(JNIEnv*,jclass,jstring);
 auto save=(Save)dlsym(h,"Java_org_dolphinemu_dolphinemu_NativeLibrary_SaveStateAs");if(!save)return -3;
 auto path=env->NewStringUTF("/sdcard/Android/data/org.dolphinemu.dolphinemu.quest.bully/files/StateSaves/recovery-current.sav");
 save(env,nullptr,path);env->DeleteLocalRef(path);
 __android_log_print(ANDROID_LOG_INFO,"BullyRecovery","Current scene capture requested");return 0;
}
