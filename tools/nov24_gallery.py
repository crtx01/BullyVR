"""Build a local gallery from real single-eye captures; never invent scenery."""
import json,re,shutil,html,zipfile
from pathlib import Path
root=Path(__file__).resolve().parents[1];out=root/'Gallery/RecoveryPlus-2026-11-24-r40'
(out/'images').mkdir(parents=True,exist_ok=True)
catalog=(root/'src/weapon_catalog.h').read_text()
names=re.findall(r'"([^"]*)"',re.search(r'weaponNames\[\]=\{([^}]+)',catalog)[1])
records=json.loads((root/'evidence/nov24-stability/granted-items.json').read_text())
entries=[]
for r in records:
 if not r.get('equipped') or not r.get('capture'):continue
 name=r['capture'];source=root/'evidence/nov24-stability'/name
 if not source.exists():continue
 shutil.copy2(source,out/'images'/name)
 entries.append({'file':name,'caption':names[r['index']],'source':'Actual Quest eye framebuffer, r39','validation':'Native equipment ID confirmed; firing/hit not visually verified'})
for p in sorted((root/'evidence/nov24-stability').glob('scene-*.png')):
 if p.name in ['scene-controls-panel.png','scene-controls-adjusted.png','scene-real-npc.png']:continue
 shutil.copy2(p,out/'images'/p.name);entries.append({'file':p.name,'caption':p.stem.replace('scene-','').replace('-',' '),'source':'Actual Quest eye framebuffer, r39'})
cards=''.join('<figure><a href="images/'+html.escape(e['file'])+'"><img loading="lazy" src="images/'+html.escape(e['file'])+'"></a><figcaption>'+html.escape(e['caption'])+'</figcaption></figure>' for e in entries)
page='''<!doctype html><html lang="pt-BR"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Bully VR — r39 real gameplay</title><style>body{background:#12171b;color:#f0ead8;font:17px system-ui;margin:0;padding:32px}h1{color:#deb14a}main{display:grid;grid-template-columns:repeat(auto-fit,minmax(280px,1fr));gap:20px}figure{margin:0;background:#20282d;border-radius:10px;overflow:hidden}img{width:100%;display:block}figcaption{padding:14px}p{max-width:900px;line-height:1.5}a{color:#deb14a}</style><h1>Bully VR — Recovery+ r39</h1><p>Capturas reais de um olho no Quest. Sem molduras binoculares, edição com IA ou NPCs adicionados. A galeria documenta a build de teste; equipamento visível não comprova disparo, acerto ou campanha validada. crtx01 · Brasil.</p><main>'''+cards+'</main></html>'
(out/'index.html').write_text(page,encoding='utf8');(out/'provenance.json').write_text(json.dumps(entries,ensure_ascii=False,indent=2),encoding='utf8')
with zipfile.ZipFile(root/'Entrega/BullyVR-r40-gallery.zip','w',zipfile.ZIP_DEFLATED) as z:
 for p in out.rglob('*'):
  if p.is_file():z.write(p,p.relative_to(out))
print(f'{len(entries)} real captures: {out}/index.html')
