#!/usr/bin/env python3
"""Build a traceable local-development corpus. Editorial approval is a separate release gate."""
import csv,json,re,zlib,hashlib
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
IPA={
'promise':'ˈprɒmɪs','responsibility':'rɪˌspɒnsəˈbɪləti','ballpoint':'ˈbɔːlpɔɪnt','barbershop':'ˈbɑːbəʃɒp','bored':'bɔːd','CD':'ˌsiː ˈdiː','cosy':'ˈkəʊzi','crossroads':'ˈkrɒsrəʊdz','customs':'ˈkʌstəmz','download':'ˌdaʊnˈləʊd','Dr':'ˈdɒktə','dvd':'ˌdiː viː ˈdiː','firework':'ˈfaɪəwɜːk','furnished':'ˈfɜːnɪʃt','grandparents':'ˈɡrænpeərənts','mm':'ˈmɪlɪmiːtə','organiser':'ˈɔːɡənaɪzə','puzzled':'ˈpʌzld','refreshments':'rɪˈfreʃmənts','regards':'rɪˈɡɑːdz','repairs':'rɪˈpeəz','skillfully':'ˈskɪlfəli','sunglasses':'ˈsʌnɡlɑːsɪz','theatre':'ˈθɪətə','vcd':'ˌviː siː ˈdiː','walkman':'ˈwɔːkmən','website':'ˈwebsaɪt','westerner':'ˈwestənə','willingness':'ˈwɪlɪŋnəs','workday':'ˈwɜːkdeɪ','workforce':'ˈwɜːkfɔːs','workmate':'ˈwɜːkmeɪt','workplace':'ˈwɜːkpleɪs','works':'wɜːks','wounded':'ˈwuːndɪd'}
CORRECTIONS={'F':'n. 字母 F；华氏温度符号','founding':'n. 建立；创办','internet':'n. 互联网','skipping':'n. 跳绳；跳跃','a':'art. 一个；一','an':'art. 一个；一','the':'art. 这；那（定冠词）','download':'v. 下载\nn. 下载','vcd':'n. 视频光盘','mm':'n. 毫米','Dr':'n. 医生；博士','CD':'n. 光盘','dvd':'n. 数字视频光盘','organiser':'n. 组织者','promise':'n. 承诺；诺言\nv. 承诺；答应','responsibility':'n. 责任；职责'}
rejects=[];rows=[];missing=[]
for r in csv.DictReader(open(ROOT/'.cache/ecdict.csv',encoding='utf-8')):
 if 'gk' not in r['tag'].split():continue
 w=r['word'].strip()
 if w=='cance':rejects.append({'word':w,'reason':'Probable typo / personal name, not a reliable school vocabulary entry'});continue
 raw=r['translation'].replace('\\r\\n','\n').replace('\\n','\n').replace('\\r','\n')
 if w in CORRECTIONS:definition=CORRECTIONS[w]
 else:
  lines=[]
  for ln in raw.splitlines():
   ln=ln.strip()
   if not ln or ln.startswith('['):continue
   ln=re.sub(r'^a\.', 'adj.',ln);ln=re.sub(r'^vt\.|^vi\.', 'v.',ln)
   if not re.match(r'^(n|v|adj|adv|prep|pron|conj|interj|art|num|aux|abbr)\.',ln):
    pos=(r['pos'].split('/')[0].split(':')[0] if r['pos'] else '')
    pos={'a':'adj','d':'adv','j':'adj','p':'prep','c':'conj','u':'pron'}.get(pos,pos)
    if pos not in ['n','v','adj','adv','prep','pron','conj','interj','art','num','aux','abbr']:pos='n' # flagged for editorial review
    ln=pos+'. '+ln
   ln=re.sub(r'\([^)]*\)|（[^）]*）','',ln)
   if ln not in lines:lines.append(ln)
  definition='\n'.join(lines)
  if not definition:
   definition='n. '+re.sub(r'^\[[^]]+\]\s*','',raw.splitlines()[0]);missing.append({'word':w,'reason':'Only specialist definition; requires original replacement'})
 ipa=IPA.get(w,r['phonetic'].strip())
 for a,b in [('ә','ə'),('ɑ:','ɑː'),('ɒ:','ɔː'),('i:','iː'),('u:','uː'),('ə:','ɜː'),('ɜ:','ɜː'),('ei','eɪ'),('ai','aɪ'),('au','aʊ'),('ou','əʊ'),('ɔi','ɔɪ'),('ʃәn','ʃən'),('ʒәn','ʒən'),('ʤ','dʒ'),('ʧ','tʃ'),("'",'ˈ'),(':','ː')]:ipa=ipa.replace(a,b)
 ipa=ipa.replace('[','').replace(']','').strip()
 if not ipa:raise ValueError('Missing IPA: '+w)
 rows.append({'id':zlib.crc32(w.lower().encode()) or 1,'word':w,'ipa':'/'+ipa+'/','definition':definition,'rank':int(r['bnc']) if r['bnc'].isdigit() and int(r['bnc'])>0 else 999999,'reviewed':False,'pronunciation_override':w in IPA})
rows.sort(key=lambda r:(r['rank'],r['word'].lower()));assert len(rows)<=4096
assert len(set(r['id'] for r in rows))==len(rows)
p=ROOT/'assets/data/words.json';p.write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n')
report={'source':'https://github.com/skywind3000/ECDICT','source_sha256':hashlib.sha256((ROOT/'.cache/ecdict.csv').read_bytes()).hexdigest(),'selection':'gk tag; one rejected typo; original-field provenance retained in source cache','count':len(rows),'rejected':rejects,'needs_original_replacement':missing,'reviewed_count':sum(r['reviewed'] for r in rows),'status':'LOCAL_DEVELOPMENT_ONLY_PENDING_EDITORIAL_AND_PROVENANCE_REVIEW','normalization':'Legacy IPA normalized; all pronunciation/definition entries require release review','manual_definition_corrections':list(CORRECTIONS),'manual_ipa_fills':list(IPA)}
(ROOT/'assets/data/corpus-review.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n')
print(json.dumps({'count':len(rows),'rejected':rejects,'specialist_only':len(missing),'max_definition':max(len(r['definition']) for r in rows)},ensure_ascii=False))
