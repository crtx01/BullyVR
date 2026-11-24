const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const memory=new Map();let hook,invalidates=0;
class P{add(){return this;}}
const main={base:new P()},rec={base:{add(){return {readU8(){return 1;}};}}};
const payload={site:0x80362078,original:0x38610010,entry:4,returnOffset:8,words:[0x4e800020,0x60000000,0,0x48554443]};
const context=vm.createContext({NativeFunction:function(){return ()=>{invalidates++;return new P();};},Interceptor:{attach(_,h){hook=h;return {detach(){}};}}});
// Native constructor order: writer, state getter, JIT getter, invalidate.
let n=0;context.NativeFunction=function(){n++;if(n===1)return (_,v,a)=>memory.set(a,v>>>0);if(n===4)return ()=>invalidates++;return ()=>new P();};
vm.runInContext(fs.readFileSync('src/recoveryplus/tutorial-panel.js','utf8'),context);
memory.set(payload.site,payload.original);memory.set(0x80001810,0x00d0c0de);memory.set(0x80001814,0x00d0c0de);memory.set(0x80001818,0xf0000000);
const api=context.createRecoveryPlusTutorialPanel(main,rec,(_,a)=>memory.get(a)||0,{},payload);
hook.onEnter();const body=api.report().installed;assert.ok(body);const branch=memory.get(payload.site);
hook.onEnter();assert.equal(memory.get(payload.site),branch);
memory.set(body+4,0xdeadbeef);hook.onEnter();assert.equal(memory.get(payload.site),payload.original);assert.equal(api.report().installed,0);
assert.match(api.report().error,/replaced/);assert.ok(invalidates>=4);
console.log('Passed: valid slot retained; reused/corrupted slot restores native instruction and invalidates JIT');
