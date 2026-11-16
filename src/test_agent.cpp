#include <jni.h>
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
extern "C" __attribute__((visibility("default"))) int Agent_OnAttach(JavaVM*,char* options,void*){
 int field;float value;
 if(!options||sscanf(options,"touch:%d:%f",&field,&value)!=2)return -1;
 void* library=dlopen("libbully_recovery.so",RTLD_NOW);
 auto control=(void(*)(int,float))dlsym(library,"BullyRecovery_TestControl");
 if(!control)return -2;control(field,value);return 0;
}
