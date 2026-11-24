// Avoid rebuilding the Gecko codelist while the player is navigating menus or
// while native streaming owns the scene. Flag changes remain pending naturally.
function createRecoveryPlusMenuSettle(delay=750){
 let stableSince=null;
 return {allow(busy,now){
  if(busy){stableSince=null;return false;}
  if(stableSince===null)stableSince=now;
  return now-stableSince>=delay;
 }};
}
