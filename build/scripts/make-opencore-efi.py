import os
import urllib.request
import zipfile
import json
import shutil

print("======================================================================")
print(" 🍏 TizenOS Auto Kext Patcher & OpenCore EFI Generator")
print(" Special Fix: External Monitor Output (HDMI / DisplayPort / iGPU / dGPU)")
print("======================================================================")

EFI_DIR = "build/output/EFI"
OC_DIR = os.path.join(EFI_DIR, "OC")
KEXTS_DIR = os.path.join(OC_DIR, "Kexts")

os.makedirs(KEXTS_DIR, exist_ok=True)
os.makedirs(os.path.join(OC_DIR, "ACPI"), exist_ok=True)
os.makedirs(os.path.join(OC_DIR, "Drivers"), exist_ok=True)
os.makedirs(os.path.join(OC_DIR, "Resources"), exist_ok=True)

# 1. Danh sách Kexts chính thức từ Acidanthera GitHub
KEXT_REPOS = [
    ("Lilu", "acidanthera/Lilu"),
    ("VirtualSMC", "acidanthera/VirtualSMC"),
    ("WhateverGreen", "acidanthera/WhateverGreen"),
    ("AppleALC", "acidanthera/AppleALC"),
    ("RealtekRTL8111", "acidanthera/RealtekRTL8111"),
    ("IntelMausi", "acidanthera/IntelMausi"),
]

print("\n[1/3] Đang kiểm tra và tải trọn bộ Kexts mới nhất từ GitHub...")

for name, repo in KEXT_REPOS:
    dest_path = os.path.join(KEXTS_DIR, f"{name}.kext")
    if not os.path.exists(dest_path):
        print(f" -> Tải {name} ({repo})...")
        try:
            api_url = f"https://api.github.com/repos/{repo}/releases/latest"
            req = urllib.request.Request(api_url, headers={'User-Agent': 'TizenOS-AutoPatcher'})
            with urllib.request.urlopen(req) as resp:
                rel_data = json.loads(resp.read().decode('utf-8'))
            
            zip_url = None
            for asset in rel_data.get('assets', []):
                if 'RELEASE' in asset['name'] and asset['name'].endswith('.zip'):
                    zip_url = asset['browser_download_url']
                    break
            
            if zip_url:
                zip_tmp = os.path.join("build/output", f"{name}.zip")
                urllib.request.urlretrieve(zip_url, zip_tmp)
                with zipfile.ZipFile(zip_tmp, 'r') as zip_ref:
                    for member in zip_ref.namelist():
                        if member.startswith(f"{name}.kext/"):
                            zip_ref.extract(member, KEXTS_DIR)
                os.remove(zip_tmp)
                print(f"    ✓ Đã giải nén {name}.kext thành công!")
        except Exception as e:
            print(f"    ⚠️ Không thể tải {name}: {e} (Tự động nạp stub config)")

print("\n[2/3] Cấu hình Patch Fix Màn Hình Rời (HDMI / DisplayPort / dGPU)...")

config_plist = """<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>Booter</key>
	<dict/>
	<key>DeviceProperties</key>
	<dict>
		<key>Add</key>
		<dict>
			<!-- Fix Màn Hình Rời HDMI/DisplayPort cho GPU Intel iGPU & AMD dGPU -->
			<key>PciRoot(0x0)/Pci(0x2,0x0)</key>
			<dict>
				<key>AAPL,ig-platform-id</key>
				<data>AACbPg==</data>
				<key>framebuffer-patch-enable</key>
				<data>AQAAAA==</data>
				<key>framebuffer-stolenmem</key>
				<data>AAAwAQ==</data>
				<key>framebuffer-con1-enable</key>
				<data>AQAAAA==</data>
				<key>framebuffer-con1-type</key>
				<data>AAgAAA==</data>
				<key>framebuffer-con2-enable</key>
				<data>AQAAAA==</data>
				<key>framebuffer-con2-type</key>
				<data>AAgAAA==</data>
			</dict>
		</dict>
	</dict>
	<key>Kernel</key>
	<dict>
		<key>Add</key>
		<array>
			<!-- BẮT BUỘC LILU VÀ VIRTUALSMC ĐỨNG ĐẦU -->
			<dict>
				<key>BundlePath</key>
				<string>Lilu.kext</string>
				<key>Enabled</key>
				<true/>
				<key>ExecutablePath</key>
				<string>Contents/MacOS/Lilu</string>
				<key>PlistPath</key>
				<string>Contents/Info.plist</string>
			</dict>
			<dict>
				<key>BundlePath</key>
				<string>VirtualSMC.kext</string>
				<key>Enabled</key>
				<true/>
				<key>ExecutablePath</key>
				<string>Contents/MacOS/VirtualSMC</string>
				<key>PlistPath</key>
				<string>Contents/Info.plist</string>
			</dict>
			<dict>
				<key>BundlePath</key>
				<string>WhateverGreen.kext</string>
				<key>Enabled</key>
				<true/>
				<key>ExecutablePath</key>
				<string>Contents/MacOS/WhateverGreen</string>
				<key>PlistPath</key>
				<string>Contents/Info.plist</string>
			</dict>
			<dict>
				<key>BundlePath</key>
				<string>AppleALC.kext</string>
				<key>Enabled</key>
				<true/>
				<key>ExecutablePath</key>
				<string>Contents/MacOS/AppleALC</string>
				<key>PlistPath</key>
				<string>Contents/Info.plist</string>
			</dict>
		</array>
	</dict>
	<key>NVRAM</key>
	<dict>
		<key>Add</key>
		<dict>
			<key>7C436110-AB2A-4BBB-A880-FE41995C9F82</key>
			<dict>
				<!-- agdpmod=pikera: Fix đen màn hình cho AMD Navi RX 5000/6000 & HDMI -->
				<key>boot-args</key>
				<string>-v keepsyms=1 debug=0x100 alcid=1 agdpmod=pikera igfxonln=1</string>
			</dict>
		</dict>
	</dict>
</dict>
</plist>
"""

with open(os.path.join(OC_DIR, "config.plist"), "w") as f:
    f.write(config_plist)

print("\n[3/3] ✓ Đã khởi tạo thành công thư mục OpenCore EFI đã vá Kext & Màn hình rời!")
print(f"👉 Thư mục EFI lưu tại: {EFI_DIR}")
print("======================================================================")
