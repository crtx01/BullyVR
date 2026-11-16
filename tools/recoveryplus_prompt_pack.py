"""Stage Recovery+ prompt changes without importing the older Recovery assets.

The baseline chemistry textures are deliberately preserved byte for byte.
Only bindings implemented by src/recoveryplus/review.js are relabelled here.
"""
import base64
import hashlib
import io
import json
import re
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
BASE = ROOT / 'recovered/recoveryplus-original/libbully_runtime.script.so'
OUT = ROOT / 'build/recoveryplus-prompts'


def button(label):
    image = Image.new('RGBA', (256, 256))
    draw = ImageDraw.Draw(image)
    draw.ellipse((8, 8, 248, 248), fill='#eeeede', outline='#30383e', width=7)
    size = 80
    font = ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf', size)
    while draw.textlength(label, font=font) > 218:
        size -= 1
        font = ImageFont.truetype('C:/Windows/Fonts/arialbd.ttf', size)
    draw.text((128, 128), label, font=font, anchor='mm', fill='#161c20')
    buffer = io.BytesIO()
    image.save(buffer, format='PNG')
    return buffer.getvalue()


def main():
    source = BASE.read_text(encoding='utf8')
    match = re.search(r' const pack=(\{[^\n]+\});', source)
    if not match:
        raise ValueError('Original Recovery+ prompt pack missing')
    original = json.loads(match.group(1))
    pack = {name: base64.b64decode(value, validate=True) for name, value in original.items()}
    metadata = json.loads((ROOT / 'recovered/texture-hashes.json').read_text())
    changes = []
    # These glyphs represent the held native focus and grab controls, respectively.
    # Class-specific gesture glyphs have their own hashes and are not remapped.
    labels = {'Wii_C': 'L2', 'Wii_z': 'Y',
              'Wii_pad': 'L2 + R', 'Wii_pad_u_d': 'L2 + R',
              'Wii_pad_l': 'L2 + R <', 'Wii_pad_r': 'L2 + R >',
              'Wii_pad_u': 'L2 + R ^', 'Wii_pad_d': 'L2 + R v'}
    for digest, info in metadata.items():
        if info['name'] not in labels:
            continue
        width, height = info['size']
        for mip in ('', '_m'):
            name = f'tex1_{width}x{height}{mip}_{digest}_{info["format"]}.png'
            if name not in pack:
                raise ValueError(f'Baseline glyph not found: {name}')
            pack[name] = button(labels[info['name']])
            changes.append({'file': name, 'source': info['name'], 'quest': labels[info['name']]})
    OUT.mkdir(parents=True, exist_ok=True)
    for name, data in pack.items():
        Image.open(io.BytesIO(data)).verify()
        (OUT / name).write_bytes(data)
    manifest = {
        'baseline_sha256': hashlib.sha256(BASE.read_bytes()).hexdigest(),
        'texture_count': len(pack),
        'changes': changes,
        'unchanged_count': len(pack) - len(changes),
        'requires': 'Recovery+ review.js focus/grab mapping; not deployed independently',
        'class_validation': 'pending; existing chemistry prompts retained',
    }
    (OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2), encoding='utf8')
    print(f'{len(pack)} valid PNGs; {len(changes)} updated; {manifest["unchanged_count"]} preserved')


if __name__ == '__main__':
    main()
