// Recovered libmain: the VR draw hook can append glyphs after FinalizeImGui's
// texture update pass. Upload late font requests on the video thread before
// DrawImGui resolves that command's ImTextureRef. Keep the native texture owner.
function createRecoveryPlusImguiTextures(main){
 const site=main.base.add(0x809348),instruction=Instruction.parse(site);
 if(instruction.mnemonic!=='ldr'||instruction.opStr!=='x8, [x25, #0x10]')throw Error('Unsupported ImGui texture draw site');
 const owner=Memory.alloc(Process.pointerSize),stats=Memory.alloc(8);owner.writePointer(ptr(0));stats.writeByteArray(new Uint8Array(8));
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 extern void *ui_owner;
 extern unsigned int texture_stats[];
 extern void update_texture(void*,void*);
 void draw_enter(GumInvocationContext*ic){ui_owner=gum_invocation_context_get_nth_argument(ic,0);}
 void command_enter(GumInvocationContext*ic){
  unsigned char *cmd=(unsigned char*)ic->cpu_context->x[25];
  unsigned char *tex=*(unsigned char**)(cmd+0x10);
  int status;if(!tex||!ui_owner)return;
  status=*(int*)(tex+4);
  if(status==2||status==3){update_texture(ui_owner,tex);texture_stats[0]++;}
  if(!*(void**)(tex+0x10))texture_stats[1]++;
 }
 `,{ui_owner:owner,texture_stats:stats,update_texture:main.base.add(0x80be38)});
 const draw=Interceptor.attach(main.base.add(0x80820c),{onEnter:module.draw_enter});
 const command=Interceptor.attach(site,{onEnter:module.command_enter});
 return {module,owner,stats,report(){return {lateUploads:stats.readU32(),unresolved:stats.add(4).readU32()};},dispose(){command.detach();draw.detach();}};
}
