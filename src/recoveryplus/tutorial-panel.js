// Center the native tutorial's measured text rectangle before its background
// and glyphs are drawn. This changes no map or mission scripting coordinates.
function createRecoveryPlusTutorialPanel(main,rec,read,mem,payload){
 const write=new NativeFunction(main.base.add(0x5bb688),'void',['pointer','uint32','uint32']);
 const state=new NativeFunction(main.base.add(0x6b3af4),'pointer',['pointer'])(main.base.add(0xdec9c8));
 const jit=new NativeFunction(main.base.add(0x6b3a44),'pointer',['pointer'])(main.base.add(0xdec9c8));
 const invalidate=new NativeFunction(main.base.add(0x6a6e78),'void',['pointer','pointer','pointer','uint32']);
 let installed=0,applies=0,error='';
 const branch=(from,to)=>0x48000000|((to-from)&0x03fffffc);
 function install(){
  const current=read(mem,payload.site);
  if(installed&&current===(branch(payload.site,installed+payload.entry)>>>0))return;
  if(current!==payload.original){error='Unexpected tutorial renderer; patch withheld';return;}
  let start=0;
  for(let a=0x80001810;a<0x80002ff0;a+=4)if(read(mem,a)===0x00d0c0de&&read(mem,a+4)===0x00d0c0de){start=a+8;break;}
  if(!start)return;
  let body=0;
  for(let a=start;a<0x80002ff0;a+=8){
   const code=read(mem,a);
   if(code===0xc0000000){const size=read(mem,a+4)*8;if(size>4000||a+8+size>=0x80002ff0)return;
    if(size===payload.words.length*4&&read(mem,a+8+payload.returnOffset+4)===0x48554443){body=a+8;break;}
    a+=size;continue;
   }
   if(code===0xf0000000){const size=payload.words.length*4;if(a+size+16>=0x80002ff0)return;
    body=a+8;for(let i=0;i<payload.words.length;i++)write(mem,payload.words[i],body+i*4);
    write(mem,0xf0000000,body+size);write(mem,0,body+size+4);write(mem,size/8,a+4);write(mem,0xc0000000,a);break;
   }
  }
  if(!body)return;
  const back=body+payload.returnOffset;
  write(mem,branch(back,payload.site+4),back);
  write(mem,branch(payload.site,body+payload.entry),payload.site);
  for(const at of [payload.site,body,back])invalidate(state.add(0x2550),mem,jit,at&0xffffffe0);
  write(mem,0xd01f1bad,0x80001800);installed=body;applies++;error='';
 }
 const hook=Interceptor.attach(main.base.add(0x53e4dc),{onEnter(){if(!rec.base.add(0x12bf98).readU8())return;try{install();}catch(e){error=String(e);}}});
 return {report(){return {installed,applies,error};},dispose(){hook.detach();}};
}
