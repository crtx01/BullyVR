// Original Bully clock textures, driven by the native ClockGet bytes.
static const Mesh* wristWatch(const Snapshot& s,int layer){
 const auto& hand=s.c[0];if(!hand.connected||!hand.grip.valid)return nullptr;
 static Mesh pieces[4];static bool initialized=false;
 if(!initialized){for(int i=0;i<4;i++){pieces[i].parts.resize(1);pieces[i].parts[0].v.reserve(216);}
  pieces[0].parts[0].texture=pieces[1].parts[0].texture="HUD_clock";
  pieces[2].parts[0].texture="HUD_ArmHour";pieces[3].parts[0].texture="HUD_ArmMinute";initialized=true;}
 auto& mesh=pieces[layer];auto& v=mesh.parts[0].v;v.clear();
 V x=rotate(hand.grip.q,{1,0,0}),up=rotate(hand.grip.q,{0,0,-1}),normal=rotate(hand.grip.q,{0,1,0});
 V wrist=V{hand.grip.p[0],hand.grip.p[1],hand.grip.p[2]}-up*.025f;
 auto point=[&](float a,float b,float depth){return wrist+x*a+up*b+normal*depth;};
 auto vertex=[&](V p,float u,float w){v.push_back({p,u,w,1,1,1,1});};
 auto quad=[&](V a,V b,V c,V d,float y0,float y1){vertex(a,0,y0);vertex(b,1,y0);vertex(c,1,y1);vertex(a,0,y0);vertex(c,1,y1);vertex(d,0,y1);};
 if(layer==0){
  // A dark strap around the wrist, using the opaque center of the face.
  for(int i=0;i<12;i++){
   float a=i*2*locomotion::pi/12,b=(i+1)*2*locomotion::pi/12;
   V a0=wrist+x*(std::cos(a)*.029f)+normal*(std::sin(a)*.024f);
   V b0=wrist+x*(std::cos(b)*.029f)+normal*(std::sin(b)*.024f);
   for(V p:{a0-up*.013f,b0-up*.013f,b0+up*.013f,a0-up*.013f,b0+up*.013f,a0+up*.013f})vertex(p,.5f,.5f);
  }
 }else if(layer==1){
  for(int i=0;i<32;i++){
   float a=i*2*locomotion::pi/32,b=(i+1)*2*locomotion::pi/32;
   vertex(point(0,0,.031f),.5f,.5f);
   vertex(point(std::cos(a)*.039f,std::sin(a)*.039f,.031f),.5f+std::cos(a)*.32f,.5f-std::sin(a)*.32f);
   vertex(point(std::cos(b)*.039f,std::sin(b)*.039f,.031f),.5f+std::cos(b)*.32f,.5f-std::sin(b)*.32f);
  }
 }else{
  unsigned time=nativeClock.load(),hour=time/60,minute=time%60;
  float angle=layer==2?((hour%12)+minute/60.f)*2*locomotion::pi/12:minute*2*locomotion::pi/60;
  float len=layer==2?.021f:.030f,width=.0025f,depth=.031f+(layer-1)*.0007f;
  auto p=[&](float a,float b){return point(a*std::cos(angle)+b*std::sin(angle),-a*std::sin(angle)+b*std::cos(angle),depth);};
  quad(p(-width,len),p(width,len),p(width,-.003f),p(-width,-.003f),layer==2?39.f/128:25.f/128,73.f/128);
 }
 return &mesh;
}
