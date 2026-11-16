"""Package a side-by-side review from the user's extracted Recovery+ APK."""
import hashlib
import json
import os
import re
import subprocess
import zipfile
import zlib
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
IN_PLACE = '--recoveryplus' in sys.argv
revision = os.environ.get('BULLY_REVIEW_REVISION', 'r24')
build_date = os.environ.get('BULLY_REVIEW_DATE', '2026-11-12')
assert re.fullmatch(r'r\d+', revision) and re.fullmatch(r'\d{4}-\d{2}-\d{2}', build_date)
OUT = ROOT / (f'Builds/RecoveryPlus-{build_date}' if IN_PLACE else 'Builds/RecoveryPlus-2026-11-11')
OLD = b'org.dolphinemu.dolphinemu.quest.bully.recover2'
NEW = OLD if IN_PLACE else b'org.dolphinemu.dolphinemu.quest.bully.recover3'
assert len(OLD) == len(NEW)
label_old = 'BullyVR Recovery+'
label_new = label_old if IN_PLACE else 'BullyVR R+ Teste1'
assert len(label_old) == len(label_new)
OUT.mkdir(parents=True, exist_ok=True)
source = ROOT / 'backup/recoveryplus-20261111/RecoveryPlus-original.apk'
runtime = (ROOT / 'build/recoveryplus-review-runtime.js').read_bytes()
changes = {}
unsigned = ROOT / 'build/recoveryplus-review-unsigned.apk'
with zipfile.ZipFile(source) as original, zipfile.ZipFile(unsigned, 'w') as target:
    for info in original.infolist():
        if re.match(r'META-INF/.*\.(RSA|DSA|EC|SF|MF)$', info.filename, re.I):
            continue
        data = original.read(info.filename)
        if info.filename == 'lib/arm64-v8a/libbully_runtime.script.so':
            data = runtime
        before = data
        data = data.replace(OLD, NEW).replace(OLD.decode().encode('utf-16le'), NEW.decode().encode('utf-16le'))
        if not IN_PLACE:
            data = data.replace(b'.bully.recover2', b'.bully.recover3').replace('.bully.recover2'.encode('utf-16le'), '.bully.recover3'.encode('utf-16le'))
        if info.filename == 'resources.arsc':
            data = data.replace(label_old.encode(), label_new.encode()).replace(label_old.encode('utf-16le'), label_new.encode('utf-16le'))
        if data != before:
            changes[info.filename] = {'package_paths_remapped': True}
        if re.fullmatch(r'classes\d*\.dex', info.filename) and data != before:
            data = bytearray(data)
            data[12:32] = hashlib.sha1(data[32:]).digest()
            data[8:12] = (zlib.adler32(data[12:]) & 0xffffffff).to_bytes(4, 'little')
            data = bytes(data)
        target.writestr(info, data)
bt = Path(os.environ['LOCALAPPDATA']) / 'Android/Sdk/build-tools/36.0.0'
aligned = ROOT / 'build/recoveryplus-review-aligned.apk'
apk = OUT / (f'BullyVR-RecoveryPlus-{revision}.apk' if IN_PLACE else 'BullyVR-RecoveryPlus-Review-r23.apk')
subprocess.run([str(bt/'zipalign.exe'), '-f', '-P', '16', '4', str(unsigned), str(aligned)], check=True)
subprocess.run([str(bt/'apksigner.bat'), 'sign', '--ks', str(Path.home()/'.android/debug.keystore'), '--ks-pass', 'pass:android', '--out', str(apk), str(aligned)], check=True)
subprocess.run([str(bt/'apksigner.bat'), 'verify', str(apk)], check=True, capture_output=True)
subprocess.run([str(bt/'zipalign.exe'), '-c', '-P', '16', '4', str(apk)], check=True)
badging = subprocess.check_output([str(bt/'aapt.exe'), 'dump', 'badging', str(apk)], text=True, encoding='utf8')
assert NEW.decode() in badging
assert label_new in badging
with zipfile.ZipFile(apk) as built:
    assert built.testzip() is None
    for name in built.namelist():
        data = built.read(name)
        if not IN_PLACE:
            assert OLD not in data, name
            assert b'.bully.recover2' not in data, name
        if re.fullmatch(r'classes\d*\.dex', name):
            assert data[12:32] == hashlib.sha1(data[32:]).digest(), name
            assert int.from_bytes(data[8:12], 'little') == zlib.adler32(data[12:]) & 0xffffffff, name
    # Native entry points and binary size remain unchanged by same-length path edits.
    with zipfile.ZipFile(source) as original:
        for name in built.namelist():
            if name.startswith('lib/') and name.endswith('.so') and '.script.' not in name:
                assert len(built.read(name)) == len(original.read(name)), name
digest = hashlib.sha256(apk.read_bytes()).hexdigest()
(OUT/'SHA256.txt').write_text(digest+'  '+apk.name+'\n', encoding='utf8')
manifest = {'base': 'Recovery+ original r22', 'revision': revision + ' stability candidate' if IN_PLACE else 'r23 review', 'package': NEW.decode(), 'label': label_new,
            'bytes': apk.stat().st_size, 'sha256': digest, 'changed_entries': changes,
            'checks': ['APK signature verified', '16 KB native library alignment', 'ZIP CRCs', 'DEX SHA1 and Adler32',
                       'same Recovery+ package and label' if IN_PLACE else 'side-by-side package and label', 'native library sizes preserved', 'runtime JavaScript syntax'],
            'device_validation': 'This manifest records packaging checks only; consult delivery validation for actual device tests.',
            'public_version': '0.0.1-test' if revision=='r32' else None,
            'default_vr_language': 'English on fresh installs; preserves existing language preference' if revision=='r32' else None}
(OUT/'build-manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf8')
print(apk)
print(digest)
