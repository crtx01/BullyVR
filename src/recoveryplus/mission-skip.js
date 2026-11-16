function createRecoveryPlusMissionSkip(main,rec,read,mem,payload) {
 const write=new NativeFunction(main.base.add(0x5bb688),'void',['pointer','uint32','uint32']);
 let pending=null,mailbox=0,lastResult=null;
 const valid=p=>p>=0x80001810&&p<0x80002ff0;
 function install(){
  if(mailbox&&read(mem,mailbox+12)===0x534b4950)return true;
  mailbox=0;
  const magic=read(mem,0x80001800);
  if(magic<0xd01f1bad||magic>0xd01f1bb2)return false;
  let start=0;
  for(let a=0x80001810;a<0x80002ff0;a+=4)if(read(mem,a)===0x00d0c0de&&read(mem,a+4)===0x00d0c0de){start=a+8;break;}
  if(!start)return false;
  for(let a=start;a<0x80002ff0;a+=8){
   const code=read(mem,a);
   if(code===0xc0000000){
    const bytes=read(mem,a+4)*8;if(bytes>4000||!valid(a+8+bytes))return false;
    const slot=a+8+payload.mailboxOffset;
    if(payload.mailboxOffset+16<=bytes&&read(mem,slot+12)===0x534b4950){mailbox=slot;return true;}
    a+=bytes;continue;
   }
   if(code===0xf0000000){
    const bytes=payload.words.length*4;if(a+8+bytes+8>=0x80002ff0)return false;
    for(let i=0;i<payload.words.length;i++)write(mem,payload.words[i],a+8+i*4);
    write(mem,0xf0000000,a+8+bytes);write(mem,0,a+12+bytes);
    write(mem,bytes/8,a+4);write(mem,0xc0000000,a);
    write(mem,0xd01f1bad,0x80001800);mailbox=a+8+payload.mailboxOffset;return true;
   }
  }
  return false;
 }
 const hook=Interceptor.attach(main.base.add(0x53e4dc),{onEnter(){
  if(!pending)return;
  if(!rec.base.add(0x12bf98).readU8()||rec.base.add(0x12bf9c).readU8()||rec.base.add(0x12c070).readU8()||rec.base.add(0x12b754).readS32()!==-1||read(mem,0x80639578)!==pending.id){
   // A queued completion belongs only to the mission that requested it.
   if(mailbox&&read(mem,mailbox+12)===0x534b4950)write(mem,0,mailbox);
   lastResult={status:'transition or mission changed; cancelled',id:pending.id};pending=null;return;
  }
  if(Date.now()-pending.started>4000){lastResult={status:'timeout',id:pending.id};pending=null;return;}
  if(!install())return;
  if(!pending.sent){
   if(read(mem,0x80639578)!==pending.id){lastResult={status:'mission changed; cancelled'};pending=null;return;}
   write(mem,pending.id,mailbox+4);write(mem,0,mailbox+8);write(mem,1,mailbox);pending.sent=true;
  }else{
   const result=read(mem,mailbox+8)|0;
   if(result){lastResult={status:result===1?'native completion requested':'rejected',id:pending.id};pending=null;}
  }
 }});
 return {request(id){if(!Number.isInteger(id)||id<0||pending)return false;pending={id,started:Date.now(),sent:false};return true;},report(){return {pending:pending&&pending.id,lastResult};},dispose(){hook.detach();}};
}
