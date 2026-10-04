#!/usr/bin/env python3
from pathlib import Path
import json,wave,hashlib,struct,math,re
root=Path(__file__).resolve().parents[1]
m=json.loads((root/'assets/music/feedback-manifest.json').read_text());sounds=[]
for name,entry in m['sounds'].items():
 p=root/f'assets/music/feedback-{name}.wav';assert hashlib.sha256(p.read_bytes()).hexdigest()==entry['sha256']
 with wave.open(str(p),'rb') as w:
  assert (w.getframerate(),w.getnchannels(),w.getsampwidth())==(16000,1,2)
  assert w.getnframes()==entry['frames'];raw=w.readframes(w.getnframes())
 samples=struct.unpack('<'+'h'*(len(raw)//2),raw);assert 0<len(samples)<=4000
 assert max(abs(x) for x in samples)<18000 and math.sqrt(sum(x*x for x in samples)/len(samples))>1200
 assert abs(samples[0])<100 and abs(samples[-1])<100
 compiled=(root/"main/wc_feedback.c").read_text()
 encoded=re.search(r"static const int16_t "+name+r"_pcm\[\]=\{([^}]+)\}",compiled).group(1)
 assert tuple(map(int,encoded.split(",")))==samples
 sounds.append(samples)
assert sounds[0]!=sounds[1]
print('Original feedback audio format/length/clipping/endpoints: PASS')
