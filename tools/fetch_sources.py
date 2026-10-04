#!/usr/bin/env python3
"""Fetch public build inputs; never sends user content or credentials."""
from pathlib import Path
import concurrent.futures, requests, hashlib, json
ROOT=Path(__file__).resolve().parents[1]
BASE='https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_GB/alba/medium/'
URLS={
 '.cache/alba.onnx':BASE+'en_GB-alba-medium.onnx',
 '.cache/alba.onnx.json':BASE+'en_GB-alba-medium.onnx.json',
 'assets/music/alba-MODEL_CARD':BASE+'MODEL_CARD',
 'assets/data/ECDICT-LICENSE':'https://raw.githubusercontent.com/skywind3000/ECDICT/master/LICENSE',
 '.cache/NotoSansSC.ttf':'https://raw.githubusercontent.com/google/fonts/main/ofl/notosanssc/NotoSansSC%5Bwght%5D.ttf',
 'assets/fonts/NotoSansSC-OFL.txt':'https://raw.githubusercontent.com/google/fonts/main/ofl/notosanssc/OFL.txt',
 '.cache/NotoSans.ttf':'https://raw.githubusercontent.com/google/fonts/main/ofl/notosans/NotoSans%5Bwdth,wght%5D.ttf',
 'assets/fonts/NotoSans-OFL.txt':'https://raw.githubusercontent.com/google/fonts/main/ofl/notosans/OFL.txt',
}
def fetch(item):
 name,url=item;p=ROOT/name;p.parent.mkdir(parents=True,exist_ok=True)
 if not p.exists():
  r=requests.get(url,timeout=180);r.raise_for_status();p.write_bytes(r.content)
 return name,{'url':url,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'size':p.stat().st_size}
with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
 records=dict(pool.map(fetch,URLS.items()))
(ROOT/'.cache/sources.json').write_text(json.dumps(records,indent=2))
print(json.dumps({k:v['size'] for k,v in records.items()},indent=2))
