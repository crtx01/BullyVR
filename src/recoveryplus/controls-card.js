// Scale only vertices appended by the native controls card, around the viewport
// center. Preserve draw commands, texture refs, indices and other overlay groups.
function createRecoveryPlusControlsCard(rec){
 for(const [offset,before,after] of [
  [0x31744,'X: interagir / montar | Menu: pausa','X: pausa | Y: agarrar (segure)'],
  [0x2bb53,'X: interact / mount | Menu: pause','X: pause | Y: grab (hold)']]){
  const at=rec.base.add(offset);
  if(at.readUtf8String()!==before)continue;
  Memory.protect(at,before.length+1,'rw-');at.writeUtf8String(after);Memory.protect(at,before.length+1,'r--');
 }
 const state=Memory.alloc(32);state.writeByteArray(new Uint8Array(32));
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 typedef struct {unsigned char *list;unsigned int first;float cx,cy;unsigned int active,frames;} State;
 extern State card_state;
 extern unsigned char rec_base[];
 void enter(GumInvocationContext*ic){
  unsigned char *ctx;card_state.active=0;
  if(!rec_base[0x12bc3c])return;
  card_state.list=gum_invocation_context_get_nth_argument(ic,0);
  ctx=gum_invocation_context_get_nth_argument(ic,1);
  if(!ctx||!card_state.list)return;
  card_state.first=*(unsigned int*)(card_state.list+0x20);
  card_state.cx=*(float*)(ctx+8)*.5f;card_state.cy=*(float*)(ctx+12)*.5f;
  card_state.active=1;
 }
 void leave(GumInvocationContext*ic){
  unsigned int end,i;unsigned char *data;float *p;
  if(!card_state.active)return;card_state.active=0;
  end=*(unsigned int*)(card_state.list+0x20);data=*(unsigned char**)(card_state.list+0x28);
  if(!data||end<card_state.first||end-card_state.first>200000)return;
  for(i=card_state.first;i<end;i++){
   p=(float*)(data+i*20);p[0]=card_state.cx+(p[0]-card_state.cx)*.68f;
   p[1]=card_state.cy+(p[1]-card_state.cy)*.68f;
  }
  card_state.frames++;
 }
 `,{card_state:state,rec_base:rec.base});
 const hook=Interceptor.attach(rec.base.add(0x69a20),{onEnter:module.enter,onLeave:module.leave});
 return {module,state,report(){return {scale:.68,frames:state.add(24).readU32()};},dispose(){hook.detach();}};
}
