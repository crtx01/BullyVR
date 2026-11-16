function ensureRecoveryInfrastructure(){
 const rec=Process.findModuleByName('libbully_recovery.so');if(!rec)return;
 const root='/data/user/0/org.dolphinemu.dolphinemu.quest.bully.recover2/files/';
 function text(path,fallback=''){try{return File.readAllText(path);}catch(e){return fallback;}}
 function atomic(path,value){const temp=path+'.recovery.tmp';File.writeAllText(temp,value);const rename=new NativeFunction(Module.getGlobalExportByName('rename'),'int',['pointer','pointer']);if(rename(Memory.allocUtf8String(temp),Memory.allocUtf8String(path))!==0)throw Error('Cannot update '+path);}
 const configPath=root+'Config/Dolphin.ini';let config=text(configPath,'[Core]\nEnableCheats = True\n');
 if(/^EnableCheats\s*=/m.test(config))config=config.replace(/^EnableCheats\s*=.*$/m,'EnableCheats = True');
 else if(/^\[Core\]/m.test(config))config=config.replace(/^\[Core\]/m,'[Core]\nEnableCheats = True');
 else config+='\n[Core]\nEnableCheats = True\n';
 if(config!==text(configPath))atomic(configPath,config);
 const path=root+'GameSettings/RB7E54.ini';let ini=text(path),before=ini;
 const label='$Recovery infrastructure';
 if(!ini.split(/\r?\n/).some(line=>line.trim()===label)){
  const words=['C0000000 00000125'];for(let i=0;i<293;i++)words.push(rec.base.add(0x33290+i*8).readU32().toString(16).padStart(8,'0').toUpperCase()+' '+rec.base.add(0x33294+i*8).readU32().toString(16).padStart(8,'0').toUpperCase());
  const block=label+'\n'+words.join('\n')+'\n';
  ini=/^\[Gecko\]/m.test(ini)?ini.replace(/^\[Gecko\]/m,'[Gecko]\n'+block):ini+'\n[Gecko]\n'+block;
 }
 const lines=ini.split(/\r?\n/);let start=lines.findIndex(line=>line.trim()==='[Gecko_Enabled]');
 if(start<0)lines.push('','[Gecko_Enabled]',label);
 else{let end=start+1;while(end<lines.length&&!/^\s*\[/.test(lines[end]))end++;if(!lines.slice(start+1,end).some(line=>line.trim()===label))lines.splice(start+1,0,label);}
 ini=lines.join('\n');if(ini!==before)atomic(path,ini);
}


function installQuestPrompts(){
 const root='/data/user/0/org.dolphinemu.dolphinemu.quest.bully.recover2/files/',marker=root+'quest-prompts-r21.txt';
 try{if(File.readAllText(marker)==='r22')return;}catch(e){}
 const mkdir=new NativeFunction(Module.getGlobalExportByName('mkdir'),'int',['pointer','uint32']);
 let path=root;for(const part of ['Load','Textures','RB7E54']){path+=part+'/';mkdir(Memory.allocUtf8String(path),448);}
 const alphabet='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/';
 const pack={}; // Embedded textures omitted; see BUILD.md.
 for(const name of Object.keys(pack)){const s=pack[name],out=[];let acc=0,bits=0;for(const c of s){const n=alphabet.indexOf(c);if(n<0)break;acc=(acc<<6)|n;bits+=6;if(bits>=8){bits-=8;out.push((acc>>bits)&255);}}const file=new File(path+name,'wb');file.write(new Uint8Array(out).buffer);file.close();}
 File.writeAllText(marker,'r22');
}


function installReadableFont(){
 const rec=Process.findModuleByName('libbully_recovery.so');if(!rec)return;
 const s="" /* Embedded font omitted; see BUILD.md. */,alphabet='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/',out=[];let acc=0,bits=0;
 for(const c of s){const n=alphabet.indexOf(c);if(n<0)break;acc=(acc<<6)|n;bits+=6;if(bits>=8){bits-=8;out.push((acc>>bits)&255);}}
 const data=Memory.alloc(out.length);data.writeByteArray(out);
 // The atlas keeps this allocation; prevent ImGui from freeing Frida-owned memory.
 const configs=[];
 const hook=Interceptor.attach(rec.base.add(0xae130),{onEnter(args){
  if(args[3].isNull())return;
  const cfg=Memory.alloc(144);Memory.copy(cfg,args[3],144);cfg.add(52).writeU8(0);cfg.add(108).writeFloat(1.05);
  configs.push(cfg);args[1]=data;args[2]=ptr(out.length);args[3]=cfg;
 }});
 return {hook,data,configs};
}
function createMissionAudit(rec,read,mem,root){
 const valid=a=>a>=0x80000000&&a<0x94000000;
 function text(a){if(!valid(a))return '';let out='';for(let i=0;i<96;i+=4){const w=read(mem,a+i);for(let k=3;k>=0;k--){const c=(w>>>(k*8))&255;if(!c)return out;out+=String.fromCharCode(c);}}return out;}
 function list(){const manager=0x806390bc,base=read(mem,manager+0x4b8),count=read(mem,manager+0x74c);if(!valid(base)||count>1000)throw Error('Mission table unavailable');const rows=[];for(let i=0;i<count;i++)rows.push({id:i,name:text(read(mem,base+i*0x78+0x18))});File.writeAllText(root+'recovery-missions.json',JSON.stringify(rows));return rows;}
 return {list,start(id){if(!Number.isInteger(id)||id<0||id>=read(mem,0x806390bc+0x74c))throw Error('Invalid mission');rec.base.add(0x12b720).writeS32(id);}};
}


function createChemistryControls(main,rec){
 const system=main.base.add(0xdec9c8),get=off=>new NativeFunction(main.base.add(off),'pointer',['pointer'])(system);
 const mem=get(0x6b3a84),ppc=get(0x6b3af4),jit=get(0x6b3a44);
 const read=new NativeFunction(main.base.add(0x5bb354),'uint32',['pointer','uint32']);
 const write=new NativeFunction(main.base.add(0x5bb688),'void',['pointer','uint32','uint32']);
 const invalidate=new NativeFunction(main.base.add(0x6a6e78),'void',['pointer','pointer','pointer','uint32']);
 const address=0x802812c8,words=[478415356, 2091065876, 2160394520, 482738204, 2093298196, 2162557216, 671547412, 1098973280, 1023442984, 1627919048, 1424560186, 963117248, 2099988526, 2168979776, 2102085688, 2081114112, 1082261564, 2168979780, 2102020153, 1099038768, 738656266, 1099038732, 738656267, 1082261528, 2168657192, 961150977, 2437092648, 738852869, 1098907660, 945815553, 1317011488, 945815552, 1317011488, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 1616928864, 4, 8, 1, 2, 4, 8, 4, 8, 4, 32, 32, 32, 4, 1, 2, 1, 17, 2, 1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
 let held=0,edges=0,lastHeld=0,applied=false,applies=0,presses=0,lastError='',probe=false,lastProbe=0;
 const box=new ArrayBuffer(4),ui=new Uint32Array(box),fl=new Float32Array(box),rf=a=>{ui[0]=read(mem,a);return fl[0];};
 function guestState(){const p=read(mem,0x806ec1e8);if(p<0x80000000||p>0x93ffe000)return null;const step=read(mem,p+0x118);if(step>85)return null;const row=p+0x120+step*28;return {pointer:p.toString(16),vtable:read(mem,p).toString(16),step,action:read(mem,row),time:rf(p+0x114),start:rf(row+4),end:rf(row+8),flags:read(mem,row+12).toString(16),mash:read(mem,p+0x1528)};}
 const input=Interceptor.attach(rec.base.add(0x5f884),{onEnter(){
  const p=this.context.x19;let bits=0;
  if(p.add(0xc8).readU8()){if(p.add(0xc9).readU8())bits|=1;if(p.add(0xca).readU8())bits|=2;if(p.add(0xcc).readU8())bits|=16;}
  if(p.add(0x40).readU8()){if(p.add(0x41).readU8())bits|=4;if(p.add(0x42).readU8())bits|=8;if(p.add(0x44).readU8())bits|=32;}
  const fresh=bits&~lastHeld;edges|=fresh;if(fresh)presses++;lastHeld=held=bits;
 }});
 const cpu=Interceptor.attach(main.base.add(0x53e4dc),{onEnter(){
  try{
   const first=read(mem,address);
   if(first===0x9421ffe0){for(let i=0;i<words.length;i++)write(mem,words[i],address+i*4);for(let i=0;i<words.length*4;i+=32)invalidate(ppc.add(0x2550),mem,jit,(address+i)&0xffffffe0);applied=true;applies++;}
   else if(first!==words[0]){applied=false;throw Error('Unexpected chemistry function; patch withheld');}
   if(applied){write(mem,held,address+0x140);write(mem,edges,address+0x144);edges=0;}
   if(probe&&Date.now()-lastProbe>=80){lastProbe=Date.now();File.writeAllText('/data/user/0/org.dolphinemu.dolphinemu.quest.bully.recover2/files/chemistry-probe.json',JSON.stringify(guestState()));}
   lastError='';
  }catch(e){lastError=String(e);}
 }});
 return {setProbe(value){probe=!!value;},report(){return {applied,applies,presses,held,lastError,guest:guestState(),scheme:'Touch buttons; LT mash requires five taps'};},dispose(){input.detach();cpu.detach();}};
}


/* Recovery cheat synchronization. Address authority: recovered ELF + live probes.
   Infrastructure C0 is kept separate from user cheats and is never saved as a user cheat. */
function createCheatSync(main,rec,withEnv,jni,setBoolean){
 const NULL=ptr(0),root='/data/user/0/org.dolphinemu.dolphinemu.quest.bully.recover2/files/';
 const defs=[
 ['InfiniteHealth','Infinite Health',0x1741618],['NeverWanted','Never Wanted',0x1741619],
 ['InfiniteMoney','Infinite Money',0x174161a],['TimeFreeze','Freeze Time',0x174161b],
 ['InfiniteTickets','Infinite Tickets',0x174161c],['DelictsNeverPrescribe','Delicts Never Prescribe',0x174161d],
 ['AlwaysDisplayMoney','Always Display Money',0x174161e],['UnlimitedNewspapers','Unlimited Newspapers',0x174161f],
 ['CarnivalAdmission','Lifetime Carnival Admission',0x1741620],['InfiniteAmmo','Infinite Ammo',0x1a46820],
 ['FreeShops','Free Shops',0x1a46821],['InstantKO','Instant KO',0x1a46822],
 ['NoInventoryWeapons','No Inventory Weapons',0x1a46823],['HideClock','Hide Clock',0x1a46824],
 ['UnlimitedChocolates','Unlimited Chocolates',0x1a46825],['UnlimitedFlowers','Unlimited Flowers',0x1a46826]
 ];
 const active=new NativeFunction(main.base.add(0x53e094),'void',['pointer','uint64','pointer','uint16']);
 const sso=t=>{const p=Memory.alloc(24);p.writeByteArray(new Uint8Array(24));if(t.length>22)throw Error('SSO too long');p.writeU8(t.length*2);p.add(1).writeUtf8String(t);return p;};
 const game=sso('RB7E54'),carrier=Memory.alloc(104),count=294,lines=Memory.alloc(count*32);
 carrier.writeByteArray(new Uint8Array(104));lines.writeByteArray(new Uint8Array(count*32));
 carrier.add(0x18).writeByteArray(sso('Recovery bridge').readByteArray(24));carrier.add(0x60).writeU8(1);
 lines.writeU32(0xc0000000);lines.add(4).writeU32(0x125);
 for(let i=0;i<293;i++){lines.add((i+1)*32).writeU32(rec.base.add(0x33290+i*8).readU32());lines.add((i+1)*32+4).writeU32(rec.base.add(0x33294+i*8).readU32());}
 carrier.writePointer(lines);carrier.add(8).writePointer(lines.add(count*32));carrier.add(16).writePointer(lines.add(count*32));
 let forceDisable=false,last=null,initialized=false,queuedUndo=null,applies=0,undos=0,lastError='',reapplyAt=0,stage='idle',lastAppliedAt=0;
 const system=main.base.add(0xdec9c8),get=off=>new NativeFunction(main.base.add(off),'pointer',['pointer'])(system);
 const mem=get(0x6b3a84),ppc=get(0x6b3af4),jitInterface=get(0x6b3a44);
 const read=new NativeFunction(main.base.add(0x5bb354),'uint32',['pointer','uint32']);
 const write=new NativeFunction(main.base.add(0x5bb688),'void',['pointer','uint32','uint32']);
 const invalidate=new NativeFunction(main.base.add(0x6a6e78),'void',['pointer','pointer','pointer','uint32']);
 const undo=[['TimeFreeze',0x8010e028,0x988dd9e9,0x60000000],['NeverWanted',0x8017f4f0,0x901e1ef0,0x60000000],['FreeShops',0x8018b77c,0x90031e4c,0x60000000],['DelictsNeverPrescribe',0x801abc18,0x90031ef0,0x60000000],['AlwaysDisplayMoney',0x80344d8c,0xc01f0038,0xc01f002c],['NoInventoryWeapons',0x80143860,0xa8030002,0x38000000],['HideClock',0x8010e228,0x9bcdd9e2,0x60000000],['InfiniteAmmo',0x8014413c,0xb01d0000,null],['InfiniteAmmo',0x8008f8f8,0x7c05212e,null]];
 function branchToHandler(address,word){
  if((word&0xfc000003)!==0x48000000)return false;
  let displacement=word&0x03fffffc;if(displacement&0x02000000)displacement-=0x04000000;
  const target=(address+displacement)>>>0;return target>=0x80001800&&target<0x80003000;
 }
 const cpuHook=Interceptor.attach(main.base.add(0x53e4dc),{onEnter(){
  if(queuedUndo===null)return;const flags=queuedUndo;queuedUndo=null;
  for(const [key,address,original,patched] of undo){if(flags[key])continue;const current=read(mem,address);const recognized=patched===null?branchToHandler(address,current):current===patched;if(recognized){write(mem,original,address);invalidate(ppc.add(0x2550),mem,jitInterface,address&0xffffffe0);undos++;}}
 }});
 const actionHook=Interceptor.attach(rec.base.add(0x60dec),{onEnter(){if(this.context.x21.toInt32()===7)forceDisable=true;}});
 function persist(flags){
  const path=root+'Config/BullyVR.ini';let text=File.readAllText(path);
  for(const [key] of defs){const line='Cheat'+key+' = '+(flags[key]?'True':'False'),re=new RegExp('^Cheat'+key+'[ \\t]*=.*$','m');text=re.test(text)?text.replace(re,line):text.replace(/\s*$/,'\n')+line+'\n';}
  const temp=path+'.recovery-fixes.tmp';File.writeAllText(temp,text);
  const rename=new NativeFunction(Module.getGlobalExportByName('rename'),'int',['pointer','pointer']);
  if(rename(Memory.allocUtf8String(temp),Memory.allocUtf8String(path))!==0)throw Error('Cannot persist cheat settings');
 }

 function atomicText(path,text){
  const temporary=path+'.recovery-fixes.tmp';File.writeAllText(temporary,text);
  const rename=new NativeFunction(Module.getGlobalExportByName('rename'),'int',['pointer','pointer']);
  if(rename(Memory.allocUtf8String(temporary),Memory.allocUtf8String(path))!==0)throw Error('Cannot save '+path);
 }
 function readCheatFiles(){
  const result=new Map(),selected=new Set();
  function parse(text,loadSelected){let section='',current=null;
   for(const raw of text.split(/\r?\n/)){const line=raw.trim(),header=/^\[([^\]]+)\]/.exec(line);
    if(header){section=header[1];current=null;continue;}
    if(section==='Gecko'&&line.startsWith('$')){const name=line.slice(1).replace(/\s*\[[^\]]*\]\s*$/,'').trim();current={name,codes:[]};result.set(name,current);}
    else if(section==='Gecko'&&current){const words=/^([0-9A-Fa-f]{8})\s+([0-9A-Fa-f]{8})/.exec(line);if(words)current.codes.push([parseInt(words[1],16),parseInt(words[2],16)]);}
    else if(loadSelected&&section==='Gecko_Enabled'&&line.startsWith('$'))selected.add(line.slice(1).trim());
   }
  }
  parse(File.readAllText(root+'Sys/GameSettings/RB7E54.ini'),false);
  const userPath=root+'GameSettings/RB7E54.ini';let user='';try{user=File.readAllText(userPath);}catch(e){}parse(user,true);
  return {definitions:Array.from(result.values()),selected,user,userPath};
 }
 function saveSelection(file,names){
  const replacement=['[Gecko_Enabled]',...names.map(name=>'$'+name),''];
  let inside=false,found=false;const output=[];
  for(const line of file.user.split(/\r?\n/)){
   const header=/^\s*\[([^\]]+)\]/.exec(line);
   if(header){inside=header[1]==='Gecko_Enabled';if(inside){output.push(...replacement);found=true;continue;}}
   if(!inside)output.push(line);
  }
  if(!found)output.push('',...replacement);atomicText(file.userPath,output.join('\n'));
 }
 function apply(flags,disableAll,persistFiles){
  stage='read-definitions';const files=readCheatFiles();
  const selected=files.definitions.filter(code=>{if(['Recovery infrastructure','Recovery bridge'].includes(code.name))return false;const def=defs.find(d=>code.name==='Bully VR - '+d[1]);return !disableAll&&(def?flags[def[0]]:files.selected.has(code.name));});
  const enabled=selected.map(code=>code.name),references=[];
  const buffer=Memory.alloc((selected.length+1)*104);buffer.writeByteArray(new Uint8Array((selected.length+1)*104));buffer.writeByteArray(carrier.readByteArray(104));references.push(buffer,carrier,lines,game);
  for(let i=0;i<selected.length;i++){
   const definition=selected[i],object=buffer.add((i+1)*104),entries=Memory.alloc(Math.max(32,definition.codes.length*32));
   entries.writeByteArray(new Uint8Array(Math.max(32,definition.codes.length*32)));references.push(entries);
   for(let k=0;k<definition.codes.length;k++){entries.add(k*32).writeU32(definition.codes[k][0]);entries.add(k*32+4).writeU32(definition.codes[k][1]);}
   object.writePointer(entries);object.add(8).writePointer(entries.add(definition.codes.length*32));object.add(16).writePointer(entries.add(definition.codes.length*32));
   object.add(0x18).writeByteArray(sso(definition.name.replace(/^Bully VR - /,'').slice(0,22)).readByteArray(24));object.add(0x60).writeU8(1);
  }
  // The engine is enabled before boot, keeping JNI configuration out of this transaction.
  stage='activate-codes';active(buffer,selected.length+1,game,0);
  if(references.length!==selected.length+4)throw Error('Temporary native span lifetime');
  rec.base.add(0x12c11c).writeU32(0);queuedUndo=flags;applies++;lastAppliedAt=Date.now();
  stage='persist-flags';if(persistFiles){saveSelection(files,enabled);persist(flags);}
  main.base.add(0x1a4680c).writeU8(1);
  const state={applies,enabled,disabledAll:disableAll,carrier:true,undos};
  File.writeAllText(root+'recovery-cheats-status.json',JSON.stringify(state)+'\n');console.log('[RecoveryCheats] '+JSON.stringify(state));
 }

 return {
  tick(ready){if(!ready)return;try{
   const flags=Object.fromEntries(defs.map(([key,,off])=>[key,Boolean(main.base.add(off).readU8())]));
   const signature=JSON.stringify(flags),disabled=forceDisable;
   const missingCarrier=initialized&&Date.now()-lastAppliedAt>1500&&read(mem,0x80002338)===0x00d0c0de&&read(mem,0x80002340)!==0xc0000000;
   const reapply=(reapplyAt!==0&&Date.now()>=reapplyAt)||missingCarrier;
   if(!initialized||signature!==last||disabled||reapply){forceDisable=false;reapplyAt=0;apply(flags,disabled,initialized&&(signature!==last||disabled));initialized=true;last=signature;lastError='';}
  }catch(e){const error=stage+': '+String(e);if(error!==lastError){lastError=error;console.error('[RecoveryCheats] '+error+' '+e.stack);}}},
  setFlag(name,value){const d=defs.find(d=>d[0]===name);if(!d)throw Error('Unknown cheat '+name);main.base.add(d[2]).writeU8(value?1:0);},
  requestReapply(){reapplyAt=Date.now();},
  disableAll(){for(const [,,offset]of defs)main.base.add(offset).writeU8(0);forceDisable=true;},
  dispose(){cpuHook.detach();actionHook.detach();},
  report(){return {initialized,applies,undos,lastError};}
 };
}



// Proven by live Meta B pulses: physical3 mapped15(DPAD_DOWN) suppresses jump in
// native camera4; mapped5(JUMP) preserves camera4 and produces a native jump.
// Data-only fix. No native instruction patches, no JIT invalidation needed.
function createJumpMapSync(main,rec){
 const delta=rec.getExportByName('BullyRecovery_TestControl').sub(rec.base).toInt32()-0x5acb0;
 const mem=new NativeFunction(main.base.add(0x6b3a84),'pointer',['pointer'])(main.base.add(0xdec9c8));
 const read=new NativeFunction(main.base.add(0x5bb354),'uint32',['pointer','uint32']);
 const write=new NativeFunction(main.base.add(0x5bb688),'void',['pointer','uint32','uint32']);
 const valid=a=>a>=0x80000000&&a<=0x93ffffd8&&(a&3)===0;
 let patchedAddress=0,applies=0,restores=0,frames=0,lastError='',stopping=false,stopped=false,resolveStop=null;
 function state(){
  const ready=rec.base.add(0x12bf98+delta).readU8(),cinema=rec.base.add(0x12bf9c+delta).readU8(),
   owned=rec.base.add(0x12c150+delta).readU8(),enabled=rec.base.add(0x12b730+delta).readU8(),
   camera=read(mem,0x80624834),map=read(mem,0x806ebe64);
  return {ready,cinema,owned,enabled,camera,map,mapping:valid(map)?read(mem,map+0x24):null};
 }
 const hook=Interceptor.attach(main.base.add(0x53e4dc),{onEnter(){
  frames++;if(stopped)return;
  try{
   const s=state(),address=valid(s.map)?s.map+0x24:0;
   const active=!stopping&&!!s.ready&&!s.cinema&&!!s.owned&&!!s.enabled&&s.camera===4;
   // Input managers can be recreated after a load. Never write an inactive old
   // pointer that may already have been released/reused by the guest allocator.
   if(patchedAddress&&patchedAddress!==address)patchedAddress=0;
   if(address){
    const current=read(mem,address);
    if(active){
     if(current===15){write(mem,5,address);patchedAddress=address;applies++;console.log('[RecoveryJump] mapped B to native JUMP');}
     else if(current===5)patchedAddress=address;
     else throw Error('Unexpected physical3 mapping '+current+' at0x'+address.toString(16));
    }else if(patchedAddress===address){
     if(current===5){write(mem,15,address);restores++;}
     else if(current!==15)throw Error('Refusing to overwrite foreign physical3 mapping '+current);
     patchedAddress=0;
    }
   }
   lastError='';
   if(stopping&&!patchedAddress){stopped=true;setImmediate(()=>{hook.detach();if(resolveStop)resolveStop({restored:true,frames});});}
  }catch(e){const error=String(e);if(error!==lastError){lastError=error;console.error('[RecoveryJump] '+error);}}
 }});
 return {
  report(){return {applies,restores,frames,patchedAddress,lastError,stopped,delta,...state()};},
  dispose(){if(stopped)return Promise.resolve({restored:!patchedAddress,frames});stopping=true;return new Promise(resolve=>{resolveStop=resolve;});}
 };
}


function createDrawDistanceFloor(main,rec){
 const mem=new NativeFunction(main.base.add(0x6b3a84),'pointer',['pointer'])(main.base.add(0xdec9c8));
 const read=new NativeFunction(main.base.add(0x5bb354),'uint32',['pointer','uint32']);
 const write=new NativeFunction(main.base.add(0x5bb688),'void',['pointer','uint32','uint32']);
 const buf=new ArrayBuffer(4),u=new Uint32Array(buf),f=new Float32Array(buf);
 const valid=a=>a>=0x80000000&&a<=0x93ffff00&&(a&3)===0;
 let writes=0,lastFar=null,lastCamera=0,lastError='',enabled=true;
 const hook=Interceptor.attach(main.base.add(0x53e4dc),{onEnter(){
  try{
   if(!enabled||!rec.base.add(0x12bf98).readU8()||rec.base.add(0x12bf9c).readU8())return;
   // Never modify the reflection camera or its clipping planes.
   if((read(mem,0x80632aac)&0x0000ff00)!==0)return;
   const camera=read(mem,0x80624830);if(!valid(camera))return;
   u[0]=read(mem,camera+0xa4);const far=f[0];lastCamera=camera;lastFar=far;
   if(Number.isFinite(far)&&far>=100&&far<600){f[0]=600;write(mem,u[0],camera+0xa4);writes++;lastFar=600;}
   lastError='';
  }catch(e){lastError=String(e);}
 }});
 return {setEnabled(value){enabled=!!value;},report(){return {enabled,far:lastFar,camera:lastCamera,writes,lastError,floor:600,reflectionProtected:true};},dispose(){hook.detach();}};
}


function createPanelControl(main){
 const prefix='Java_org_dolphinemu_dolphinemu_features_settings_model_NativeConfig_';
 const setB=new NativeFunction(main.getExportByName(prefix+'setBoolean'),'void',['pointer','pointer','int','pointer','pointer','pointer','uchar']);
 const getB=new NativeFunction(main.getExportByName(prefix+'getBoolean'),'uchar',['pointer','pointer','int','pointer','pointer','pointer','uchar']);
 const ptrSize=Process.pointerSize,nullPtr=ptr(0);
 function jni(table,index,ret,args){return new NativeFunction(table.add(index*ptrSize).readPointer(),ret,args);}
 function withEnv(callback){
  const getVMs=new NativeFunction(Module.getGlobalExportByName('JNI_GetCreatedJavaVMs'),'int',['pointer','int','pointer']);
  const out=Memory.alloc(8),count=Memory.alloc(4);out.writePointer(nullPtr);count.writeS32(0);
  if(getVMs(out,1,count)!==0||count.readS32()<1)throw Error('Java VM unavailable');
  const vm=out.readPointer(),invoke=vm.readPointer(),envOut=Memory.alloc(8);
  const getEnv=jni(invoke,6,'int',['pointer','pointer','int']);
  let attached=false,rc=getEnv(vm,envOut,0x10006);
  if(rc===-2){rc=jni(invoke,4,'int',['pointer','pointer','pointer'])(vm,envOut,nullPtr);attached=true;}
  if(rc!==0)throw Error('JNI attachment failed: '+rc);
  const env=envOut.readPointer();
  try{return callback(env,env.readPointer());}finally{if(attached)jni(invoke,5,'int',['pointer'])(vm);}
 }
 function setPanelMode(flat,exact=false){
  withEnv((env,table)=>{
   const make=jni(table,167,'pointer',['pointer','pointer']);
   const del=jni(table,23,'void',['pointer','pointer']);
   const refs=[];const str=t=>{const p=make(env,Memory.allocUtf8String(t));refs.push(p);return p;};
   const file=str('GFX'),section=str('VR'),flatKey=str('FlatScreen'),exactKey=str('ExactScreenDepth');
   if(Boolean(getB(env,nullPtr,3,file,section,flatKey,0))!==flat)setB(env,nullPtr,4,file,section,flatKey,flat?1:0);
   setB(env,nullPtr,4,file,section,exactKey,exact?1:0);
   for(const p of refs)del(env,p);
  });
 }

setPanelMode.withEnv=withEnv;setPanelMode.jni=jni;return setPanelMode;
}


function createTravelMenu(rec,main,read,mem){
 const table=rec.base.add(0x120418),areas=rec.base.add(0x1401a0),refs=[],pages={};
 const row=(name,x=0,y=0,z=0,heading=0,area=0,ticket=false)=>({name,x,y,z,heading,area,ticket});
 // Coordinates from the original Recovery and the game's reference test script.
 pages.root=[{name:'School >',page:'school'},{name:'Town districts >',page:'town'},{name:'Carnival >',page:'carnival'},{name:'Interiors >',page:'interiors'}];
 const back={name:'< Regions',page:'root'};
 pages.school=[back,row('School gate',288.889,-74.866,5.9,1.57),row('Dorm courtyard',230,-73,6.3,1.57),row('Main building',211.6,-95.1,8.6,1.57),row('School gardens',271.37,-69.866,6.1,1.57),row('Library',-784.875,203.098,90.31,0,9),row('Boys dorm',-502.47,309.606,31.963,0,14),row('Auditorium',-778.496,292.221,77.951,0,19)];
 pages.town=[back,row('Commercial district',458.59,-80.7372,5.91727),row('Old Bullworth Vale',565.259,56.345,13.2),row('Residential streets',338.8,113.8,7.8,1.57),row('New Coventry',499.367,-245.193,1.94765,-1.5708),row('Blue Skies',343.065,-436.711,3,3.14)];
 pages.carnival=[back,row('Carnival entrance',226.2,412.6,9,2.61799,0,true),row('Carnival grounds',160,448,6.3,0,0,true)];
 pages.interiors=[back,row('Boys dorm',-502.47,309.606,31.963,0,14),row('Library',-784.875,203.098,90.31,0,9),row('Auditorium',-778.496,292.221,77.951,0,19),row('Boxing club',-702.445,372.796,293.937,4.407,27),row('Bike shop',-785.601,380.055,.73,0,29),row('Comic shop',-724.707,12.604,1.696,0,30),{name:'More interiors >',page:'interiors2'}];
 pages.interiors2=[back,row('Prep house',-569.538,133.33,46.3,0,32),row('Girls dorm',-454,311,3.5,0,35),row('Tenements',-544.463,-48.881,32.009,0,36),row('Funhouse',-700.446,-537.674,11.27,0,37,true),row('Asylum',-736.295,422.404,2.5,.524,38),row('Observatory',-696.615,61.633,21.089,1.571,40)];
 // Keep all allocations strongly referenced across render/input callbacks.
 for(const rows of Object.values(pages))for(const r of rows){r.label=Memory.allocUtf8String(r.name);refs.push(r.label);}
 const empty=Memory.allocUtf8String('-');refs.push(empty);
 let page='root',selected=null,lastError='',serial=0;
 Memory.protect(table,256,'rw-');
 Memory.protect(areas,32,'rwx');
 function show(name){
  page=name;const rows=pages[page];
  for(let i=0;i<8;i++){const r=rows[i],p=table.add(i*32);p.writePointer(r?r.label:empty);p.add(24).writePointer(r?r.label:empty);p.add(8).writeFloat(r&&r.x||0);p.add(12).writeFloat(r&&r.y||0);p.add(16).writeFloat(r&&r.z||0);p.add(20).writeFloat(r&&r.heading||0);areas.add(i*4).writeS32(r&&r.area||0);}
  rec.base.add(0x12c078).writeS32(0);
 }
 show('root');
 function choose(index){
  const r=pages[page][index];
  if(!r||r.page){if(r)show(r.page);return false;}
  // No teleport during a cinematic/mission: native scripts may own player state.
  if(rec.base.add(0x12bf9c).readU8()){lastError='Travel unavailable during a scene';return false;}
  selected={name:r.name,area:r.area||0,ticket:r.ticket,request:index};serial++;
  if(r.ticket){ticketPending=true;ticketStage=0;ticketStarted=Date.now();pendingIndex=index;return false;}
  return true;
 }
 const hook=Interceptor.attach(rec.base.add(0x5fe5c),{onEnter(){if(!choose(this.context.x21.toInt32()))this.context.x21=ptr('0xffffffffffffffff');}});
 const request=new NativeFunction(rec.base.add(0x739bc),'uchar',['uint32','uint32','float','float','float','float','int']);
 let ticketPending=false,pendingIndex=-1,ticketStage=0,ticketSerial=0,ticketStarted=0;
 function tick(){
  if(!ticketPending)return;
  if(Date.now()-ticketStarted>8000){ticketPending=false;ticketStage=0;lastError='Admission setup timed out; travel cancelled';return;}
  const mailbox=rec.base.add(0x12c11c).readU32(),ped=read(mem,0x80624204);
  if(ped<0x80000000||ped>=0x94000000||!mailbox)return;
  if(ticketStage===0){if(request(4,ped,0,0,0,0,479)){ticketSerial=rec.base.add(0x12c120).readU32();ticketStage=1;}}
  else if(read(mem,mailbox+0x30)===ticketSerial){rec.base.add(0x12b754).writeS32(pendingIndex);ticketPending=false;ticketStage=0;}
 }
 return {tick,show,request(index){if(choose(index))rec.base.add(0x12b754).writeS32(index);},report(){return {page,selected,serial,lastError,ticketPending,ticketStage,rows:pages[page].map(r=>r.name)};},dispose(){hook.detach();}};
}


function createSlingDraw(rec){
 let active=false,previousGrip=false,startDistance=0,pull=0,charged=false,releasingUntil=0,grabs=0,shots=0,lastError='';
 const hook=Interceptor.attach(rec.base.add(0x5f884),{onEnter(){try{
  const p=this.context.x19,item=rec.base.add(0x12b710).readS32(),grip=!!p.add(0x45).readU8();
  const eligible=(item===303||item===306)&&!!rec.base.add(0x12bf98).readU8()&&!rec.base.add(0x12bf9c).readU8()&&!rec.base.add(0x12bc2c).readU8()&&!rec.base.add(0x12bc3c).readU8();
  if(!eligible){active=false;charged=false;pull=0;previousGrip=grip;releasingUntil=0;return;}
  const valid=!!p.add(0x7c).readU8()&&!!p.add(0x104).readU8();
  const distance=valid?Math.hypot(...[0,4,8].map(i=>p.add(0x80+i).readFloat()-p.add(0x108+i).readFloat())):Infinity;
  if(!active&&grip&&!previousGrip&&distance<.18&&!p.add(0xcc).readU8()){active=true;startDistance=distance;charged=false;grabs++;}
  previousGrip=grip;
  if(active){
   // The grip near the pouch belongs to drawing the string, not equipment cycling.
   p.add(0x45).writeU8(0);p.add(0x4c).writeFloat(0);p.add(0x50).writeFloat(0);p.add(0x9c).writeU8(0);
   if(!grip||!valid){if(charged){shots++;releasingUntil=Date.now()+150;}active=false;pull=0;charged=false;}
   else{pull=Math.max(0,Math.min(1,(distance-startDistance)/.45));charged=charged||pull>.08;p.add(0xcc).writeU8(charged?1:0);p.add(0xd0).writeFloat(charged?Math.max(.26,pull):0);}
  }
  if(!active&&Date.now()<releasingUntil){p.add(0xcc).writeU8(0);p.add(0xd0).writeFloat(0);}
  lastError='';
 }catch(e){lastError=String(e);}}});
 return {report(){return {active,pull,charged,grabs,shots,lastError,power:'native charge timing; distance drives draw gesture'};},dispose(){hook.detach();}};
}


function createRound22(main,rec,read,mem){
 let frames=0,gaps=0,maxGap=0,lastTime=0,input={};const slingDraw=createSlingDraw(rec);
 const hook=Interceptor.attach(rec.base.add(0x60b68),{onEnter(){const p=this.context.x19;input={LT:p.add(0x44).readU8(),RT:p.add(0xcc).readU8(),trigger:p.add(0xd0).readFloat(),leftCombo:p.add(0x212).readU8(),rightCombo:p.add(0x213).readU8(),motionLeft:p.add(0x1f8).readU8(),motionRight:p.add(0x1f9).readU8()};}});
 let savedHud=null;
 function panels(flat){if(flat){if(!savedHud)savedHud=[...Array.from({length:6},(_,i)=>main.base.add(0x1741600+i*4).readFloat()),main.base.add(0xde4f04).readFloat()];for(let i=0;i<6;i++)main.base.add(0x1741600+i*4).writeFloat(0);main.base.add(0xde4f04).writeFloat(1);}else if(savedHud){for(let i=0;i<6;i++)main.base.add(0x1741600+i*4).writeFloat(savedHud[i]);main.base.add(0xde4f04).writeFloat(savedHud[6]);savedHud=null;}}
 const tick=Interceptor.attach(main.base.add(0x53e4dc),{onEnter(){const t=Date.now();if(lastTime){const gap=t-lastTime;if(gap>100)gaps++;maxGap=Math.max(maxGap,gap);}lastTime=t;frames++;}});
 return {panels,scopeActive(){return rec.base.add(0x12b710).readS32()===306&&!!rec.base.add(0x132660+12).readU32()&&!rec.base.add(0x12bf9c).readU8();},report(){return {slingDraw:slingDraw.report(),scopeNear:rec.base.add(0x132660+12).readU32(),flatHud:!!savedHud,mirrors:rec.base.add(0x132660+8).readU32(),frames,gaps,maxGap,input};},dispose(){hook.detach();tick.detach();slingDraw.dispose();}};
}


'use strict';
const build='recovery-r22-weapons-transitions';
let timer=null,jump=null,screenHook=null,testControl=null,heightHook=null,equipHook=null;
function initialize(){
 const rec=Process.findModuleByName('libbully_recovery.so'),main=Process.findModuleByName('libmain.so');
 if(!rec||!main||!rec.base.add(0x12bbd0).readPointer().equals(main.base))return false;
 if(!rec.getExportByName('BullyRecovery_TestControl').equals(rec.base.add(0x5acb0)))throw Error('Unexpected Recovery layout');
 const engineRunning=new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_NativeLibrary_IsRunning'),'uchar',['pointer','pointer']);
 if(!engineRunning(ptr(0),ptr(0)))return false;
 const root='/data/user/0/org.dolphinemu.dolphinemu.quest.bully.recover2/files/';
 const setPanel=createPanelControl(main),draw=createDrawDistanceFloor(main,rec);
 const tc=new NativeFunction(rec.getExportByName('BullyRecovery_TestControl'),'void',['int','float']);
 testControl=tc;
 const diagnostic=new NativeFunction(rec.getExportByName('BullyRecovery_Diagnostic'),'void',['int']);
 const equip=new NativeFunction(rec.base.add(0x5b6d4),'void',['int']);
 let heightResets=0,lastEquip=null;
 const saveVR=new NativeFunction(rec.base.add(0x688f8),'void',[]);
 try{if(File.readAllText(root+'hud-instructions-r21.txt')==='r21'){main.base.add(0x1741610).writeFloat(0);main.base.add(0x1741614).writeFloat(0);saveVR();File.writeAllText(root+'hud-instructions-r21.txt','r22-reset');}}catch(e){}
 try{if(File.readAllText(root+'hud-layout-r22.txt')!=='r22h')throw Error('old');}catch(e){main.base.add(0x1741610).writeFloat(.20);main.base.add(0x1741614).writeFloat(.08);saveVR();File.writeAllText(root+'hud-layout-r22.txt','r22h');}
 function cameraHeight(value,persist=false){
  if(!Number.isFinite(value))return;
  rec.base.add(0x12b73c).writeFloat(Math.max(1.2,Math.min(2.1,value)));
  rec.base.add(0x12c0f4).writeU8(1);
  if(persist)saveVR();
 }
 heightHook=Interceptor.attach(main.base.add(0x9176a4),{onEnter(){cameraHeight(1.6);heightResets++;}});
 equipHook=Interceptor.attach(rec.base.add(0x5b6d4),{onEnter(args){this.index=args[0].toInt32();this.before=rec.base.add(0x12b734).readS32();},onLeave(){lastEquip={index:this.index,before:this.before,pending:rec.base.add(0x12b734).readS32(),known:rec.base.add(0x12bf2c).readU8()};}});
 const unpause=new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_NativeLibrary_UnPauseEmulation'),'void',['pointer','pointer']);
 const running=new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_NativeLibrary_IsRunningAndUnpaused'),'uchar',['pointer','pointer']);
 const mem=new NativeFunction(main.base.add(0x6b3a84),'pointer',['pointer'])(main.base.add(0xdec9c8));
 const read=new NativeFunction(main.base.add(0x5bb354),'uint32',['pointer','uint32']);
 const valid=a=>a>=0x80000000&&a<0x94000000&&(a&3)===0;
 const b=new ArrayBuffer(4),u=new Uint32Array(b),f=new Float32Array(b),rf=a=>{u[0]=read(mem,a);return f[0];},vec=a=>[rf(a),rf(a+4),rf(a+8)];
 const travel=createTravelMenu(rec,main,read,mem),missionAudit=createMissionAudit(rec,read,mem,root);
 const chemistry=createChemistryControls(main,rec),round=createRound22(main,rec,read,mem);
 let lastDiagnostic=0;
 let control={},lastSequence=null,commandError='',last='',lastFlat=null,cheats=null,testTimer=null;
 function loadState(name){
  if(!/^[a-zA-Z0-9_.-]+\.sav$/.test(name))throw Error('Invalid state filename');
  rec.base.add(0x12c070).writeU8(0);rec.base.add(0x12b83c).writeS32(-1);rec.base.add(0x12b754).writeS32(-1);
  setPanel.withEnv((env,table)=>{
   const make=setPanel.jni(table,167,'pointer',['pointer','pointer']),del=setPanel.jni(table,23,'void',['pointer','pointer']);
   const value=make(env,Memory.allocUtf8String(root+'StateSaves/'+name));
   try{new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_NativeLibrary_LoadStateAs'),'void',['pointer','pointer','pointer'])(env,ptr(0),value);}finally{del(env,value);}
  });
  // The host keeps its active code list; recreate its handler after old RAM is restored.
  main.base.add(0xdefc50).writeU32(0);rec.base.add(0x12c11c).writeU32(0);
 }
 screenHook=Interceptor.attach(rec.base.add(0x62e90),{onEnter(){if(control.panels===false)return;const p=this.context.x19;p.add(0xb0).writeFloat(4.2);p.add(0xb4).writeFloat(2.3625);p.add(0xb8).writeFloat(6);}});
 timer=setInterval(()=>{
  try{
   try{control=JSON.parse(File.readAllText(root+'recovery-control.json'));}catch(e){}
   if(control.unpause&&!running(ptr(0),ptr(0)))unpause(ptr(0),ptr(0));
   if(!engineRunning(ptr(0),ptr(0)))return;
   if(control.sequence!==undefined&&control.sequence!==lastSequence){
    lastSequence=control.sequence;commandError='';
    try{
     if(control.listMissions)missionAudit.list();
     if(Number.isInteger(control.startMission))missionAudit.start(control.startMission);
     if(Number.isInteger(control.diagnostic))diagnostic(control.diagnostic);
     if(control.unpause)unpause(ptr(0),ptr(0));
     if(typeof control.loadState==='string')loadState(control.loadState);
     if(Number.isFinite(control.height))cameraHeight(control.height,true);
     if(control.recenter)new NativeFunction(main.getExportByName('Java_org_dolphinemu_dolphinemu_NativeLibrary_RequestOpenXRRecenter'),'void',['pointer','pointer'])(ptr(0),ptr(0));
     if(control.capture)rec.base.add(0x12bc34).writeU8(1);
     if(control.grantAll)rec.base.add(0x12bc30).writeU8(1);
     if(Number.isInteger(control.equip))equip(control.equip);
     if(control.cheat&&cheats)cheats.setFlag(control.cheat.name,control.cheat.value);
     if(control.disableCheats&&cheats)cheats.disableAll();
     if(Number.isFinite(control.rigYaw))rec.base.add(0x12bc20).writeFloat(control.rigYaw*Math.PI/180);
     if(typeof control.travelPage==='string')travel.show(control.travelPage);
     if(Number.isInteger(control.travel))travel.request(control.travel);
     if(Number.isInteger(control.button)){if(testTimer)clearTimeout(testTimer);tc(0,0);tc(0,1);const button=control.button;tc(button,Number.isFinite(control.value)?control.value:1);testTimer=setTimeout(()=>{tc(button,0);tc(0,0);},Math.min(control.holdMs||250,30000));}
    }catch(e){commandError=String(e);}
   }
   const scoped=round.scopeActive();
   const photoTool=rec.base.add(0x12b710).readS32()===328||rec.base.add(0x12b710).readS32()===329;
   rec.base.add(0x12b730).writeU8(photoTool?0:control.nativeCamera===false?0:1);
   const ready=Boolean(rec.base.add(0x12bf98).readU8()),cinema=Boolean(rec.base.add(0x12bf9c).readU8());
   if(ready&&!cinema)travel.tick();
   if(ready&&!jump)jump=createJumpMapSync(main,rec);
   if(ready&&!cheats)cheats=createCheatSync(main,rec,setPanel.withEnv,setPanel.jni,null);
   if(cheats&&control.cheatSync!==false)cheats.tick(ready);
   chemistry.setProbe(control.chemistryProbe);
   draw.setEnabled(control.drawDistance!==false);
   const flat=control.panels!==false&&(!ready||cinema||Boolean(rec.base.add(0x12c0e8).readU8())),exact=typeof control.exactDepth==='boolean'?control.exactDepth:!flat,mode=flat+':'+exact;
   main.base.add(0xde4edc).writeFloat(6);main.base.add(0xde4ee0).writeFloat(4.2);
   round.panels(flat);
   if(mode!==lastFlat){setPanel(flat,exact);lastFlat=mode;}
   const ped=read(mem,0x80624204),mailbox=rec.base.add(0x12c11c).readU32();
   const inventory=valid(ped)?read(mem,ped+0x300):0,held=valid(ped)?read(mem,ped+0x1ec):0;
   if(Date.now()-lastDiagnostic<1000)return;lastDiagnostic=Date.now();
   const state={build,round:round.report(),chemistry:chemistry.report(),travel:travel.report(),height:{value:rec.base.add(0x12b73c).readFloat(),resets:heightResets},language:rec.base.add(0x12bfa0).readS32(),equipmentKnown:rec.base.add(0x12bf2c).readU8(),lastEquip,guestEquip:valid(inventory)?{inventory:inventory.toString(16),desired:read(mem,inventory+0x20),held:held.toString(16),model:valid(held)?read(mem,held+0x110):0,cached:read(mem,ped+0x16b8),force:read(mem,ped+0x14c8),type:read(mem,ped+0x1450)}:null,sequence:lastSequence,commandError,running:!!running(ptr(0),ptr(0)),ready,cinema,flatPanel:flat,exactDepth:exact,control,cheats:cheats?cheats.report():null,rigYaw:rec.base.add(0x12bc20).readFloat(),nativeHeading:rec.base.add(0x12bc24).readFloat(),nativeForward:[0,4,8].map(x=>rec.base.add(0x12c160+x).readFloat()),controlled:Array.from({length:16},(_,i)=>rec.base.add(0x12c1ac+i*4).readFloat()),rwCamera:read(mem,0x806ec7bc),gameCamera:read(mem,0x80624830),cullingRadiusSquared:rec.base.add(0x33990).readFloat(),ammo:Array.from({length:23},(_,i)=>rec.base.add(0x12bdbc+i*4).readS32()),drawDistance:draw.report(),hands:{program:rec.base.add(0x12bb78).readU32(),failed:!!rec.base.add(0x12bba0).readU8()},jump:jump?jump.report():null,ped:valid(ped)?ped.toString(16):null,position:valid(ped)?vec(ped+4):null,gameFrame:rec.base.add(0x12bf78).readU64().toString(),ticks:read(mem,0x806eb740),clock:read(mem,0x806ead68).toString(16),nativeCamera:rec.base.add(0x12b730).readU8(),nativeViewValid:rec.base.add(0x12c1f0).readU8(),actual:rec.base.add(0x12b710).readS32(),pending:rec.base.add(0x12b734).readS32(),grantPending:rec.base.add(0x12bc30).readU8(),stock:Array.from({length:23},(_,i)=>rec.base.add(0x12bc4c+i*4).readS32()),mailbox:valid(mailbox)?{address:mailbox.toString(16),cmd:read(mem,mailbox),sequence:read(mem,mailbox+0x20),ack:read(mem,mailbox+0x30),ticks:read(mem,mailbox+0x28)}:null};
   const value=JSON.stringify(state);if(value!==last){File.writeAllText(root+'recovery-fixes-status.json',value+'\n');last=value;}
  }catch(e){console.error('[RecoveryControl] '+e);}
 },250);return true;
}
rpc.exports={init(){try{ensureRecoveryInfrastructure();installQuestPrompts();installReadableFont();}catch(e){console.error('[RecoveryPreflight] '+e);}const wait=setInterval(()=>{try{if(initialize())clearInterval(wait);}catch(e){console.error(String(e));}},100);},dispose(){if(timer)clearInterval(timer);if(screenHook)screenHook.detach();if(heightHook)heightHook.detach();if(equipHook)equipHook.detach();if(testControl)testControl(0,0);if(jump)return jump.dispose();}};
