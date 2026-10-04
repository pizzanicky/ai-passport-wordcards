#!/usr/bin/env python3
"""Compile local corpus, approved artwork and open-font subsets for LVGL."""
from pathlib import Path
import json,struct,zlib,subprocess,hashlib,shutil,os
from PIL import Image
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
ROOT=Path(__file__).resolve().parents[1];rows=json.loads((ROOT/'assets/data/words.json').read_text())
corpus=zlib.crc32(json.dumps([(r['id'],r['word'],r['ipa'],r['definition']) for r in rows],ensure_ascii=False).encode())
def cs(t):return json.dumps(t,ensure_ascii=False)
(ROOT/'main/wc_data.h').write_text('#pragma once\n#include <stdint.h>\n#define WC_WORD_COUNT '+str(len(rows))+'\n#define WC_CORPUS_ID '+str(corpus)+'u\ntypedef struct {const char *word,*ipa,*definition;uint32_t id,audio_offset,audio_length;} wc_word_t;\nextern const wc_word_t wc_words[WC_WORD_COUNT];\nextern const uint32_t wc_word_ids[WC_WORD_COUNT];\n')
lines=['#include "wc_data.h"','const wc_word_t wc_words[WC_WORD_COUNT]={']
for r in rows:
 if not r.get('audio_length'):raise RuntimeError('Generate full audio before packing: '+r['word'])
 lines.append('{'+','.join([cs(r['word']),cs(r['ipa']),cs(r['definition']),str(r['id'])+'u',str(r['audio_offset'])+'u',str(r['audio_length'])+'u'])+'},')
lines+=['};','const uint32_t wc_word_ids[WC_WORD_COUNT]={'+','.join(str(r['id'])+'u' for r in rows)+'};']
(ROOT/'main/wc_data.c').write_text('\n'.join(lines)+'\n')
# Assets are rendered into fixed-size RGB565 planes; no full-screen framebuffer.
asset=Image.open(ROOT/'assets/images/character-source.png').convert('RGBA');defs=['#include "lvgl.h"'];image_report={}
for name,size in [('home',(92,117)),('front',(46,55)),('finish',(67,82))]:
 plane=Image.new('RGB',size,'#FBF5EB');thumb=asset.copy();thumb.thumbnail(size,Image.Resampling.LANCZOS);plane.paste(thumb,((size[0]-thumb.width)//2,(size[1]-thumb.height)//2),thumb)
 if name=='home':
  from PIL import ImageDraw
  ImageDraw.Draw(plane).rounded_rectangle((3,3,86,111),radius=39,fill='#343654',outline='#D4BEA1',width=2);plane.paste(thumb,((size[0]-thumb.width)//2,(size[1]-thumb.height)//2),thumb)
 blob=b''.join(struct.pack('<H',((r>>3)<<11)|((g>>2)<<5)|(b>>3)) for r,g,b in plane.getdata());symbol='wc_portrait_'+name
 defs.append('static const uint8_t '+symbol+'_data[]={'+','.join(str(v) for v in blob)+'};')
 defs.append('const lv_image_dsc_t '+symbol+'={.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_RGB565,.w='+str(size[0])+',.h='+str(size[1])+',.stride='+str(size[0]*2)+'},.data_size='+str(len(blob))+',.data='+symbol+'_data};')
 image_report[name]={'size':size,'bytes':len(blob)}
(ROOT/'assets/images/wc_portraits.c').write_text('\n'.join(defs)+'\n')
UI='随身词卡第轮词新开始继续学习进度设置发音先回忆它的意思看释义认识再练长按返回首页翻页评价本完成下见已接触熟悉待复习库总数依据你自记录全部功能离线使用增大减小保存并将清除和保持可用版本英式与需要处理操作未后重试正在…取消确认正在保存存储不可用进度未保存重试音频不可用词库待发布审核迁移失败请确认重置按钮初始化失败声设备检查内部错误未完成还原电量--0123456789 /%:›↑↓OKnvadjprepronartauxconjinterjnumabbr.；，。（）'
UI+=''.join((ROOT/'main/wc_ui.c').read_text()+(ROOT/'main/wc_app.c').read_text())
UI+='·'
UI=''.join(c for c in UI if ord(c)<128 or '\u4e00'<=c<='\u9fff' or c in '·…›↑↓；，。（）')
latin=''.join(r['word']+r['ipa'] for r in rows)+UI
word_chars=''.join(r['word'] for r in rows)
ASCII=''.join(chr(i) for i in range(32,127))
chinese=''.join(r['definition'] for r in rows)+UI
for source,out in [('NotoSansSC.ttf','NotoSansSC-Regular.ttf'),('NotoSans.ttf','NotoSans-Regular.ttf')]:
 p=ROOT/'.cache'/out
 if not p.exists():
  font=TTFont(ROOT/'.cache'/source);axes={a.axisTag:(400 if a.axisTag=='wght' else 100) for a in font['fvar'].axes};instantiateVariableFont(font,axes,inplace=True).save(p)
converter=os.environ.get('LV_FONT_CONV',str(ROOT/'tools/node_modules/lv_font_conv/lv_font_conv.js'));node=shutil.which('node')
if not node or not Path(converter).exists():raise RuntimeError('Install Node.js and run npm ci --prefix tools before generating fonts')
missing={}
for size in [16,18,20,22,24,26,30]:
 title_chars={22:'认识再练0123456789',24:'本轮完成学习进度设置重置记录版本与词库需要处理随身卡发声音量0123456789',26:'随身词卡0123456789词',30:'0123456789%'}
 cnchars=''.join(sorted(set(chinese if size==20 else title_chars.get(size,UI))));enchars=''.join(sorted(set(latin if size==16 else word_chars+ASCII+title_chars.get(size,UI))))
 cnmap=TTFont(ROOT/'.cache/NotoSansSC-Regular.ttf').getBestCmap();enmap=TTFont(ROOT/'.cache/NotoSans-Regular.ttf').getBestCmap()
 bad=[c for c in set(cnchars+enchars) if ord(c) not in cnmap and ord(c) not in enmap and not c.isspace()]
 if bad:missing[str(size)]=bad
 cnchars=''.join(c for c in cnchars if ord(c) in cnmap)
 enchars=''.join(c for c in enchars if ord(c) in enmap)
 cmd=[node,converter,'--font',str(ROOT/'.cache/NotoSansSC-Regular.ttf'),'--symbols',cnchars,'--font',str(ROOT/'.cache/NotoSans-Regular.ttf'),'--symbols',enchars,'--size',str(size),'--bpp','2' if size==20 else '4','--format','lvgl','--lv-include','lvgl.h','--lv-font-name',f'wc_font_{size}','-o',str(ROOT/f'assets/fonts/wc_font_{size}.c')]
 if size!=20:cmd.append("--no-compress")
 subprocess.run(cmd,check=True,capture_output=True)
 output=ROOT/f'assets/fonts/wc_font_{size}.c'
 output.write_text('\n'.join(line for line in output.read_text().splitlines() if not line.strip().startswith('* Opts:'))+'\n')
if missing:raise RuntimeError(json.dumps(missing,ensure_ascii=False))
(ROOT/'assets/asset-manifest.json').write_text(json.dumps({'corpus_id':corpus,'count':len(rows),'images':image_report,'font_sizes':[16,18,20,22,24,26,30],'missing_glyphs':missing,'character_sha256':hashlib.sha256((ROOT/'assets/images/character-source.png').read_bytes()).hexdigest()},ensure_ascii=False,indent=2)+'\n')
print('Compiled',len(rows),'words, images, seven font sizes.')

manifest=json.loads((ROOT/"assets/music/audio-manifest.json").read_text())
(ROOT/"main/wc_audio_asset.h").write_text("#pragma once\n#include <stdint.h>\n#define WC_AUDIO_BYTES "+str(manifest["bytes"])+"u\nstatic const uint8_t wc_audio_sha256[32]={"+",".join("0x"+manifest["sha256"][i:i+2] for i in range(0,64,2))+"};\n")
