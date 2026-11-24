const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const runtime=fs.readFileSync('build/recoveryplus-review-runtime.js','utf8');
const start=runtime.indexOf('function createTravelMenu('),end=runtime.indexOf('function createSlingDraw(',start);
const memory=new Map(),guest=new Map([[0x80639578,0xffffffff]]),hooks=[];
class P{
 constructor(n){this.n=n;}add(n){return new P(this.n+n);}writePointer(v){memory.set(this.n,v);}writeFloat(v){memory.set(this.n,v);}writeS32(v){memory.set(this.n,v);}readS32(){return memory.get(this.n)??0;}readU32(){return this.readS32()>>>0;}readU8(){return this.readS32()&255;}
}
memory.set(0x12b754,-1);memory.set(0x12b734,-2);memory.set(0x12b738,-1);memory.set(0x12bf98,1);
const context=vm.createContext({console,Memory:{allocUtf8String:s=>s,protect(){}},ptr:n=>new P(Number(n)),NativeFunction:function(){return ()=>true;},Interceptor:{attach(at,callbacks){hooks.push(callbacks);return {detach(){}};}},createRecoveryPlusMissionSkip(){return {request(){return true;},report(){return {};}}}});
vm.runInContext(runtime.slice(start,end),context);
const menu=context.createTravelMenu({base:new P(0)},{},(_,a)=>guest.get(a)??0,{});
assert.equal(menu.report().page,'root','startup must not read ticketPending in TDZ');
menu.show('town');assert.equal(menu.report().page,'town');
menu.request(-1);assert.equal(memory.get(0x12b754),-1);
menu.request(99);assert.equal(memory.get(0x12b754),-1);
guest.set(0x80639578,8);menu.request(1);assert.equal(memory.get(0x12b754),-1);assert.match(menu.report().lastError,/mission/);
guest.set(0x80639578,0xffffffff);memory.set(0x12c094,1);menu.request(1);assert.equal(memory.get(0x12b754),-1);
memory.set(0x12c094,0);memory.set(0x12b734,303);menu.request(1);assert.equal(memory.get(0x12b754),-1);
memory.set(0x12b734,-2);menu.request(1);assert.equal(memory.get(0x12b754),1);
menu.show('school');assert.equal(menu.report().page,'town','pending destination must remain immutable');
memory.set(0x12b754,-1);memory.set(0x12c070,1);menu.show('school');assert.equal(menu.report().page,'town');
memory.set(0x12c070,0);menu.show('school');assert.equal(menu.report().page,'school');
menu.show('missing');assert.equal(menu.report().page,'school');
console.log('Passed: startup, invalid rows, mission/class/equip gates, accepted free travel, frozen destination, invalid page');
menu.show('carnival');menu.request(1);assert.equal(menu.report().ticketPending,true);
guest.set(0x80639578,8);menu.tick();assert.equal(menu.report().ticketPending,false);
assert.equal(memory.get(0x12b754),-1,'admission must not resume travel in a new mission');
guest.set(0x80639578,0xffffffff);menu.request(1);memory.set(0x12bf9c,1);menu.tick();
assert.equal(menu.report().ticketPending,false,'scene entry cancels pending admission');
memory.set(0x12bf9c,0);menu.request(1);memory.set(0x12b734,303);menu.tick();
assert.equal(menu.report().ticketPending,false,'equipment change cancels pending admission');
memory.set(0x12b734,-2);menu.request(1);guest.set(0x80624204,0x85000000);memory.set(0x12c11c,0x80004000);menu.tick();
assert.equal(menu.report().ticketStage,0,'unmapped guest gap cannot be passed as player');
guest.set(0x80624204,0x80010000);memory.set(0x12c11c,0x817ffff0);menu.tick();
assert.equal(menu.report().ticketStage,0,'mailbox span must fit physical RAM');
console.log('Passed: admission cancellation on mission/scene/equipment change and guest pointer bounds');
// Traverse the real composed location and mission menu graph. Keep gameplay
// unavailable so selecting leaves cannot start native travel or missions.
memory.set(0x12c094,1);menu.tick();memory.set(0x12c094,0);memory.set(0x12bf98,0);
const pending=['root'],seen=new Set();let selectedRows=0;
while(pending.length){
 const name=pending.shift();if(seen.has(name)||name==='confirm')continue;seen.add(name);
 menu.show(name);const rows=menu.report().rows.length;
 for(let i=0;i<rows;i++){
  menu.show(name);menu.request(i);selectedRows++;
  const next=menu.report().page;
  if(next==='confirm'){
   menu.request(1);assert.equal(memory.get(0x12b720)||0,0,'loading gate must reject mission start');
  }else if(next!==name&&!seen.has(next))pending.push(next);
  assert.equal(memory.get(0x12b754),-1,'menu sweep cannot queue travel during loading');
 }
 menu.show(name);menu.request(-1);menu.request(999);menu.show('missing-page');
 assert.equal(menu.report().page,name);
}
assert.ok(seen.size>=30,`expected complete mission/location graph, visited ${seen.size}`);
console.log(`Passed: ${seen.size} actual location/mission pages, ${selectedRows} rows, confirmations and invalid navigation; native VR tabs require headset tests`);
