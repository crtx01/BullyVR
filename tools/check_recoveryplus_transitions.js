const fs=require('fs'),vm=require('vm'),assert=require('node:assert/strict');
const context=vm.createContext({});
vm.runInContext(fs.readFileSync('src/recoveryplus/transitions.js','utf8'),context);
const filter=context.createRecoveryPlusTransitionFilter();
assert.equal(filter.step(false,0),false);
assert.equal(filter.step(true,100),true); // Cutscene begins without delay.
assert.equal(filter.step(false,200),true);
assert.equal(filter.step(true,250),true); // Brief false flag during a fade.
assert.equal(filter.step(false,300),true);
assert.equal(filter.step(false,649),true);
assert.equal(filter.step(false,650),false);
assert.equal(filter.step(false,1000),false);
assert.equal(filter.step(true,1001),true); // Next scene enters immediately.
console.log('Passed: immediate cinematic entry, fade flicker, stable stereo return, consecutive scene');
