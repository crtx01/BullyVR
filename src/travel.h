struct TravelDestination{const char* name;V position;float heading=0.f;const char* english=nullptr;};
static constexpr TravelDestination travelDestinations[]={
 {"Escola - portao",{288.889f,-74.866f,5.90f},1.57f,"School - gate"},
 {"Escola - dormitorios",{230.f,-73.f,6.30f},1.57f,"School - dormitories"},
 {"Escola - predio principal",{211.6f,-95.1f,8.60f},1.57f,"School - main building"},
 {"Escola - jardins",{271.37f,-69.866f,6.10f},1.57f,"School - gardens"},
 {"Parque de diversoes",{160.f,448.f,6.30f},0.f,"Carnival - Big Canyon"},
 {"Centro - rua comercial",{500.323f,-43.602f,5.45f},3.14f,"Town - shopping street"},
 {"Old Bullworth Vale",{565.259f,56.345f,13.20f},0.f,"Old Bullworth Vale - bridge"},
 {"Blue Skies - distrito industrial",{343.065f,-436.711f,3.f},3.14f,"Blue Skies - industrial district"},
};
static constexpr int travelCount=BULLY_DEMO?4:sizeof(travelDestinations)/sizeof(*travelDestinations);
static constexpr int quickMenuCount=travelCount+4;
static std::atomic<bool> travelOpen{false};
static std::atomic<bool> travelInProgress{false};
static std::atomic<int> travelHover{0},travelRequest{-1},travelResult{0};
static std::atomic<int> diagnosticTravel{-1};
static std::atomic<double> travelFeedbackUntil{0};
static void processTravel(uint32_t player,bool cinematic){
 static int travelling=-1;static double deadline=0;
 if(travelling>=0){
  uint32_t transform=gameRead(player+0x18),position=gameAddress(transform)?transform+0x30:player+4;
  V here{gameFloat(position),gameFloat(position+4),gameFloat(position+8)};
  V delta=here-travelDestinations[travelling].position;
  if(gameRead(0x806eadac)==0&&std::hypot(delta.x,delta.y)<3.f&&std::isfinite(here.z)&&std::abs(delta.z)<3.f){
   travelResult=1;travelOpen=false;travelInProgress=false;travelling=-1;
   LOG("Exploration transition completed");
  }else if(now()>deadline){
   travelResult=-2;travelFeedbackUntil=now()+6.;travelInProgress=false;travelling=-1;travelOpen=true;
   LOG("Exploration transition timed out");
  }
  return;
 }
 int target=travelRequest.exchange(-1);if(target<0||target>=travelCount)return;
 travelResult=-1;travelFeedbackUntil=now()+4.;
 // The destination scene is exterior 0. The world loader handles leaving an
 // interior as well; rejecting all interior requests made A appear broken.
 if(cinematic||!gameAddress(player))return;
 constexpr uint32_t world=0x8061d584;auto destination=travelDestinations[target].position;
 uint32_t transform=gameRead(player+0x18),position=gameAddress(transform)?transform+0x30:player+4;
 for(int i=0;i<3;i++){
  gameFloatWrite(world+0x870+i*4,(&destination.x)[i]);gameFloatWrite(world+0x8cc+i*4,(&destination.x)[i]);
  gameWrite(world+0x8dc+i*4,gameRead(position+i*4));
 }
 gameFloatWrite(world+0x880,travelDestinations[target].heading);gameWrite(world+0x884,0);
 gameWrite(world+0x888,gameRead(world+0x888)&0x00ffffffu);
 gameWrite(world+0x86c,(gameRead(world+0x86c)&0x00ffffffu)|0x01000000u);
 gameWrite(world+0x864,(gameRead(world+0x864)&0x00ffffffu)|0x01000000u);
 travelling=target;deadline=now()+20.;travelInProgress=true;travelResult=2;
 mem<uint8_t>(0x170b4ef)=0;LOG("Exploration transition queued: %s",travelDestinations[target].name);
}
