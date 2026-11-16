// Enter the director view immediately. Require a stable return to gameplay
// before recompiling back to stereo: fades can briefly clear the cinema flag.
function createRecoveryPlusTransitionFilter(graceMs=350){
 let flat=null,exitSince=null,heldExits=0;
 return {
  step(wanted,now){
   if(flat===null||wanted){flat=!!wanted;exitSince=null;return flat;}
   if(!flat)return false;
   if(exitSince===null)exitSince=now;
   if(now-exitSince<graceMs){heldExits++;return true;}
   flat=false;exitSince=null;return false;
  },
  report(){return {flat,exitPending:exitSince!==null,heldExits};}
 };
}
