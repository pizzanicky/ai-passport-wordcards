#!/usr/bin/env python3
from pathlib import Path
import json,hashlib,re,subprocess
ROOT=Path(__file__).resolve().parents[1]
rows=json.loads((ROOT/'assets/data/words.json').read_text());audio=(ROOT/'assets/music/words.mp3').read_bytes();manifest=json.loads((ROOT/'assets/music/audio-manifest.json').read_text())
assert 3000<=len(rows)<=4096
assert len({r['id'] for r in rows})==len(rows)
pos=0
for r in rows:
 assert r['word'] and r['ipa'].startswith('/') and r['ipa'].endswith('/') and r['definition']
 assert re.match(r'^(n|v|adj|adv|prep|pron|conj|interj|art|num|aux|abbr)\.',r['definition']),r['word']
 assert r['audio_offset']==pos and r['audio_length']>0
 clip=audio[pos:pos+r['audio_length']];assert len(clip)==r['audio_length'];assert clip[0]==255 and clip[1]&0xe0==0xe0
 assert clip[1]&0x18==0x10 and clip[1]&6==2, 'Expected MPEG-2 Layer III'
 assert [0,8,16,24,32,40,48,56,64,80,96,112,128,144,160][clip[2]>>4]==manifest['bitrate_kbps']
 pos+=r['audio_length']
assert pos==len(audio) and manifest['count']==len(rows)
assert hashlib.sha256(audio).hexdigest()==manifest['sha256']
assert not json.loads((ROOT/'assets/asset-manifest.json').read_text())['missing_glyphs']
subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-xerror','-i',str(ROOT/'assets/music/words.mp3'),'-f','null','-'],check=True)
print(f'Assets: PASS ({len(rows)} words, {len(audio)} audio bytes; editorial/listening audit separate)')

# Read actual firmware tokens and check text contrast after RGB565 quantization.
tokens={n:int(v,16) for n,v in re.findall(r"#define (PAPER|INK|WINE|DARK_TEAL|LIGHT|PINK) 0x([0-9A-F]+)",(ROOT/'main/wc_ui.c').read_text())}
def luminance(value):
 channels=[(((value>>16)&255)>>3)*255/31,(((value>>8)&255)>>2)*255/63,((value&255)>>3)*255/31]
 linear=[(c/255)/12.92 if c/255<=.04045 else ((c/255+.055)/1.055)**2.4 for c in channels]
 return sum(a*b for a,b in zip(linear,[.2126,.7152,.0722]))
for a,b in [('INK','PAPER'),('PAPER','WINE'),('DARK_TEAL','PAPER'),('INK','LIGHT'),('INK','PINK')]:
 l,h=sorted([luminance(tokens[a]),luminance(tokens[b])]);assert (h+.05)/(l+.05)>=4.5,(a,b)
print('RGB565 text contrast: PASS')
