#!/usr/bin/env python3
"""Generate original, short offline rating cues; no external audio assets."""
from pathlib import Path
import math, struct, wave, json, hashlib
ROOT=Path(__file__).resolve().parents[1]
RATE=16000
specs={'known':(.24,[(0,1046.5,.105),(.055,1318.5,.12),(.11,1568,.13)]),'again':(.22,[(0,392,.11),(.075,293.66,.145)])}
lines=['#include "wc_feedback.h"'];report={}
for name,(duration,notes) in specs.items():
 samples=[]
 for i in range(round(duration*RATE)):
  t=i/RATE;value=0
  for start,hz,length in notes:
   u=t-start
   if 0<=u<length:
    envelope=min(1,u/.004)*min(1,(length-u)/.012)*math.exp(-u/(.065 if name=='known' else .085))
    value+=envelope*(math.sin(2*math.pi*hz*u)+.16*math.sin(4*math.pi*hz*u))
  samples.append(round(value*9000))
 peak=max(abs(x) for x in samples);assert peak<18000
 data=struct.pack('<'+'h'*len(samples),*samples);path=ROOT/f'assets/music/feedback-{name}.wav'
 with wave.open(str(path),'wb') as w:w.setnchannels(1);w.setsampwidth(2);w.setframerate(RATE);w.writeframes(data)
 lines.append(f'static const int16_t {name}_pcm[]={{'+','.join(map(str,samples))+'};')
 lines.append(f'const wc_feedback_t wc_feedback_{name}={{.pcm={name}_pcm,.count={len(samples)}}};')
 report[name]={'seconds':duration,'frames':len(samples),'peak':peak,'rms':round(math.sqrt(sum(x*x for x in samples)/len(samples)),1),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()}
(ROOT/'main/wc_feedback.c').write_text('\n'.join(lines)+'\n')
(ROOT/'assets/music/feedback-manifest.json').write_text(json.dumps({'sample_rate':RATE,'channels':1,'bits':16,'origin':'original procedural chimes','sounds':report},indent=2)+'\n')
print(json.dumps(report,indent=2))
