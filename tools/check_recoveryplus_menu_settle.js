const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const c=vm.createContext({});vm.runInContext(fs.readFileSync('src/recoveryplus/menu-settle.js','utf8'),c);
const g=c.createRecoveryPlusMenuSettle();
assert.equal(g.allow(true,0),false);assert.equal(g.allow(false,100),false);
assert.equal(g.allow(false,849),false);assert.equal(g.allow(false,850),true);
assert.equal(g.allow(true,851),false);assert.equal(g.allow(false,900),false);
assert.equal(g.allow(false,1650),true);
for(let i=0;i<100;i++){assert.equal(g.allow(true,2000+i*100),false);assert.equal(g.allow(false,2050+i*100),false);}
assert.equal(g.allow(false,12700),true);
console.log('Passed: codelist updates deferred until 750ms of uninterrupted gameplay; repeated menu transitions restart the delay');
