// Recovery+ r22: the untextured sky gradient uses orthographic triangles too.
// It must not receive the map's user-adjustable HUD translation.
function createRecoveryPlusSkyRoutingPrototype(main,rec){
 if(rec.getExportByName('BullyRecovery_TestControl').sub(rec.base).toInt32()!==0x5acb0)throw Error('Unsupported sky routing binary');
 const format=new NativeFunction(main.base.add(0x87b11c),'pointer',[]);
 let gradient=false,seen=0,bypassed=0,lastError='';
 const draw=Interceptor.attach(rec.base.add(0x6316c),{
  onEnter(args){
   gradient=false;
   try{
    if(!rec.base.add(0x12bf98).readU8()||rec.base.add(0x12bf9c).readU8()||main.base.add(0x170b4ef).readU8())return;
    const xf=main.base.add(0xdd02c0).readPointer();if(xf.isNull()||!xf.add(0x4098).readS32())return;
    const bp=main.base.add(0xdd0e28).readPointer();if(bp.isNull())return;
    const stages=(bp.readU32()>>>10)&15;
    for(let stage=0;stage<=stages;stage++)if((bp.add(0xa0+(stage>>>1)*4).readU32()>>>((stage&1)*12))&64)return;
    const f=format();if(f.isNull()||f.add(8).readU32()!==16)return;
    const end=args[0].add(8).readPointer(),begin=args[0].add(0x10).readPointer();
    if(begin.isNull()||end.sub(begin).toInt32()!==96)return;
    let lx=Infinity,hx=-Infinity,ly=Infinity,hy=-Infinity,light=false;
    for(let i=0;i<6;i++){
     const at=begin.add(i*16),x=at.readFloat(),y=at.add(4).readFloat(),color=at.add(12).readU32();
     if(!Number.isFinite(x)||!Number.isFinite(y))return;
     lx=Math.min(lx,x);hx=Math.max(hx,x);ly=Math.min(ly,y);hy=Math.max(hy,y);
     light=light||((color&255)>48)||(((color>>>8)&255)>48)||(((color>>>16)&255)>48);
    }
    gradient=light&&hx-lx>2.8&&hx-lx<3.3&&hy-ly>.001;
    if(gradient)seen++;
   }catch(e){lastError=String(e);}
  },onLeave(){gradient=false;}
 });
 const geometry=Interceptor.attach(rec.base.add(0x62f3c),{onEnter(){
  if(gradient){
   const constants=this.context.x19;
   constants.add(8).writeFloat(0);constants.add(12).writeFloat(0);constants.add(0x130).writeU8(1);
   this.context.x8=ptr(0);bypassed++;
  }
 }});
 return {report(){return {seen,bypassed,lastError};},dispose(){geometry.detach();draw.detach();}};
}

// Production callbacks run natively: no per-draw JS crossing or allocations.
// CModule callback interface: https://frida.re/docs/javascript-api/#cmodule
function createRecoveryPlusSkyRouting(main,rec){
 if(rec.getExportByName('BullyRecovery_TestControl').sub(rec.base).toInt32()!==0x5acb0)throw Error('Unsupported sky routing binary');
 const state=Memory.alloc(16);state.writeByteArray(new Uint8Array(16));
 const module=new CModule(`
 #include <gum/guminterceptor.h>
 extern unsigned char main_base[];
 extern unsigned char rec_base[];
 extern unsigned int routing_state[];
 extern void *native_format(void);
 static int is_gradient(void *batch){
  unsigned char *xf,*bp,*f,*begin,*end,*at;
  unsigned int stages,stage,color;int i,light=0;
  float x,y,lx=1e6f,hx=-1e6f,ly=1e6f,hy=-1e6f;
  if(!rec_base[0x12bf98]||rec_base[0x12bf9c]||main_base[0x170b4ef])return 0;
  xf=*(unsigned char **)(main_base+0xdd02c0);if(!xf||!*(int *)(xf+0x4098))return 0;
  bp=*(unsigned char **)(main_base+0xdd0e28);if(!bp)return 0;
  stages=(*(unsigned int *)bp>>10)&15;
  for(stage=0;stage<=stages;stage++)if(((*(unsigned int *)(bp+0xa0+(stage>>1)*4)>>((stage&1)*12))&64))return 0;
  f=native_format();if(!f||*(unsigned int *)(f+8)!=16)return 0;
  end=*(unsigned char **)((unsigned char *)batch+8);begin=*(unsigned char **)((unsigned char *)batch+0x10);
  if(!begin||end-begin!=96)return 0;
  for(i=0;i<6;i++){
   at=begin+i*16;x=*(float *)at;y=*(float *)(at+4);color=*(unsigned int *)(at+12);
   if(!(x>-1e6f&&x<1e6f&&y>-1e6f&&y<1e6f))return 0;
   if(x<lx)lx=x;if(x>hx)hx=x;if(y<ly)ly=y;if(y>hy)hy=y;
   if((color&255)>48||((color>>8)&255)>48||((color>>16)&255)>48)light=1;
  }
  return light&&hx-lx>2.8f&&hx-lx<3.3f&&hy-ly>.001f;
 }
 void draw_enter(GumInvocationContext *ic){
  routing_state[0]=is_gradient(gum_invocation_context_get_nth_argument(ic,0));
  routing_state[3]++;if(routing_state[0])routing_state[1]++;
 }
 void draw_leave(GumInvocationContext *ic){routing_state[0]=0;}
 void geometry_enter(GumInvocationContext *ic){
  unsigned char *constants;
  if(!routing_state[0])return;
  constants=(unsigned char *)ic->cpu_context->x[19];
  *(float *)(constants+8)=0;*(float *)(constants+12)=0;constants[0x130]=1;
  ic->cpu_context->x[8]=0;routing_state[2]++;
 }
 `,{main_base:main.base,rec_base:rec.base,routing_state:state,native_format:main.base.add(0x87b11c)});
 const draw=Interceptor.attach(rec.base.add(0x6316c),{onEnter:module.draw_enter,onLeave:module.draw_leave});
 const geometry=Interceptor.attach(rec.base.add(0x62f3c),{onEnter:module.geometry_enter});
 return {module,state,report(){return {native:true,seen:state.add(4).readU32(),bypassed:state.add(8).readU32(),draws:state.add(12).readU32()};},dispose(){geometry.detach();draw.detach();/* Keep code mapped until intercepted calls have drained. */}};
}
