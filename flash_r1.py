#!/usr/bin/env python3
"""Flash a merged R1 image only when the user explicitly supplies a serial port."""
import argparse,hashlib,json,subprocess,sys
from pathlib import Path
p=argparse.ArgumentParser(description='GATEPASS original R1 ESP32-S3-N8R2 flash utility')
p.add_argument('--port',required=True,help='Actual serial port, e.g. /dev/cu.usbmodemXXXX')
p.add_argument('--mode',choices=['bench','control'],default='bench')
p.add_argument('--baud',type=int,default=460800)
a=p.parse_args();root=Path(__file__).resolve().parent
name='GATEPASS_R1_1.0.0_'+a.mode.upper()+'_FULL.bin';binary=root/'bin'/name
manifest=json.loads((root/'MANIFEST.json').read_text())
if hashlib.sha256(binary.read_bytes()).hexdigest()!=manifest['sha256']['bin/'+name]:raise SystemExit('Image SHA-256 mismatch; refusing to flash.')
local_tool=Path.home()/'.platformio/packages/tool-esptoolpy/esptool.py'
pio_python=Path.home()/'.platformio/penv/bin/python'
command=[str(pio_python),str(local_tool)] if local_tool.exists() and pio_python.exists() else [sys.executable,'-m','esptool']
print('Flashing original R1 / ESP32-S3 / N8R2:',a.mode,'offset 0x0')
subprocess.run(command+['--chip','esp32s3','--port',a.port,'--baud',str(a.baud),'write_flash','--flash_mode','dio','0x0',str(binary)],check=True)
