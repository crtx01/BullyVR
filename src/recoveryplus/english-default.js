// Fresh installations start with English VR menus. Existing explicit choices
// remain intact, including Portuguese selected in the VR settings menu.
function applyRecoveryPlusEnglishDefault(rec,root){
 const path=root+'Config/RecoveryControls.ini';let contents='';
 try{contents=File.readAllText(path);}catch(e){}
 let match=contents.match(/^Language\s*=\s*([01])\s*$/m);
 if(!match){contents+=(contents.endsWith('\n')||!contents?'':'\n')+'Language=1\n';File.writeAllText(path,contents);}
 rec.base.add(0x12bfa0).writeS32(match?Number(match[1]):1);
}
