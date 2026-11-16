"""Prepare the exact recovered inputs from a user-supplied original Recovery+ APK."""
import argparse, hashlib, zipfile
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
EXPECTED_MAIN='0c779f7939b6790e9e06678d02e7f518987906f3a8bb290a0ae98b575c180262'
EXPECTED_SCRIPT='7951bac83a9bf1d7504b4c3b8ae937462093aeaa38bd4c5405ac7d8e1155d1b7'
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('apk',type=Path)
    args=parser.parse_args()
    data=args.apk.read_bytes()
    with zipfile.ZipFile(args.apk) as apk:
        main=apk.read('lib/arm64-v8a/libmain.so')
        script=apk.read('lib/arm64-v8a/libbully_runtime.script.so')
    if hashlib.sha256(main).hexdigest()!=EXPECTED_MAIN or hashlib.sha256(script).hexdigest()!=EXPECTED_SCRIPT:
        raise SystemExit('Unsupported base: provide the original Recovery+ r22 APK, not a newer hotfix.')
    original=ROOT/'backup/recoveryplus-20261111/RecoveryPlus-original.apk'
    runtime=ROOT/'recovered/recoveryplus-original/libbully_runtime.script.so'
    original.parent.mkdir(parents=True,exist_ok=True);runtime.parent.mkdir(parents=True,exist_ok=True)
    if not original.exists():original.write_bytes(data)
    elif original.read_bytes()!=data:raise SystemExit('Existing base differs; refusing to overwrite it.')
    runtime.write_bytes(script)
    (ROOT/'build').mkdir(exist_ok=True)
    print('Verified original native engine and runtime; staged private build inputs.')
