import os
import shutil

rootfs = '/var/tmp/tizenos_rootfs'
debian_branding = os.path.join(rootfs, 'etc/calamares/branding/debian')
tizen_branding = os.path.join(rootfs, 'etc/calamares/branding/tizenos')

# 1. Copy Debian branding base to TizenOS branding
os.makedirs(tizen_branding, exist_ok=True)
if os.path.exists(debian_branding):
    for item in os.listdir(debian_branding):
        s = os.path.join(debian_branding, item)
        d = os.path.join(tizen_branding, item)
        if os.path.isdir(s):
            shutil.copytree(s, d, dirs_exist_ok=True)
        else:
            shutil.copy2(s, d)

# 2. Update branding.desc with TizenOS details
branding_desc = """---
componentName:   tizenos
welcomeStyleCalamares: true
welcomeExpandingLogo: true
windowExpanding: normal
windowSize: 800px,520px
windowPlacement: center

strings:
    productName:         TizenOS
    shortProductName:    TizenOS
    version:             1.0 (Debian Edition)
    shortVersion:        1.0
    versionedName:       TizenOS 1.0 (Debian Edition)
    shortVersionedName:  TizenOS 1.0
    bootloaderEntryName: TizenOS
    productUrl:          https://tizenos.org
    supportUrl:          https://tizenos.org/support
    knownIssuesUrl:      https://tizenos.org/issues
    releaseNotesUrl:     https://tizenos.org/notes

images:
    productLogo:         "/usr/share/plymouth/themes/spinner/watermark.png"
    productIcon:         "/usr/share/icons/hicolor/48x48/apps/system-software-install.png"
    productWelcome:      "welcome.png"

slideshow:               "show.qml"

style:
   sidebarBackground:    "#1a1b26"
   sidebarText:          "#FFFFFF"
   sidebarTextSelect:    "#41a6b5"
   sidebarTextHighlight: "#7aa2f7"

slideshowAPI: 2
"""

with open(os.path.join(tizen_branding, 'branding.desc'), 'w') as f:
    f.write(branding_desc)

# 3. Update settings.conf to point to tizenos branding
settings_path = os.path.join(rootfs, 'etc/calamares/settings.conf')
if os.path.exists(settings_path):
    with open(settings_path, 'r') as f:
        content = f.read()
    content = content.replace('branding: debian', 'branding: tizenos')
    with open(settings_path, 'w') as f:
        f.write(content)

# 4. Fix sudo: unable to resolve host TizenOS in /etc/hosts
hosts_path = os.path.join(rootfs, 'etc/hosts')
hosts_content = """127.0.0.1\tlocalhost
127.0.1.1\tTizenOS

::1\t\tlocalhost ip6-localhost ip6-loopback
ff02::1\tip6-allnodes
ff02::2\tip6-allrouters
"""
with open(hosts_path, 'w') as f:
    f.write(hosts_content)

print("[SUCCESS] Calamares TizenOS branding & /etc/hosts updated successfully!")
