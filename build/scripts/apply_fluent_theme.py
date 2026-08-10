import os

gtk3 = """[Settings]
gtk-theme-name=Fluent-Dark
gtk-icon-theme-name=Adwaita
gtk-cursor-theme-name=Adwaita
gtk-application-prefer-dark-theme=1
"""

gtk4 = """[Settings]
gtk-theme-name=Fluent-Dark
gtk-application-prefer-dark-theme=1
"""

xset = """<?xml version="1.0" encoding="UTF-8"?>
<channel name="xsettings" version="1.0">
  <property name="Net" type="empty">
    <property name="ThemeName" type="string" value="Fluent-Dark"/>
    <property name="IconThemeName" type="string" value="Adwaita"/>
  </property>
  <property name="Gtk" type="empty">
    <property name="CursorThemeName" type="string" value="Adwaita"/>
  </property>
</channel>
"""

dirs = [
    '/var/tmp/tizenos_rootfs/etc/skel/.config',
    '/var/tmp/tizenos_rootfs/home/tizen/.config',
    '/var/tmp/tizenos_rootfs/root/.config'
]

for d in dirs:
    os.makedirs(d + '/gtk-3.0', exist_ok=True)
    os.makedirs(d + '/gtk-4.0', exist_ok=True)
    os.makedirs(d + '/xfce4/xfconf/xfce-perchannel-xml', exist_ok=True)
    
    with open(d + '/gtk-3.0/settings.ini', 'w') as f:
        f.write(gtk3)
    with open(d + '/gtk-4.0/settings.ini', 'w') as f:
        f.write(gtk4)
    with open(d + '/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml', 'w') as f:
        f.write(xset)

os.system('chown -R 1000:1000 /var/tmp/tizenos_rootfs/home/tizen')
print("[SUCCESS] Fluent-Dark GTK theme settings applied successfully!")
