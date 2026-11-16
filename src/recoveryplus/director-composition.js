// Cutscenes/classes already have their own native composition. Do not apply
// first-person suppression to its material passes or timing-window graphics.
function createRecoveryPlusDirectorComposition(rec) {
 const state=Memory.alloc(8);state.writeByteArray(new Uint8Array(8));
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 extern unsigned char rec_base[];
 extern unsigned int director_stats[];
 extern void *tls(void*);
 void batch_enter(GumInvocationContext *ic){
  unsigned char *skip;
  if(!rec_base[0x12bf98]||!rec_base[0x12bf9c])return;
  skip=tls(rec_base+0x12b908);
  director_stats[1]++;
  if(*skip){*skip=0;director_stats[0]++;}
 }
 `,{rec_base:rec.base,director_stats:state,tls:rec.base.add(0x11b0a0)});
 const hook=Interceptor.attach(rec.base.add(0x64d88),{onEnter:module.batch_enter});
 return {module,state,report(){return {restored:state.readU32(),batches:state.add(4).readU32()};},dispose(){hook.detach();}};
}
