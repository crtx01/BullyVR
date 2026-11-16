// One-shot framebuffer evidence, independent of Quest passthrough/compositor layers.
static void captureFramebuffer(bool leftEye){
 static FILE* movie=nullptr;static double movieAt=0,nextFrame=0;
 if(leftEye){
  int request=recordGameFrames.exchange(0);
  if(request&&movie){fclose(movie);movie=nullptr;}
  if(request==1){
   movie=fopen(BULLY_FILES "/recovery-gameplay.rgba","wb");movieAt=now();nextFrame=movieAt;
   if(movie){uint32_t header[4]={0x42565231,640,672,10};fwrite(header,4,4,movie);}
  }
  if(movie&&now()-movieAt>=45.){fclose(movie);movie=nullptr;}
 }
 bool still=captureGameFrame.exchange(false),video=leftEye&&movie&&now()>=nextFrame;
 if(!still&&!video)return;
 if(video)nextFrame=now()+.1;
 GLint draw=0,read=0,viewport[4]{},texture=0,pack=0,alignment=0;
 glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&draw);glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING,&read);glGetIntegerv(GL_VIEWPORT,viewport);
 glGetIntegerv(GL_TEXTURE_BINDING_2D,&texture);glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING,&pack);glGetIntegerv(GL_PACK_ALIGNMENT,&alignment);
 int width=viewport[2],height=viewport[3];if(width<1||height<1||width>4096||height>4096)return;
 // The diagnostic movie records the real single-eye render, before the
 // compositor's late reprojection. No stereo stitching or generated frames.
 int sourceWidth=width,sourceHeight=height;
 if(video&&!still){width=640;height=672;}
 GLboolean scissor=glIsEnabled(GL_SCISSOR_TEST);glDisable(GL_SCISSOR_TEST);
 GLuint resolve=0,color=0;glGenTextures(1,&color);glBindTexture(GL_TEXTURE_2D,color);glTexStorage2D(GL_TEXTURE_2D,1,GL_RGBA8,width,height);
 glGenFramebuffers(1,&resolve);glBindFramebuffer(GL_DRAW_FRAMEBUFFER,resolve);glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,color,0);
 glBindFramebuffer(GL_READ_FRAMEBUFFER,draw);glBlitFramebuffer(viewport[0],viewport[1],viewport[0]+sourceWidth,viewport[1]+sourceHeight,0,0,width,height,GL_COLOR_BUFFER_BIT,GL_LINEAR);
 glBindFramebuffer(GL_READ_FRAMEBUFFER,resolve);glBindBuffer(GL_PIXEL_PACK_BUFFER,0);glPixelStorei(GL_PACK_ALIGNMENT,1);
 std::vector<uint8_t> pixels(size_t(width)*height*4);glReadPixels(0,0,width,height,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());
 if(still){FILE* file=fopen(BULLY_FILES "/recovery-frame.rgba","wb");
  if(file){uint32_t header[2]={uint32_t(width),uint32_t(height)};fwrite(header,4,2,file);fwrite(pixels.data(),1,pixels.size(),file);fclose(file);LOG("Game framebuffer evidence %dx%d",width,height);}}
 if(video&&!still)fwrite(pixels.data(),1,pixels.size(),movie);
 glBindFramebuffer(GL_DRAW_FRAMEBUFFER,draw);glBindFramebuffer(GL_READ_FRAMEBUFFER,read);glBindTexture(GL_TEXTURE_2D,texture);glBindBuffer(GL_PIXEL_PACK_BUFFER,pack);glPixelStorei(GL_PACK_ALIGNMENT,alignment);glDeleteFramebuffers(1,&resolve);glDeleteTextures(1,&color);if(scissor)glEnable(GL_SCISSOR_TEST);
}
