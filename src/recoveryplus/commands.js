// Diagnostic actions are one-shot commands, never startup preferences.
// Seed from the existing file so reopening the game cannot replay a checkpoint
// load, mission entry, teleport, equip request or capture from a prior session.
function createRecoveryPlusCommandGate(initial) {
 let last = initial && initial.sequence;
 return {
  accept(control) {
   if (!control || control.sequence === undefined || control.sequence === last) return false;
   last = control.sequence;
   return true;
  }
 };
}
