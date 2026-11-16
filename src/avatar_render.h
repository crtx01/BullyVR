// The original ImGui body renderer cannot interpolate UVs in perspective or
// resolve intersecting limbs. Render to a transparent, depth-backed layer first.
struct AvatarVertex {float x,y,z,w,u,v,light;};
struct AvatarPass {
 GLuint program=0,vao=0,vbo=0,fbo=0,depth=0,sampler=0;
 Texture* color=nullptr;uint32_t width=0,height=0;bool failed=false;
 std::vector<AvatarVertex> vertices;
};
static AvatarPass avatarPass;
static GLuint avatarShader(GLenum type,const char* source){
 GLuint shader=glCreateShader(type);glShaderSource(shader,1,&source,nullptr);glCompileShader(shader);
 GLint okay=0;glGetShaderiv(shader,GL_COMPILE_STATUS,&okay);
 if(!okay){char log[512];glGetShaderInfoLog(shader,sizeof(log),nullptr,log);LOG("Avatar shader: %s",log);glDeleteShader(shader);return 0;}return shader;
}
static bool initAvatarPass(){
 auto& a=avatarPass;if(a.program)return true;if(a.failed)return false;
 const char* vertex="#version 300 es\nprecision highp float;layout(location=0) in vec4 position;layout(location=1) in vec2 uv;layout(location=2) in float shade;out vec2 texcoord;out float light;void main(){gl_Position=position;texcoord=uv;light=shade;}";
 const char* fragment="#version 300 es\nprecision highp float;precision highp sampler2DArray;in vec2 texcoord;in float light;uniform sampler2DArray image;out vec4 color;void main(){vec4 c=texture(image,vec3(texcoord,0.0));if(c.a<0.15)discard;color=vec4(c.rgb*light,c.a);}";
 GLuint vs=avatarShader(GL_VERTEX_SHADER,vertex),fs=avatarShader(GL_FRAGMENT_SHADER,fragment);
 if(!vs||!fs){a.failed=true;return false;}
 a.program=glCreateProgram();glAttachShader(a.program,vs);glAttachShader(a.program,fs);glLinkProgram(a.program);glDeleteShader(vs);glDeleteShader(fs);
 GLint okay;glGetProgramiv(a.program,GL_LINK_STATUS,&okay);if(!okay){a.failed=true;return false;}
 glGenVertexArrays(1,&a.vao);glGenBuffers(1,&a.vbo);glGenFramebuffers(1,&a.fbo);glGenRenderbuffers(1,&a.depth);glGenSamplers(1,&a.sampler);
 glSamplerParameteri(a.sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR);glSamplerParameteri(a.sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
 glSamplerParameteri(a.sampler,GL_TEXTURE_WRAP_S,GL_REPEAT);glSamplerParameteri(a.sampler,GL_TEXTURE_WRAP_T,GL_REPEAT);
 a.vertices.reserve(14000);LOG("Avatar GPU renderer initialized");return true;
}
static Texture* renderAvatar(void* context,const Snapshot& s){
 auto& a=avatarPass;if(a.failed)return nullptr;
 auto* raw=(uint8_t*)context;auto* eye=*(float**)context;
 float screenW=*(float*)(raw+8),screenH=*(float*)(raw+12);
 if(screenW<1||screenH<1||!eye)return nullptr;
 // Load native texture objects before saving GL state; Dolphin handles its own
 // upload bindings here. No asset reads are performed after their first use.
 texture("VRHands");for(const char* name:{"HUD_clock","HUD_ArmHour","HUD_ArmMinute"})texture(name);
 for(auto& [name,mesh]:meshes)if(name==weaponModels[weaponCurrent.load()])for(auto& part:mesh.parts)texture(part.texture);
 GLint oldProgram,oldVao,oldBuffer,oldDrawFbo,oldReadFbo,oldRenderbuffer,oldActive,oldTexture,oldSampler,viewport[4],depthFunc;
 GLboolean depthTest=glIsEnabled(GL_DEPTH_TEST),scissor=glIsEnabled(GL_SCISSOR_TEST),blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE),depthMask,colorMask[4];
 GLfloat clearColor[4],clearDepth,depthRange[2];
 glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&oldVao);glGetIntegerv(GL_ARRAY_BUFFER_BINDING,&oldBuffer);
 glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&oldDrawFbo);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&oldReadFbo);glGetIntegerv(GL_RENDERBUFFER_BINDING,&oldRenderbuffer);
 glGetIntegerv(GL_ACTIVE_TEXTURE,&oldActive);glActiveTexture(GL_TEXTURE0);glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY,&oldTexture);glGetIntegerv(GL_SAMPLER_BINDING,&oldSampler);
 glGetIntegerv(GL_VIEWPORT,viewport);glGetIntegerv(GL_DEPTH_FUNC,&depthFunc);glGetBooleanv(GL_DEPTH_WRITEMASK,&depthMask);glGetBooleanv(GL_COLOR_WRITEMASK,colorMask);
 glGetFloatv(GL_COLOR_CLEAR_VALUE,clearColor);glGetFloatv(GL_DEPTH_CLEAR_VALUE,&clearDepth);glGetFloatv(GL_DEPTH_RANGE,depthRange);
 auto restore=[&]{
  glBindFramebuffer(GL_DRAW_FRAMEBUFFER,oldDrawFbo);glBindFramebuffer(GL_READ_FRAMEBUFFER,oldReadFbo);glBindRenderbuffer(GL_RENDERBUFFER,oldRenderbuffer);
  glUseProgram(oldProgram);glBindVertexArray(oldVao);glBindBuffer(GL_ARRAY_BUFFER,oldBuffer);
  glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D_ARRAY,oldTexture);glBindSampler(0,oldSampler);glActiveTexture(oldActive);
  glViewport(viewport[0],viewport[1],viewport[2],viewport[3]);glDepthFunc(depthFunc);glDepthMask(depthMask);glDepthRangef(depthRange[0],depthRange[1]);glColorMask(colorMask[0],colorMask[1],colorMask[2],colorMask[3]);
  glClearColor(clearColor[0],clearColor[1],clearColor[2],clearColor[3]);glClearDepthf(clearDepth);
  for(auto state:std::array<std::pair<GLenum,GLboolean>,4>{{{GL_DEPTH_TEST,depthTest},{GL_SCISSOR_TEST,scissor},{GL_BLEND,blend},{GL_CULL_FACE,cull}}}){if(state.second)glEnable(state.first);else glDisable(state.first);}
 };
 if(!initAvatarPass()){restore();return nullptr;}
 float scale=std::min(1.f,1024.f/std::max(screenW,screenH));uint32_t width=uint32_t(screenW*scale),height=uint32_t(screenH*scale);
 if(!a.color||a.width!=width||a.height!=height){
  if(a.color)delete a.color;
  TextureConfig config;config.width=width;config.height=height;config.flags=1;
  auto* gfx=*mem<void**>(0xdd14d8);auto** vt=*(void***)gfx;
  using Create=std::unique_ptr<Texture>(*)(void*,const TextureConfig&,std::string_view);
  a.color=((Create)vt[16])(gfx,config,"Jimmy VR depth layer").release();a.width=width;a.height=height;
  if(!a.color||!glIsTexture(*(GLuint*)((uint8_t*)a.color+0x28))){a.failed=true;LOG("Avatar texture ABI rejected");restore();return nullptr;}
  glBindFramebuffer(GL_FRAMEBUFFER,a.fbo);glFramebufferTextureLayer(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,*(GLuint*)((uint8_t*)a.color+0x28),0,0);
  glBindRenderbuffer(GL_RENDERBUFFER,a.depth);glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT16,width,height);glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,a.depth);
  if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE){a.failed=true;LOG("Avatar framebuffer incomplete");restore();return nullptr;}
  LOG("Avatar depth layer %ux%u",width,height);
 }
 glBindFramebuffer(GL_FRAMEBUFFER,a.fbo);glViewport(0,0,a.width,a.height);glDisable(GL_SCISSOR_TEST);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
 glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LESS);glDepthMask(GL_TRUE);glDepthRangef(0,1);glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
 glClearColor(0,0,0,0);glClearDepthf(1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
 glActiveTexture(GL_TEXTURE0);glUseProgram(a.program);glUniform1i(glGetUniformLocation(a.program,"image"),0);glBindVertexArray(a.vao);glBindBuffer(GL_ARRAY_BUFFER,a.vbo);glBindSampler(0,a.sampler);
 glEnableVertexAttribArray(0);glVertexAttribPointer(0,4,GL_FLOAT,GL_FALSE,sizeof(AvatarVertex),(void*)0);
 glEnableVertexAttribArray(1);glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,sizeof(AvatarVertex),(void*)(4*sizeof(float)));
 glEnableVertexAttribArray(2);glVertexAttribPointer(2,1,GL_FLOAT,GL_FALSE,sizeof(AvatarVertex),(void*)(6*sizeof(float)));
 Q inverse{-eye[0],-eye[1],-eye[2],eye[3]};V eyePosition{eye[4],eye[5],eye[6]};
 float left=std::tan(eye[7]),right=std::tan(eye[8]),up=std::tan(eye[9]),down=std::tan(eye[10]);
 int weapon=weaponCurrent.load();
 auto emit=[&](const Mesh& mesh,bool body,float shade=1.f){
  for(auto& part:mesh.parts){

   auto found=textures.find(part.texture);if(found==textures.end())continue;
   a.vertices.clear();
   for(size_t i=0;i+2<part.v.size();i+=3){

    V positions[3];
    for(int j=0;j<3;j++){
     V point=part.v[i+j].p;
     if(body)point=part.v[i+j].p;
     else point=weaponPoint(point,mesh,weapon,s);
     positions[j]=rotate(inverse,point-eyePosition);
    }
    V normal=norm(cross(positions[1]-positions[0],positions[2]-positions[0]));float lighting=shade*(.72f+.28f*std::abs(dot(normal,norm(V{-.3f,.6f,.7f}))));
    for(int j=0;j<3;j++){auto& p=positions[j];auto& v=part.v[i+j];float depth=-p.z;
     a.vertices.push_back({2*p.x/(right-left)+p.z*(right+left)/(right-left),2*p.y/(up-down)+p.z*(up+down)/(up-down),depth-.02f,depth,v.u,v.v,lighting});
    }
   }
   glBindTexture(GL_TEXTURE_2D_ARRAY,*(GLuint*)((uint8_t*)found->second+0x28));glBufferData(GL_ARRAY_BUFFER,a.vertices.size()*sizeof(AvatarVertex),a.vertices.data(),GL_STREAM_DRAW);
   glDrawArrays(GL_TRIANGLES,0,a.vertices.size());
  }
 };
 // Preserve the original game's skinned, animated Jimmy. The old CPU deformation
 // applied to bind-space vertices caused folded arms and long textured triangles.
 for(int side=0;side<2;side++)if(const Mesh* hand=poseHand(side,s))emit(*hand,true);
 for(int layer=0;layer<4;layer++)if(const Mesh* watch=wristWatch(s,layer))emit(*watch,true,layer==0?.09f:1.f);
 if(weapon>0&&weaponIds[weapon]!=437&&s.c[1].grip.valid)if(auto object=meshes.find(weaponModels[weapon]);object!=meshes.end())emit(object->second,false);
 restore();return a.color;
}
