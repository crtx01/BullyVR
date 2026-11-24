// Temporarily move the HUD projection farther away while the controls card is
// open. Restore the user's setting on close, without persisting a new HUD value.
function createRecoveryPlusHelpDistance(main,rec){
 const depth=main.base.add(0xde4ed8),help=rec.base.add(0x12bc3c);
 let saved=null,applied=null;
 return {tick(){
  if(help.readU8()){
   if(saved===null){const value=depth.readFloat();if(!Number.isFinite(value)||value<.1||value>20)return;saved=value;applied=Math.max(2.5,value);}
   depth.writeFloat(applied);
  }else if(saved!==null){
   if(Math.abs(depth.readFloat()-applied)<.001)depth.writeFloat(saved);
   saved=null;applied=null;
  }
 },report(){return {active:saved!==null,previous:saved,distance:applied};}};
}
