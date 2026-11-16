// Local ADB test harness. Disabled by default; auto-expires and never survives restart.
static pthread_mutex_t testMutex=PTHREAD_MUTEX_INITIALIZER;
static Snapshot testSnapshot{};
static double testDeadline=0;
static float testHeadYaw=0,testHeadPitch=0,testAimYaw[2]{},testAimPitch[2]{};
static Q eulerPose(float y,float p){float a=y*.5f,b=p*.5f;return {std::cos(a)*std::sin(b),std::sin(a)*std::cos(b),-std::sin(a)*std::sin(b),std::cos(a)*std::cos(b)};}
extern "C" __attribute__((visibility("default"))) void BullyRecovery_TestControl(int field,float value){
 pthread_mutex_lock(&testMutex);
 if(field==0){
  testDeadline=value>0?now()+60.:0;
  testSnapshot={};testSnapshot.active=true;testSnapshot.head={true,{0,1.60f,0},{0,0,0,1}};
  testHeadYaw=testHeadPitch=0;
  for(int h=0;h<2;h++){auto& c=testSnapshot.c[h];c.connected=true;c.grip={true,{h?.24f:-.24f,1.28f,-.42f},{0,0,0,1}};c.aim=c.grip;testAimYaw[h]=testAimPitch[h]=0;}
  LOG("ADB Touch simulation enabled=%d",value>0);
 }else if(testDeadline>now()){
  testDeadline=now()+60.;auto& l=testSnapshot.c[0];auto& r=testSnapshot.c[1];
  switch(field){
   case 1:l.x=value;break;case 2:l.y=value;break;case 3:r.x=value;break;case 4:r.y=value;break;
   case 5:r.primary=value>.5f;break;case 6:r.secondary=value>.5f;break;case 7:l.primary=value>.5f;break;case 8:l.secondary=value>.5f;break;
   case 9:l.trigger=value>.5f;l.tv=value;break;case 10:r.trigger=value>.5f;r.tv=value;break;
   case 11:l.squeeze=value>.5f;l.sv=value;break;case 12:r.squeeze=value>.5f;r.sv=value;break;
   case 13:l.stick=value>.5f;break;case 14:r.stick=value>.5f;break;
   case 15:testHeadYaw=value*.0174532925f;break;case 16:testHeadPitch=value*.0174532925f;break;
   case 17:case 18:case 19:l.grip.p[field-17]=value;break;
   case 20:case 21:case 22:r.grip.p[field-20]=value;break;
   case 23:testAimPitch[1]=value*.0174532925f;break;case 24:testAimYaw[1]=value*.0174532925f;break;
   case 25:testAimPitch[0]=value*.0174532925f;break;case 26:testAimYaw[0]=value*.0174532925f;break;
  }
  testSnapshot.head.q=eulerPose(testHeadYaw,testHeadPitch);
  for(int h=0;h<2;h++){auto& c=testSnapshot.c[h];c.aim=c.grip;c.aim.q=eulerPose(testAimYaw[h],testAimPitch[h]);}
 }
 pthread_mutex_unlock(&testMutex);
}
static bool testInput(Snapshot& output){
 pthread_mutex_lock(&testMutex);bool enabled=now()<testDeadline;
 if(enabled){output=testSnapshot;output.generation++;}
 pthread_mutex_unlock(&testMutex);return enabled;
}
static void publishTestInput(){
 Snapshot simulated;if(!testInput(simulated))return;
 auto* mutex=&mem<pthread_mutex_t>(0xdec9e0);pthread_mutex_lock(mutex);
 auto generation=mem<Snapshot>(0xdd7f28).generation+1;mem<Snapshot>(0xdd7f28)=simulated;mem<Snapshot>(0xdd7f28).generation=generation;pthread_mutex_unlock(mutex);
}
