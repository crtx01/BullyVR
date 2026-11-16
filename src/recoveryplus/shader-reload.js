// Recovery+ libmain SHA256:
// 0c779f7939b6790e9e06678d02e7f518987906f3a8bb290a0ae98b575c180262
// Drain outstanding compiler jobs BEFORE changing the host configuration used
// by their VS/PS/GS generators. Original Reload() waits only after that change.
function createRecoveryPlusShaderReload(main){
 const site=main.base.add(0x906ff4);
 const instruction=Instruction.parse(site);
 if(instruction.mnemonic!=='ldr'||instruction.opStr!=='x0, [x27]')throw Error('Unsupported shader reload site');
 const stats=Memory.alloc(8);stats.writeByteArray(new Uint8Array(8));
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 extern unsigned char main_base[];
 extern unsigned int reload_stats[];
 extern void wait_for_compiler(void *cache);
 void before_host_change(GumInvocationContext *ic){
  void **global=*(void ***)(main_base+0xdd1518);
  void *cache=global?*global:0;
  if(!cache){reload_stats[1]++;return;}
  wait_for_compiler(cache);reload_stats[0]++;
 }
 `,{main_base:main.base,reload_stats:stats,wait_for_compiler:main.base.add(0x82122c)});
 const hook=Interceptor.attach(site,{onEnter:module.before_host_change});
 return {module,stats,report(){return {barriers:stats.readU32(),missingCache:stats.add(4).readU32()};},dispose(){hook.detach();}};
}
