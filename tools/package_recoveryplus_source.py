"""Export an explicit allowlist, never the workspace or device backups."""
import hashlib,json,re,shutil,subprocess,zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'Entrega/BullyVR-r34-source'
OUT.mkdir(parents=True,exist_ok=True)
def copy(rel):
    path=OUT/rel;path.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/rel,path)
for p in (ROOT/'src').rglob('*'):
    if p.is_file() and p.suffix in ('.h','.cpp','.js','.txt'):copy(p.relative_to(ROOT))
for p in (ROOT/'tests').iterdir():
    if p.is_file() and p.suffix in ('.py','.cpp','.md','.txt'):copy(p.relative_to(ROOT))
names=['recoveryplus_stage_review.py','recoveryplus_prompt_pack.py','build_recoveryplus_review.py',
       'build_recoveryplus_mission_skip.py','build_recoveryplus_tutorial_layout.py',
       'prepare_recoveryplus_source.py','package_recoveryplus_source.py','build_contact_bridge.py']
names += [p.name for p in (ROOT/'tools').glob('check_recoveryplus_*') if p.suffix in ('.js','.py')]
for name in names:copy(Path('tools')/name)
for name in ['Documentation/recoveryplus/mission-catalog.json','recovered/texture-hashes.json']:
    copy(Path(name))
for name in ['README.md','BUILD.md','STABILITY.md','SOURCE-PROVENANCE.md','.gitignore','requirements-dev.txt']:
    copy(Path('Documentation/source-release')/name)
    shutil.copy2(ROOT/'Documentation/source-release'/name,OUT/name)
controls=ROOT/'CONTROLS.md'
if not controls.exists():controls=ROOT/'Entrega/BullyVR-0.0.1-hf1-Kit/INSTRUCOES/CONTROLS.md'
shutil.copy2(controls,OUT/'CONTROLS.md')
# Include readable base logic, but not embedded texture/font payloads. The build
# helper obtains exact inputs from the owner's original APK and verifies hashes.
original=ROOT/'recovered/recoveryplus-original/libbully_runtime.script.so'
if original.exists():
    base=original.read_text(encoding='utf8')
    base,n=re.subn(r' const pack=\{[^\n]+\};',' const pack={}; // Embedded textures omitted; see BUILD.md.',base,count=1)
    assert n==1
    base,n=re.subn(r' const s="[A-Za-z0-9+/=]+"', ' const s="" /* Embedded font omitted; see BUILD.md. */',base,count=1)
    assert n==1
else:base=(ROOT/'reference/recoveryplus-r22-runtime-readable.js').read_text(encoding='utf8')
(OUT/'reference').mkdir(exist_ok=True)
(OUT/'reference/recoveryplus-r22-runtime-readable.js').write_text(base,encoding='utf8')
copy(Path('upstream/COPYING'))
for p in (ROOT/'upstream/LICENSES').glob('*'):
    if p.is_file():copy(p.relative_to(ROOT))
if (ROOT/'upstream/.git').exists():
    patch=subprocess.check_output(['git','-C',str(ROOT/'upstream'),'diff','--','Source/Core/VideoCommon/VideoConfig.cpp'])
else:patch=(ROOT/'reference/upstream-local-changes.patch').read_bytes()
(OUT/'reference/upstream-local-changes.patch').write_bytes(patch)
(OUT/'.gitattributes').write_bytes(b'* text=auto eol=lf\n')
for p in OUT.rglob('*'):
    if p.is_file():p.write_bytes(p.read_bytes().replace(b'\r\n',b'\n'))
manifest={str(p.relative_to(OUT)).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest()
          for p in OUT.rglob('*') if p.is_file() and p.name!='SOURCE-SHA256.json'}
(OUT/'SOURCE-SHA256.json').write_bytes(json.dumps(manifest,indent=2).encode('utf8'))
archive=ROOT/'Entrega/BullyVR-r34-source.zip'
with zipfile.ZipFile(archive,'w',zipfile.ZIP_DEFLATED) as z:
    for p in sorted(OUT.rglob('*')):
        if p.is_file():z.write(p,p.relative_to(OUT))
with zipfile.ZipFile(archive) as z:assert z.testzip() is None
print(f'{archive}: {len(manifest)} source/documentation files')
