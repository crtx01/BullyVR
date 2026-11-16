// UltimateXR baked hand poses; see bundled ULTIMATEXR_LICENSE.txt and SOURCE.md.
struct HandVertex {float position[4][3],normal[4][3],uv[2];};
static_assert(sizeof(HandVertex)==104);
struct TrackedHand {std::vector<HandVertex> source;std::vector<uint16_t> indices;Mesh mesh;};
static TrackedHand trackedHands[2];
static void loadHands(){
 for(int hand=0;hand<2;hand++){
  auto bytes=readAsset(hand?"BigHandRight.uxrh":"BigHandLeft.uxrh");
  if(bytes.size()<16||memcmp(bytes.data(),"UXRH",4))continue;
  uint32_t header[3];memcpy(header,bytes.data()+4,12);auto [version,n,count]=std::tuple{header[0],header[1],header[2]};
  if(version!=1||n>20000||count>100000||count%3||bytes.size()!=16+n*sizeof(HandVertex)+count*2)continue;
  auto& h=trackedHands[hand];h.source.resize(n);h.indices.resize(count);
  memcpy(h.source.data(),bytes.data()+16,n*sizeof(HandVertex));memcpy(h.indices.data(),bytes.data()+16+n*sizeof(HandVertex),count*2);
  bool valid=true;for(auto index:h.indices)valid&=index<n;
  for(const auto& v:h.source)for(const auto& pose:v.position)for(float value:pose)valid&=std::isfinite(value)&&std::abs(value)<2.f;
  if(!valid){h.source.clear();h.indices.clear();continue;}
  h.mesh.parts.resize(1);h.mesh.parts[0].texture="VRHands";h.mesh.parts[0].v.resize(count);
  LOG("Tracked hand loaded side=%d vertices=%u triangles=%u",hand,n,count/3);
 }
}
static const Mesh* poseHand(int hand,const Snapshot& s){
 auto& h=trackedHands[hand];const auto& c=s.c[hand];
 if(h.source.empty()||!c.connected||!c.grip.valid)return nullptr;
 float grip=std::clamp(c.sv,0.f,1.f);if(hand==1&&weaponCurrent>0)grip=std::max(grip,.90f);float trigger=std::clamp(c.tv,0.f,1.f);
 float weights[4]={(1-grip)*(1-trigger),grip*(1-trigger),(1-grip)*trigger,grip*trigger};
 V forward=norm(rotate(c.aim.valid?c.aim.q:c.grip.q,{0,0,-1}));
 V up=rotate(c.grip.q,{hand==0?-1.f:1.f,0,0});up=norm(up-forward*dot(up,forward));
 if(length(up)<.5f)return nullptr;
 // Both recovered UXRH point clouds are authored in the same canonical
 // chirality. Flipping both Y/Z through the side basis only rolls a right
 // hand; mirror the left depth axis once to retain a dorsal-up left palm.
 // Never choose parity from dot(right, gripForward): it is approximately
 // zero and used to flip unpredictably as the aim/grip poses diverged.
 V right=norm(cross(up,forward));if(hand==0)right=right*-1.f;
 V origin{c.grip.p[0],c.grip.p[1],c.grip.p[2]};auto& vertices=h.mesh.parts[0].v;
 for(size_t i=0;i<h.indices.size();i++){
  const auto& source=h.source[h.indices[i]];V local{};
  for(int pose=0;pose<4;pose++)local=local+V{source.position[pose][0],source.position[pose][1],source.position[pose][2]}*weights[pose];
  vertices[i]={origin+forward*(local.x-.055f)+right*local.z-up*local.y,source.uv[0],source.uv[1],1,1,1,1};
 }
 return &h.mesh;
}
