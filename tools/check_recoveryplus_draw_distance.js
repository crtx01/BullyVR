const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const runtime=fs.readFileSync('build/recoveryplus-review-runtime.js','utf8');
const start=runtime.indexOf('function createDrawDistanceFloor('),end=runtime.indexOf('function createPanelControl(',start);
assert.ok(start>=0&&end>start);
const guest=new Map(),host=new Map([[0x12bf98,1]]);let hook,reads=0,writes=0;
const bits=v=>{const b=Buffer.alloc(4);b.writeFloatLE(v);return b.readUInt32LE();};
class P{constructor(n){this.n=n;}add(n){return new P(this.n+n);}readU8(){return host.get(this.n)||0;}}
const context=vm.createContext({NativeFunction:function(p){if(p.n===0x6b3a84)return ()=>({});if(p.n===0x5bb354)return (_,a)=>{reads++;return guest.get(a)||0;};return (_,v,a)=>{writes++;guest.set(a,v);};},Interceptor:{attach(_,h){hook=h;return {detach(){}};}}});
vm.runInContext(runtime.slice(start,end),context);
const draw=context.createDrawDistanceFloor({base:new P(0)},{base:new P(0)});
guest.set(0x80624830,0x90010000);guest.set(0x900100a4,bits(300));
for(let i=0;i<80;i++)hook.onEnter();assert.equal(draw.report().far,650);assert.equal(writes,1);assert.ok(reads<=30,'amortize guest reads');
guest.set(0x80632aac,0x100);guest.set(0x900100a4,bits(300));for(let i=0;i<8;i++)hook.onEnter();assert.equal(writes,1,'reflection planes untouched');
guest.set(0x80632aac,0);guest.set(0x80624830,0x85000000);for(let i=0;i<8;i++)hook.onEnter();assert.equal(writes,1,'guest gap rejected');
console.log('Passed: 650 floor, amortized reads, reflection protection, physical-RAM bounds');
