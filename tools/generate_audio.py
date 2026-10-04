#!/usr/bin/env python3
"""Local UK TTS; no inference API. Cache individual MP3s for reproducible packing."""
from pathlib import Path
import argparse,json,subprocess,wave,time,hashlib
import numpy as np
from piper import PiperVoice
from piper.config import SynthesisConfig
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--limit',type=int,default=0);p.add_argument('--bitrate',type=int,default=16,choices=[16]);args=p.parse_args()
rows=json.loads((ROOT/'assets/data/words.json').read_text());voice=PiperVoice.load(ROOT/'.cache/alba.onnx');cache=ROOT/f'.cache/audio-{args.bitrate}';cache.mkdir(exist_ok=True)
config=SynthesisConfig(length_scale=1.0,noise_scale=0.4,noise_w_scale=0.5)
# Explicit readings for abbreviations; alternate stress remains an editorial release check.
overrides={'Dr':'doctor','mm':'millimetre','CD':'C D','dvd':'D V D','vcd':'V C D'}
start=time.monotonic();entries=[]
for i,r in enumerate(rows[:args.limit or None]):
 path=cache/f'{r["id"]:08x}.mp3'
 if not path.exists():
  chunks=list(voice.synthesize(overrides.get(r['word'],r['word']),config));pcm=np.concatenate([c.audio_int16_array for c in chunks])
  # Trim only outer silence and keep 50 ms around audible content.
  audible=np.where(np.abs(pcm.astype(np.int32))>160)[0];rate=voice.config.sample_rate
  if not len(audible):raise RuntimeError('Silent synthesis '+r['word'])
  pad=int(rate*.05);pcm=pcm[max(0,audible[0]-pad):min(len(pcm),audible[-1]+pad)]
  out=subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','s16le','-ar',str(rate),'-ac','1','-i','pipe:0','-ar','16000','-ac','1','-codec:a','libmp3lame','-b:a',f'{args.bitrate}k','-write_xing','0','-id3v2_version','0','-f','mp3','pipe:1'],input=pcm.tobytes(),capture_output=True,check=True)
  path.write_bytes(out.stdout)
 entries.append({'id':r['id'],'word':r['word'],'bytes':path.stat().st_size})
 if (i+1)%100==0:print(f'{i+1}/{args.limit or len(rows)} generated; {time.monotonic()-start:.1f}s',flush=True)
if not args.limit:
 blob=bytearray()
 for r in rows:
  data=(cache/f'{r["id"]:08x}.mp3').read_bytes();r['audio_offset']=len(blob);r['audio_length']=len(data);blob.extend(data)
 (ROOT/'assets/music/words.mp3').write_bytes(blob);(ROOT/'assets/data/words.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n')
 report={'engine':'piper-tts 1.8.0','voice':'en_GB-alba-medium','model_sha256':hashlib.sha256((ROOT/'.cache/alba.onnx').read_bytes()).hexdigest(),'bitrate_kbps':args.bitrate,'sample_rate':16000,'channels':1,'count':len(rows),'bytes':len(blob),'sha256':hashlib.sha256(blob).hexdigest(),'listening_status':'NOT_AUDITED','overrides':overrides}
 (ROOT/'assets/music/audio-manifest.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report),flush=True)
else:print(json.dumps({'sample_count':len(entries),'sample_bytes':sum(e['bytes'] for e in entries),'estimated_full_bytes':sum(e['bytes'] for e in entries)*len(rows)//len(entries),'elapsed':time.monotonic()-start}),flush=True)
