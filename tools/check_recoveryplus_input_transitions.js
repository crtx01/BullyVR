const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const memory=new Map(),hooks=new Map(),actions=[];
class P{
 constructor(n){this.n=n;}add(n){return new P(this.n+n);}sub(p){return new P(this.n-p.n);}toInt32(){return this.n|0;}
 readS32(){return memory.get(this.n)??0;}readU32(){return this.readS32()>>>0;}readU8(){return this.readS32()&255;}readFloat(){return memory.get(this.n)??0;}
 writeS32(v){memory.set(this.n,v);}writeU8(v){memory.set(this.n,v);}writeFloat(v){memory.set(this.n,v);}
}
const main={base:new P(0),getExportByName:name=>new P(name.endsWith('clearControlState')?3:name.endsWith('setControlState')?2:1)};
const rec={base:new P(0),getExportByName:()=>new P(0x5acb0)};
memory.set(0x3204c,-1);memory.set(0x32050,303);memory.set(0x32054,300);memory.set(0x321be,1);
const c=vm.createContext({Date,ptr:n=>new P(n),NativeFunction:function(at){return (...args)=>{if(at.n===2||at.n===3)actions.push({type:at.n,id:args[3],value:args[4]});return 0;};},Interceptor:{attach(at,h){hooks.set(at.n,h);return {detach(){}};}},setInterval(){return 1;},clearInterval(){},File:{writeAllText(){}}});
vm.runInContext(fs.readFileSync('src/recoveryplus/review.js','utf8'),c);
const api=c.createRecoveryPlusReview(main,rec,()=>0,{}),device=new P(0x30000000);
memory.set(0x12bf98,1);memory.set(0x12b754,-1);memory.set(0x12b734,-2);
const frame=()=>{hooks.get(0x5f884).onEnter.call({context:{x19:device}});hooks.get(0x60b68).onEnter.call({context:{x19:device}});};
memory.set(0x12b710,-1);frame();assert.equal(api.report().overrides,true);
for(const flag of [0x12c0e8,0x12bc2c,0x12bc3c,0x12bf9c,0x12c094,0x12c070]){
 memory.set(flag,1);frame();assert.equal(api.report().overrides,false,`release controls for ${flag.toString(16)}`);
 memory.set(flag,0);frame();assert.equal(api.report().overrides,true);
}
assert.ok(actions.some(a=>a.type===3&&a.id===31),'focus override cleared in menus');
// Held melee item with stale ranged row must retain motion pulses.
memory.set(0x12b710,300);memory.set(0x12c090,1);memory.set(device.n+0x212,1);frame();assert.equal(memory.get(device.n+0x212),1);
// Held ranged item with stale melee row must suppress cached punch pulses.
memory.set(0x12b710,303);memory.set(0x12c090,2);memory.set(device.n+0x212,1);frame();assert.equal(memory.get(device.n+0x212),0);
assert.equal(api.report().errors,'');
console.log('Passed: native/VR menu, help, cinema, class and travel release/reacquire inputs; actual held weapon controls punch suppression across stale selections');
