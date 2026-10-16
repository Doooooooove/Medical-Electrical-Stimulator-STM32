# -*- coding: utf-8 -*-
import os
import struct
import hashlib
import subprocess
import sys

BASE_DIR = r"C:\Users\24894\Desktop\ESP32C3_Wireless"
TEMP_DIR = r"C:\Users\24894\Desktop\_temp"
LUAC_EXE = os.path.join(TEMP_DIR, r"tools\luac_536.exe")
MAIN_LUA = os.path.join(BASE_DIR, "main.lua")
MAIN_LUAC = os.path.join(BASE_DIR, "main.luac")
DISK_FS = os.path.join(TEMP_DIR, r"soc_disk\disk.fs")

LUA_CONTENT = r'''PROJECT = "medical_stim"
VERSION = "2.1.0"

sys = require("sys")
require("sysplus")

local uartid = 0
uart.setup(uartid, 9600, 8, 1)

local last_status = "STM32已连接，就绪"
local cur_patient = "未选择"

uart.on(uartid, "receive", function(id, len)
    local s = uart.read(id, len)
    if s and #s > 0 then
        last_status = s
    end
end)

sys.taskInit(function()
    sys.wait(1000)
    wlan.init()
    sys.wait(500)
    wlan.createAP("Medical-Stim", "12341234")

    sys.wait(1000)
    httpsrv.start(80, function(fd, method, uri, headers, body)
        if uri:find("id=1") or uri:find("plan=1") then
            cur_patient = "01·张海波(吞咽仿生)"
            uart.write(uartid, "#1\r\n")
            return 200, {["Content-Type"]="text/plain;charset=utf-8"}, "P1 OK"
        elseif uri:find("id=2") or uri:find("plan=2") then
            cur_patient = "02·李秀华(肌力强化)"
            uart.write(uartid, "#2\r\n")
            return 200, {["Content-Type"]="text/plain;charset=utf-8"}, "P2 OK"
        elseif uri:find("id=3") or uri:find("plan=3") then
            cur_patient = "03·王德全(慢性镇痛)"
            uart.write(uartid, "#3\r\n")
            return 200, {["Content-Type"]="text/plain;charset=utf-8"}, "P3 OK"
        elseif uri:find("act=up") then
            uart.write(uartid, "#+\r\n")
            return 200, {["Content-Type"]="text/plain;charset=utf-8"}, "UP OK"
        elseif uri:find("act=down") then
            uart.write(uartid, "#-\r\n")
            return 200, {["Content-Type"]="text/plain;charset=utf-8"}, "DN OK"
        elseif uri:find("act=toggle") then
            uart.write(uartid, "#S\r\n")
            return 200, {["Content-Type"]="text/plain;charset=utf-8"}, "TG OK"
        elseif uri:find("act=stop") then
            cur_patient = "已急停归零"
            uart.write(uartid, "#0\r\n")
            return 200, {["Content-Type"]="text/plain;charset=utf-8"}, "EM OK"
        elseif uri:find("/status") then
            uart.write(uartid, "#R\r\n")
            local clean_s = last_status:gsub('"', ''):gsub('\r', ''):gsub('\n', '')
            local resp = string.format('{"p":"%s","s":"%s"}', cur_patient, clean_s)
            return 200, {["Content-Type"]="application/json;charset=utf-8"}, resp
        end

        local html = [[<!DOCTYPE html><html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1,user-scalable=no"><title>医疗刺激仪无线管理终端</title><style>*{box-sizing:border-box;margin:0;padding:0}body{font-family:sans-serif;background:#f0f2f5;padding:12px}h2{color:#096dd9;text-align:center;margin:8px 0;font-size:18px}.c{background:#fff;border-radius:10px;padding:12px;margin-bottom:10px;box-shadow:0 1px 4px rgba(0,0,0,.1)}.t{font-size:13px;font-weight:bold;color:#333;margin-bottom:8px}.st{background:#e6f7ff;border:1px solid #91d5ff;border-radius:6px;padding:8px;font-size:13px;color:#0050b3;font-weight:600;min-height:36px}.pb{width:100%;padding:10px;margin-bottom:8px;border:1.5px solid #1890ff;border-radius:8px;background:#f0f8ff;text-align:left;cursor:pointer}.pb:active{background:#bae7ff}.pn{font-size:14px;font-weight:bold;color:#096dd9}.pd{font-size:11px;color:#666;margin-top:2px}.g{display:grid;grid-template-columns:1fr 1fr;gap:8px}.b{padding:12px 6px;font-size:14px;border:none;border-radius:8px;color:#fff;font-weight:bold;cursor:pointer}.bu{background:#52c41a}.bd{background:#faad14}.bt{background:#1890ff}.bs{background:#ff4d4f;grid-column:span 2;padding:14px;font-size:16px}</style></head><body><h2>🩺 医疗刺激仪无线终端</h2><div class="c"><div class="t">📊 实时遥测状态</div><div class="st" id="st">读取中...</div></div><div class="c"><div class="t">👥 患者病例管理库 (一键下发执行)</div><button class="pb" onclick="s('/plan?id=1')"><div class="pn">👤 患者01 · 张海波 (吞咽障碍)</div><div class="pd">处方: 仿生指数波 20Hz 5mA 20分钟</div></button><button class="pb" onclick="s('/plan?id=2')"><div class="pn">👤 患者02 · 李秀华 (肌力衰减)</div><div class="pd">处方: 双向对称方波 20Hz 6mA 20分钟</div></button><button class="pb" onclick="s('/plan?id=3')"><div class="pn">👤 患者03 · 王德全 (慢性镇痛)</div><div class="pd">处方: 直流恒流模式 5mA 10分钟</div></button></div><div class="c"><div class="t">⚡ 临床执行与急停控制</div><div class="g"><button class="b bu" onclick="s('/cmd?act=up')">▲ 电流 +1mA</button><button class="b bd" onclick="s('/cmd?act=down')">▼ 电流 -1mA</button><button class="b bt" onclick="s('/cmd?act=toggle')">⏱️ 启动/暂停</button><button class="b bs" onclick="s('/cmd?act=stop')">🛑 紧急断电急停</button></div></div><script>function s(u){fetch(u).then(()=>setTimeout(p,150));}function p(){fetch('/status').then(r=>r.json()).then(d=>{document.getElementById('st').innerHTML='患者: '+d.p+'<br>状态: '+d.s;}).catch(()=>{});}setInterval(p,1500);p();</script></body></html>]]
        return 200, {["Content-Type"]="text/html;charset=utf-8"}, html
    end)
end)

sys.run()
'''

def make_record(tags, payload=b''):
    rec = b'\x01\x04\x5a\xa5\x5a\xa5'
    for tag, val in tags:
        rec += bytes([tag, len(val)]) + val
    rec += b'\xfe\x02'
    chk = sum(rec) & 0xFFFF
    rec += struct.pack('<H', chk)
    return rec + payload

def build():
    print("[1/3] 写入最新轻量 main.lua...")
    with open(MAIN_LUA, "w", encoding="utf-8") as f:
        f.write(LUA_CONTENT)

    print("[2/3] 编译为 main.luac...")
    cmd = [LUAC_EXE, "-s", "-o", MAIN_LUAC, MAIN_LUA]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("编译失败:", res.stderr)
        return False
    with open(MAIN_LUAC, "rb") as f:
        luac_bytes = f.read()
    print(f"main.luac 大小: {len(luac_bytes)} 字节")

    print("[3/3] 打包成 LuatOS luadb disk.fs...")
    # header record: version 2, headsize 24, filecount 2
    hdr = make_record([(2, struct.pack('<H', 2)), (3, struct.pack('<I', 24)), (4, struct.pack('<H', 2))])
    # file 1 record: main.luac
    rec1 = make_record([(2, b'main.luac'), (3, struct.pack('<I', len(luac_bytes)))], luac_bytes)
    # file 2 record: .airm2m_all_crc#.bin
    all_data_for_md5 = hdr + rec1
    crc_rec_hdr = b'\x01\x04\x5a\xa5\x5a\xa5\x02\x14.airm2m_all_crc#.bin\x03\x04\x10\x00\x00\x00\xfe\x02'
    chk = sum(crc_rec_hdr) & 0xFFFF
    crc_rec_hdr += struct.pack('<H', chk)
    md5_val = hashlib.md5(all_data_for_md5 + crc_rec_hdr).digest()
    rec2 = crc_rec_hdr + md5_val

    disk_data = hdr + rec1 + rec2
    # pad to 12KB (12288 bytes)
    if len(disk_data) < 12288:
        disk_data += b'\x00' * (12288 - len(disk_data))

    os.makedirs(os.path.dirname(DISK_FS), exist_ok=True)
    with open(DISK_FS, "wb") as f:
        f.write(disk_data)
    print(f"成功生成 disk.fs: {len(disk_data)} 字节 -> {DISK_FS}")
    return True

if __name__ == "__main__":
    if build():
        print("Build SUCCESSFUL!")
    else:
        sys.exit(1)
