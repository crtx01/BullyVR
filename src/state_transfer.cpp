#include <jni.h>
#include <dlfcn.h>
#include <android/log.h>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
extern "C" __attribute__((visibility("default"))) int Agent_OnAttach(JavaVM* vm,char* options,void*) {
 if(options&&(strncmp(options,"ram:",4)==0||strncmp(options,"button:",7)==0||strcmp(options,"combo")==0)){
  void* main=dlopen("libmain.so",RTLD_NOW);Dl_info info{};dladdr(dlsym(main,"Java_org_dolphinemu_dolphinemu_NativeLibrary_RequestOpenXRRecenter"),&info);auto base=(uintptr_t)info.dli_fbase;
  if(strcmp(options,"combo")==0){*(uint8_t*)(base+0xde4688)=1;return 0;}
  if(strncmp(options,"button:",7)==0){
   int control=0;double value=0;sscanf(options+7,"%d:%lf",&control,&value);
   auto reg=(void(*)(JNIEnv*,jclass,int))dlsym(main,"Java_org_dolphinemu_dolphinemu_features_input_model_InputOverrider_registerWii");
   auto set=(void(*)(JNIEnv*,jclass,int,int,double))dlsym(main,"Java_org_dolphinemu_dolphinemu_features_input_model_InputOverrider_setControlState");
   reg(nullptr,nullptr,0);set(nullptr,nullptr,0,control,value);return 0;
  }
  unsigned address=0,length=0;sscanf(options+4,"%x:%u",&address,&length);if(length>4096||length%4)return -6;
  auto memory=((void*(*)(void*))(base+0x6b3a84))((void*)(base+0xdec9c8));
  char path[240];snprintf(path,sizeof(path),BULLY_FILES "/ram-%08x.bin",address);
  FILE* file=fopen(path,"wb");if(!file)return -7;
  for(unsigned offset=0;offset<length;offset+=4){uint32_t value=((uint32_t(*)(void*,uint32_t))(base+0x5bb354))(memory,address+offset);value=__builtin_bswap32(value);fwrite(&value,4,1,file);}fclose(file);return 0;
 }
 if(options&&strncmp(options,"effect:",7)==0){
  auto h=dlopen("libbully_recovery.so",RTLD_NOW);auto set=(void(*)(int))dlsym(h,"BullyRecovery_Diagnostic");
  if(!set)return -5;set(atoi(options+7));return 0;
 }
 JNIEnv* env=nullptr;bool attached=false;
 if(vm->GetEnv((void**)&env,JNI_VERSION_1_6)!=JNI_OK){if(vm->AttachCurrentThread(&env,nullptr)!=JNI_OK)return -1;attached=true;}
 const bool load=options&&strncmp(options,"load:",5)==0;
 const char* path=options?options+(load?5:5):nullptr;if(!path)return -2;
 void* h=dlopen("libmain.so",RTLD_NOW);if(!h)return -3;
 using Method=void(*)(JNIEnv*,jclass,jstring);
 auto method=(Method)dlsym(h,load?"Java_org_dolphinemu_dolphinemu_NativeLibrary_LoadStateAs":"Java_org_dolphinemu_dolphinemu_NativeLibrary_SaveStateAs");
 if(!method)return -4;auto p=env->NewStringUTF(path);method(env,nullptr,p);env->DeleteLocalRef(p);
 __android_log_print(ANDROID_LOG_INFO,"BullyRecovery","State %s requested: %s",load?"load":"save",path);
 if(attached)vm->DetachCurrentThread();return 0;
}
