from pathlib import Path
import csv,re,json
source=Path(__file__).resolve().parents[1]/'reference/PAD_NET_MAP.csv'
# Official ESP32-S3-WROOM-1 module-pad -> GPIO mapping, datasheet v1.8 Table 3-1.
pad_gpio={4:4,5:5,6:6,7:7,8:15,9:16,10:17,11:18,12:8,13:19,14:20,15:3,16:46,17:9,18:10,19:11,20:12,21:13,22:14,23:21,24:47,25:48,26:45,27:0,28:35,29:36,30:37,31:38,32:39,33:40,34:41,35:42,36:44,37:43,38:2,39:1}
net_const={'CAM_D0':'camD0','CAM_D1':'camD1','CAM_D2':'camD2','CAM_D3':'camD3','CAM_D4':'camD4','CAM_D5':'camD5','CAM_D6':'camD6','CAM_D7':'camD7','CAM_XCLK_DRV':'camXclk','CAM_PCLK':'camPclk','CAM_VSYNC':'camVsync','CAM_HREF':'camHref','I2C_SDA':'camSda','I2C_SCL':'camScl','LCD_SCK_DRV':'lcdSck','LCD_MOSI_DRV':'lcdMosi','LCD_CS_N':'lcdCs','LCD_DC':'lcdDc','LCD_BL_CMD':'lcdBl','NFC_SCK_DRV':'nfcSck','NFC_MOSI_DRV':'nfcMosi','NFC_MISO':'nfcMiso','NFC_CS_N':'nfcCs','DOOR_SENSE_N':'door','REX_SENSE_N':'rex','QR_SENSE_N':'qrButton','TAMPER_SENSE_N':'tamper','LED_R_CMD':'ledR','LED_G_CMD':'ledG','LED_B_CMD':'ledB','BUZZ_CMD':'buzzer','LOCK_CMD':'lock'}
header=(Path(__file__).resolve().parents[1]/'include/BoardR1.h').read_text();constants={k:int(v) for k,v in re.findall(r'(\w+)\s*=\s*(-?\d+)',header)}
rows=[]
for r in csv.DictReader(source.open(encoding='utf-8-sig')):
 if r['Reference']=='U1' and r['Net'] in net_const:
  key=net_const[r['Net']];gpio=pad_gpio[int(r['Pad_number'])];assert constants[key]==gpio,(key,gpio,constants[key]);rows.append({'net':r['Net'],'module_pad':int(r['Pad_number']),'gpio':gpio,'constant':key})
assert len(rows)==len(net_const),(len(rows),len(net_const))
assert len({r['gpio'] for r in rows})==len(rows)
assert not {19,20}.intersection({r['gpio'] for r in rows})
print(json.dumps({'pinmap_checks':'PASS','signals':len(rows),'no_gpio_collisions':True,'usb_gpio19_20_reserved':True,'mapping':rows},ensure_ascii=False,indent=2))
