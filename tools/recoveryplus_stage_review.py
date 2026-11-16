"""Compose the additive review into the ORIGINAL Recovery+ script, not r4.

This stages source only. It does not sign, install, or replace the Quest app.
"""
import base64
import json
import re
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
source = (ROOT / 'recovered/recoveryplus-original/libbully_runtime.script.so').read_text(encoding='utf8')
revision = os.environ.get('BULLY_REVIEW_REVISION', 'r24')
build_date = os.environ.get('BULLY_REVIEW_DATE', '2026-11-12')
assert re.fullmatch(r'r\d+', revision) and re.fullmatch(r'\d{4}-\d{2}-\d{2}', build_date)
source = source.replace("const build='recovery-r22-weapons-transitions';", f"const build='recoveryplus-{revision}-stability-{build_date.replace('-', '')}';")
pack_dir = ROOT / 'build/recoveryplus-prompts'
manifest = json.loads((pack_dir / 'manifest.json').read_text())
pack = {p.name: base64.b64encode(p.read_bytes()).decode() for p in sorted(pack_dir.glob('*.png'))}
assert len(pack) == manifest['texture_count']
source, count = re.subn(r' const pack=\{[^\n]+\};', lambda _: ' const pack=' + json.dumps(pack, separators=(',', ':')) + ';', source, count=1)
assert count == 1
catalog=json.loads((ROOT/'Documentation/recoveryplus/mission-catalog.json').read_text())
pages={'missions':[{'name':'< Locais / Locations','page':'root'}]}
for key,title,category in [('story','Historia / Story','story_mission_stage'),('side','Secundarias / Side','side_mission_stage'),('activity','Atividades / Activities','activity_stage'),('class','Aulas / Classes','class')]:
    rows=[dict(name=r['name'],mission=r['id']) for r in catalog['records'] if r['category']==category]
    chunks=[rows[i:i+6] for i in range(0,len(rows),6)]
    pages['missions'].append(dict(name=title+' >',page=f'mission_{key}_0'))
    for i,chunk in enumerate(chunks):
        page=[dict(name='< Missoes / Missions',page='missions')]+chunk
        if i+1<len(chunks):page.append(dict(name='Proxima / Next >',page=f'mission_{key}_{i+1}'))
        pages[f'mission_{key}_{i}']=page
anchor_pages = ' // Keep all allocations strongly referenced across render/input callbacks.'
assert source.count(anchor_pages)==1
source=source.replace(anchor_pages, " Object.assign(pages,"+json.dumps(pages,separators=(',',':'))+");\n pages.root.push({name:'Missoes / Missions >',page:'missions'},{name:'Pular missao atual / Skip current',skipCurrent:true});\n"+anchor_pages)
source=source.replace('const table=rec.base.add(0x120418),areas=rec.base.add(0x1401a0),refs=[],pages={};',
 '''const table=rec.base.add(0x120418),areas=rec.base.add(0x1401a0),refs=[],pages={};
 const skip=createRecoveryPlusMissionSkip(main,rec,read,mem,''' + (ROOT/'build/recoveryplus-mission-skip.json').read_text() + ''');
 let missionConfirm=null;
 function confirmation(action){
  missionConfirm=action;
  pages.confirm=[{name:'< Cancelar / Cancel',page:'root'},
   {name:action.skip?'CONFIRMAR concluir / skip':'CONFIRMAR iniciar / start',confirmMission:true}];
  for(const r of pages.confirm){r.label=Memory.allocUtf8String(r.name);refs.push(r.label);}
  show('confirm');
 }
''')
old='  const r=pages[page][index];\n  if(!r||r.page)'
new='''  const r=pages[page][index];
  if(r&&(r.skipCurrent||Number.isInteger(r.mission))){
   if(rec.base.add(0x12bf9c).readU8()){lastError='Aguarde o fim da cena / Wait for scene';return false;}
   const current=read(mem,0x80639578)|0;
   if(r.skipCurrent&&current<0){lastError='Sem missao ativa / No active mission';return false;}
   confirmation({skip:!!r.skipCurrent,id:r.skipCurrent?current:r.mission});return false;
  }
  if(r&&r.confirmMission){
   const action=missionConfirm;missionConfirm=null;show('root');
   if(!action||rec.base.add(0x12bf9c).readU8())return false;
   if(action.skip)skip.request(action.id);else rec.base.add(0x12b720).writeS32(action.id);
   return false;
  }
  if(!r||r.page)'''
assert source.count(old)==1
source=source.replace(old,new)
travel_show=' function show(name){\n  page=name;const rows=pages[page];'
assert source.count(travel_show)==1
source=source.replace(travel_show,''' function travelBusy(){return !!rec.base.add(0x12c070).readU8()||rec.base.add(0x12b754).readS32()!==-1||ticketPending;}
 function show(name){
  if(travelBusy()){lastError='Wait for travel to finish';return false;}
  if(!Object.prototype.hasOwnProperty.call(pages,name)){lastError='Unknown location page';return false;}
  page=name;const rows=pages[page];''')
# The declaration must precede initial show('root'): show now checks the gate.
source=source.replace(" let page='root',selected=null,lastError='',serial=0;", " let page='root',selected=null,lastError='',serial=0;\n let ticketPending=false,pendingIndex=-1,ticketStage=0,ticketSerial=0,ticketStarted=0;")
source=source.replace(' let ticketPending=false,pendingIndex=-1,ticketStage=0,ticketSerial=0,ticketStarted=0;\n function tick(){',' function tick(){')
choose_anchor=' function choose(index){\n  const r=pages[page][index];'
assert source.count(choose_anchor)==1
source=source.replace(choose_anchor,''' function choose(index){
  if(!Number.isInteger(index)||index<0||index>=pages[page].length)return false;
  if(travelBusy()){lastError='Wait for travel to finish';return false;}
  const r=pages[page][index];''')
travel_anchor='  selected={name:r.name,area:r.area||0,ticket:r.ticket,request:index};serial++;'
assert source.count(travel_anchor)==1
source=source.replace(travel_anchor,'''  if(!rec.base.add(0x12bf98).readU8()||rec.base.add(0x12c094).readU8()||rec.base.add(0x12c0e8).readU8()){
   lastError='Travel unavailable during loading, class or native menu';return false;
  }
  if((read(mem,0x80639578)|0)!==-1){lastError='Finish or skip the current mission before travel';return false;}
  if(rec.base.add(0x12b734).readS32()!==-2||rec.base.add(0x12b738).readS32()!==-1){lastError='Wait for equipment';return false;}
  rec.base.add(0x12b738).writeS32(-1);
'''+travel_anchor)
source=source.replace('report(){return {page,selected,serial,lastError,ticketPending,ticketStage,rows:',
                      'report(){return {missionSkip:skip.report(),page,selected,serial,lastError,ticketPending,ticketStage,rows:')
# Admission may finish after a mission, weapon change or scene transition starts.
# Revalidate on every tick, not only when the user selected the destination.
ticket_anchor='  if(!ticketPending)return;'
assert source.count(ticket_anchor)==1
source=source.replace(ticket_anchor,ticket_anchor+'''
  if(!rec.base.add(0x12bf98).readU8()||rec.base.add(0x12bf9c).readU8()||rec.base.add(0x12c094).readU8()||
     rec.base.add(0x12c070).readU8()||rec.base.add(0x12b754).readS32()!==-1||
     rec.base.add(0x12b734).readS32()!==-2||rec.base.add(0x12b738).readS32()!==-1||
     (read(mem,0x80639578)|0)!==-1){
   ticketPending=false;ticketStage=0;pendingIndex=-1;lastError='Travel cancelled: gameplay changed';return;
  }
''')
source=source.replace('   if(ready&&!cinema)travel.tick();','   travel.tick();')
source=source.replace('  if(ped<0x80000000||ped>=0x94000000||!mailbox)return;',
'''  const inRam=(p,size)=>!(p&3)&&((p>=0x80000000&&p+size<=0x81800000)||(p>=0x90000000&&p+size<=0x94000000));
  if(!inRam(ped,0x16bc)||!inRam(mailbox,0x34))return;''')
confirm_anchor='   if(!action||rec.base.add(0x12bf9c).readU8())return false;'
assert source.count(confirm_anchor)==1
source=source.replace(confirm_anchor,'''   if(!action||!rec.base.add(0x12bf98).readU8()||rec.base.add(0x12bf9c).readU8()||rec.base.add(0x12c094).readU8()||rec.base.add(0x12b734).readS32()!==-2||rec.base.add(0x12b738).readS32()!==-1){lastError='Wait for gameplay and equipment';return false;}''')
# A failure after installing the first hooks must not reinstall them every 100ms.
init_anchor=" if(!engineRunning(ptr(0),ptr(0)))return false;"
assert source.count(init_anchor)==1
source=source.replace('function initialize(){','let recoveryInitializationStarted=false;\nfunction initialize(){')
source=source.replace(init_anchor,init_anchor+'''
 if(recoveryInitializationStarted)return true;
 recoveryInitializationStarted=true;
''')
source = source.replace("File.readAllText(marker)==='r22'", "File.readAllText(marker)==='r31-prompts'")
source = source.replace("File.writeAllText(marker,'r22')", "File.writeAllText(marker,'r31-prompts')")
source=source.replace('installQuestPrompts();installReadableFont();','installQuestPrompts();removeCandidateShirtOverrides();installReadableFont();')
anchor = ' const chemistry=createChemistryControls(main,rec),round=createRound22(main,rec,read,mem);'
assert source.count(anchor) == 1
source = source.replace(anchor, anchor + '\n const review=createRecoveryPlusReview(main,rec,read,mem),sky=createRecoveryPlusSkyRouting(main,rec);')
source = source.replace(anchor, anchor + '\n const transitionFilter=createRecoveryPlusTransitionFilter();')
source = source.replace(anchor, anchor + '\n const shaderReload=createRecoveryPlusShaderReload(main);')
source = source.replace(anchor, anchor + '\n const wristDisplay=applyRecoveryPlusWristDisplay(rec);')
source = source.replace(anchor, anchor + '\n const messageProjection=createRecoveryPlusMessageProjection(rec),tutorialPanel=createRecoveryPlusTutorialPanel(main,rec,read,mem,'+(ROOT/'build/recoveryplus-tutorial-layout.json').read_text()+');\n main.base.add(0xde4f08).writeFloat(1.25);')
source = source.replace(anchor, anchor + '\n const chemistryOverlay=createRecoveryPlusChemistryOverlay(main,rec,mem);')
source = source.replace(anchor, anchor + '\n const streamingGuard=createRecoveryPlusStreamingGuard(rec);')
english_anchor=' const setPanel=createPanelControl(main),draw=createDrawDistanceFloor(main,rec);'
assert source.count(english_anchor)==1
source=source.replace(english_anchor,' applyRecoveryPlusEnglishDefault(rec,root);\n'+english_anchor)
layout_anchor = " const travel=createTravelMenu(rec,main,read,mem),missionAudit=createMissionAudit(rec,read,mem,root);"
assert source.count(layout_anchor)==1
source=source.replace(layout_anchor, ''' try{if(File.readAllText(root+'hud-center-r26.txt')!=='r26')throw Error('old');}
 catch(e){main.base.add(0x1741610).writeFloat(0);main.base.add(0x1741614).writeFloat(.08);saveVR();File.writeAllText(root+'hud-center-r26.txt','r26');}
'''+layout_anchor)
old="const flat=control.panels!==false&&(!ready||cinema||Boolean(rec.base.add(0x12c0e8).readU8())),exact="
new="const flat=transitionFilter.step(control.panels!==false&&(!ready||cinema||Boolean(rec.base.add(0x12c0e8).readU8())),Date.now()),exact="
assert source.count(old) == 1
source = source.replace(old, new)
old = " let control={},lastSequence=null,commandError='',last='',lastFlat=null,cheats=null,testTimer=null;"
new = old + "\n try{control=JSON.parse(File.readAllText(root+'recovery-control.json'));}catch(e){}\n const commandGate=createRecoveryPlusCommandGate(control);"
assert source.count(old) == 1
source = source.replace(old, new)
old = 'if(control.sequence!==undefined&&control.sequence!==lastSequence){'
assert source.count(old) == 1
source = source.replace(old, 'if(commandGate.accept(control)){')
old="   setB(env,nullPtr,4,file,section,exactKey,exact?1:0);"
assert source.count(old) == 1
source = source.replace(old,"   if(Boolean(getB(env,nullPtr,3,file,section,exactKey,0))!==exact)setB(env,nullPtr,4,file,section,exactKey,exact?1:0);")
assert source.count('const state={build,round:') == 1
source = source.replace('const state={build,round:', 'const state={build,review:review.report(),skyRouting:sky.report(),shaderReload:shaderReload.report(),transitions:transitionFilter.report(),round:')
source = source.replace('const state={build,review:', 'const state={build,wristDisplay,review:')
source = source.replace('const state={build,wristDisplay,review:', 'const state={build,wristDisplay,messageProjection:messageProjection.report(),tutorialPanel:tutorialPanel.report(),review:')
source = source.replace('const state={build,wristDisplay,', 'const state={build,chemistryOverlay:chemistryOverlay.report(),wristDisplay,')
source = source.replace('const state={build,chemistryOverlay:', 'const state={build,streamingGuard:streamingGuard.report(),chemistryOverlay:')
patch = (ROOT / 'src/recoveryplus/review.js').read_text(encoding='utf8') + '\n' + (ROOT / 'src/recoveryplus/sky-routing.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/transitions.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/shader-reload.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/commands.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/mission-skip.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/wrist-display.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/instruction-layout.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/tutorial-panel.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/message-projection.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/chemistry-overlay.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/english-default.js').read_text(encoding='utf8')
patch += '\n' + (ROOT / 'src/recoveryplus/streaming-guard.js').read_text(encoding='utf8')
patch += '''
function removeCandidateShirtOverrides(){
 const unlink=new NativeFunction(Module.getGlobalExportByName('unlink'),'int',['pointer']);
 const base='/data/user/0/org.dolphinemu.dolphinemu.quest.bully.recover2/files/Load/Textures/RB7E54/';
 for(const hash of ['d79c0e35a60f2740_6','0f5ae8c61b4425b7_14'])for(const mip of ['','_m'])unlink(Memory.allocUtf8String(base+'tex1_256x256'+mip+'_'+hash+'.png'));
}
'''
out = ROOT / 'build/recoveryplus-review-runtime.js'
out.write_text(patch + '\n' + source, encoding='utf8')
print(f'Staged original Recovery+ + controls/fists review + {len(pack)} prompt textures: {out}')
