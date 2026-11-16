// Align the digital readout with the existing wrist face and increase legibility.
// Exact r22 instructions are checked before changing any code.
function applyRecoveryPlusWristDisplay(rec) {
 const patches = [
  [0x6944c,0x529999a8,0x52824de8], [0x69458,0x72a79988,0x72a7a068],
  [0x6945c,0x72a7a989,0x72a79989],
  [0x695bc,0x528e978a,0x529374ca], [0x695d8,0x72a7926a,0x72a7988a]
 ];
 for(const [offset,before,after] of patches) {
  const value=rec.base.add(offset).readU32();
  if(value!==before&&value!==after)throw Error('Unsupported wrist display instruction');
 }
 for(const [offset,before,after] of patches)Memory.patchCode(rec.base.add(offset),4,p=>p.writeU32(after));
 return {position:[0,.032,.025],fontScale:.024,clock:'native game time'};
}
