#!/usr/bin/env python3
"""Prepare a three-image installer that never writes the protected identity sector."""
import argparse, hashlib, json, pathlib, shutil
from verify_firmware import parse_partition_table
ROOT=pathlib.Path(__file__).resolve().parents[1]
CARDID_START=0x356000
CARDID_END=0x35a000
FLASH_SIZE=0x800000
AUDIO_OFFSET=CARDID_END
AUDIO_FIRST=0x200000
AUDIO_SPLIT=CARDID_START-AUDIO_FIRST

def check_ranges(images):
    for offset, size in images:
        end=(offset+size+4095)//4096*4096
        if offset<0 or offset%4096 or size<=0 or end>FLASH_SIZE:
            raise ValueError('Invalid flash/erase range')
        if offset<CARDID_END and end>CARDID_START:
            raise ValueError('Image would erase protected cardid')
    ordered=sorted(images)
    for (a,n),(b,_) in zip(ordered,ordered[1:]):
        if (a+n+4095)//4096*4096>b:raise ValueError('Images overlap erase sectors')

def main():
    p=argparse.ArgumentParser();p.add_argument('build_dir',type=pathlib.Path);args=p.parse_args()
    full=args.build_dir/'FoloToy-AI-Passport-full.bin';audio=ROOT/'assets/music/words.mp3'
    check_ranges([(0,full.stat().st_size),(AUDIO_FIRST,AUDIO_SPLIT),(AUDIO_OFFSET,audio.stat().st_size-AUDIO_SPLIT)])
    partitions,_=parse_partition_table((args.build_dir/"partition_table/partition-table.bin").read_bytes())
    identity=[p for p in partitions if p.label=="cardid"]
    resources=[p for p in partitions if p.label=="wordaudio"]
    first=[p for p in partitions if p.label=="audioa"]
    if len(first)!=1 or first[0].offset!=AUDIO_FIRST or first[0].size!=AUDIO_SPLIT:raise ValueError("First audio partition mismatch")
    if len(identity)!=1 or identity[0].offset!=CARDID_START or identity[0].size!=CARDID_END-CARDID_START:raise ValueError("Identity layout changed")
    if len(resources)!=1 or resources[0].offset!=AUDIO_OFFSET or audio.stat().st_size-AUDIO_SPLIT>resources[0].size:raise ValueError("Audio partition mismatch")
    manifest=json.loads((ROOT/'assets/music/audio-manifest.json').read_text())
    if hashlib.sha256(audio.read_bytes()).hexdigest()!=manifest['sha256']:raise ValueError('Audio manifest mismatch')
    dest=ROOT/'build/install';dest.mkdir(parents=True,exist_ok=True)
    shutil.copy2(full,dest/full.name)
    blob=audio.read_bytes();(dest/'audioa.bin').write_bytes(blob[:AUDIO_SPLIT]);(dest/'wordaudio.bin').write_bytes(blob[AUDIO_SPLIT:])
    if (dest/'audioa.bin').read_bytes()+(dest/'wordaudio.bin').read_bytes()!=blob:raise ValueError('Audio split changed content')
    images=[]
    for name,offset in [(full.name,0),('audioa.bin',AUDIO_FIRST),('wordaudio.bin',AUDIO_OFFSET)]:
        blob=(dest/name).read_bytes();images.append({'file':name,'offset':offset,'bytes':len(blob),'sha256':hashlib.sha256(blob).hexdigest()})
    report={'schema':1,'version':(ROOT/'version.txt').read_text().strip(),'target':'esp32c3','flash_bytes':FLASH_SIZE,'protected_identity':{'offset':CARDID_START,'bytes':CARDID_END-CARDID_START},'images':images,'device_tested':False,'platform_single_file_compatible':False}
    (dest/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
    (dest/'README.txt').write_text('DEVELOPMENT INSTALLER: NOT DEVICE VALIDATED\nGet explicit permission before flashing. Replaces the app, partition table and app NVS.\nThree independent image writes preserve cardid; NEVER concatenate the installation images or use erase_flash.\nThe full.bin alone DOES NOT contain vocabulary audio.\nActivate ESP-IDF 5.5.3 and use esptool.py --chip esp32c3 --port YOUR_PORT write_flash 0x0 FoloToy-AI-Passport-full.bin 0x200000 audioa.bin 0x35a000 wordaudio.bin\nDo not claim compatibility with a platform that only installs one full.bin.\n')
    shutil.make_archive(str(ROOT/'build/wordcards-development-install'),'zip',dest)
    print('Protected-identity install package: PASS',dest)
if __name__=='__main__':main()
