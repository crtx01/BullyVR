const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const memory=new Map(),guest=new Map(),hooks=new Map();let time=1000,reads=[];
class P{constructor(n){this.n=n;}add(n){return new P(this.n+n);}sub(p){return new P(this.n-p.n);}toInt32(){return this.n|0;}readS32(){return memory.get(this.n)??0;}readU32(){return this.readS32()>>>0;}readU8(){return this.readS32()&255;}writeS32(v){memory.set(this.n,v);}}
const context=vm.createContext({Date:{now:()=>time},ptr:n=>new P(n),NativeFunction:function(at){return at.n===0x739bc?()=>{memory.set(0x12c120,77);return true;}:()=>{};},Interceptor:{attach(at,h){hooks.set(at.n,h);return {detach(){}};}},setInterval(){return 1;},clearInterval(){},File:{writeAllText(){}}});
vm.runInContext(fs.readFileSync('src/recoveryplus/review.js','utf8'),context);
const main={base:new P(0),getExportByName:()=>new P(1)},rec={base:new P(0),getExportByName:()=>new P(0x5acb0)};
const api=context.createRecoveryPlusReview(main,rec,(_,a)=>{reads.push(a);return guest.get(a)??0;},{});
memory.set(0x12bf98,1);memory.set(0x12b754,-1);memory.set(0x12c11c,0x80004000);
const equip=()=>hooks.get(0x5b6d4).onEnter([new P(0)]),tick=()=>hooks.get(0x53e4dc).onEnter();
equip();guest.set(0x80624204,0x817ffffc);tick();assert.deepEqual(reads,[0x80624204],'reject player whose fields extend beyond RAM');
time=8000;tick();assert.equal(api.report().fists,false,'invalid state must still time out');
equip();guest.set(0x80624204,0x80010000);guest.set(0x80010300,0x80013000);guest.set(0x800101ec,0);guest.set(0x800116b8,0xffffffff);guest.set(0x80013020,0);
tick();guest.set(0x80004030,77);tick();tick();tick();
assert.equal(api.report().fistCompleted,1,'null held pointer is valid unequipped state');
assert.ok(!reads.includes(0x110),'never dereference an absent held item');
equip();memory.set(0x12c070,1);tick();assert.equal(api.report().fists,false,'travel cancels retries');
console.log('Passed: full player bounds, invalid-state timeout, empty hands acknowledgement, travel cancellation');
