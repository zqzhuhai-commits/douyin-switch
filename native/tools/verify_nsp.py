#!/usr/bin/env python3
"""Validate homebrew NSP PFS0 entries, NCA identifiers and encrypted NCA headers.
This is an integrity check, not a verification of Nintendo signatures or a runtime test.
"""
import argparse, hashlib, struct
from pathlib import Path
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
p=argparse.ArgumentParser();p.add_argument('nsp');p.add_argument('--keys',required=True);args=p.parse_args()
keys={}
for line in Path(args.keys).read_text().splitlines():
 if '=' in line:
  name,value=line.split('=',1);keys[name.strip()]=value.strip()
key=bytes.fromhex(keys['header_key'])
b=Path(args.nsp).read_bytes();magic,count,strings_size,_=struct.unpack_from('<4sIII',b)
assert magic==b'PFS0' and count==3
strings_start=16+count*24;data_start=strings_start+strings_size;types=[];title_ids=[]
for i in range(count):
 offset,size,nameoffset,_=struct.unpack_from('<QQII',b,16+i*24)
 name=b[strings_start+nameoffset:b.index(b'\0',strings_start+nameoffset)].decode()
 assert data_start+offset+size<=len(b)
 nca=b[data_start+offset:data_start+offset+size]
 assert hashlib.sha256(nca).hexdigest()[:32]==name.split('.')[0]
 header=b''
 for sector in range(6):
  ctx=Cipher(algorithms.AES(key),modes.XTS(sector.to_bytes(16,'big'))).decryptor()
  header+=ctx.update(nca[sector*512:(sector+1)*512])+ctx.finalize()
 assert header[0x200:0x204]==b'NCA3'
 assert struct.unpack_from('<Q',header,0x208)[0]==size
 tid=struct.unpack_from('<Q',header,0x210)[0];title_ids.append(tid);types.append(header[0x205])
 for section in range(4):
  start,end=struct.unpack_from('<II',header,0x240+section*16)
  if start:
   assert end*512<=len(nca) and start<end
   fs=header[0x400+section*512:0x600+section*512]
   assert hashlib.sha256(fs).digest()==header[0x280+section*32:0x2a0+section*32]
 assert tid==0x0100D0A11A020000
 print('Verified NCA content hash, size, title ID and FS header:',name)
assert sorted(types)==[0,1,2]
print('PASS: program, control and metadata archives validated. Runtime remains untested.')
