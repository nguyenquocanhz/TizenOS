#!/bin/bash
# ==============================================================================
# TizenOS Live ISO Bootstrap Script
# Debian 12 (Bookworm) Base RootFS Generator
# ==============================================================================

set -euo pipefail

ROOTFS_DIR="${1:-/var/tmp/tizenos_rootfs}"
DISTRO="bookworm"
MIRROR="http://deb.debian.org/debian"

echo "======================================================================"
echo " Khởi tạo Debian 12 RootFS bằng debootstrap..."
echo " Target Directory: $ROOTFS_DIR"
echo "======================================================================"

# 1. Kiểm tra & cài đặt debootstrap nếu chưa có
if ! command -v debootstrap &>/dev/null; then
    echo "Đang cài đặt debootstrap..."
    apt-get update && apt-get install -y debootstrap
fi

# 2. Xóa và tạo lại thư mục rootfs
rm -rf "$ROOTFS_DIR"
mkdir -p "$ROOTFS_DIR"

KEYRING_OPT=""
if [ -f "/usr/share/keyrings/debian-archive-keyring.gpg" ]; then
    KEYRING_OPT="--keyring=/usr/share/keyrings/debian-archive-keyring.gpg"
fi

# 3. Tiến hành debootstrap bản base debian 12
debootstrap $KEYRING_OPT --no-check-gpg --arch=amd64 "$DISTRO" "$ROOTFS_DIR" "$MIRROR"

# 4. Mount các hệ thống tệp ảo để chroot
mount -t proc /proc "$ROOTFS_DIR/proc"
mount -t sysfs /sys "$ROOTFS_DIR/sys"
mount --bind /dev "$ROOTFS_DIR/dev"
mount --bind /dev/pts "$ROOTFS_DIR/dev/pts"

# 5. Khởi tạo script chroot bên trong RootFS
cat << 'CHROOT_EOF' > "$ROOTFS_DIR/bootstrap_in_chroot.sh"
#!/bin/bash
set -euo pipefail

export DEBIAN_FRONTEND=noninteractive

echo "TizenOS" > /etc/hostname

# Cấu hình Apt Repositories với non-free-firmware cho Intel Audio/Firmware & Wireless
cat << 'SOURCES_EOF' > /etc/apt/sources.list
deb http://deb.debian.org/debian bookworm main contrib non-free non-free-firmware
deb http://deb.debian.org/debian-security bookworm-security main contrib non-free non-free-firmware
deb http://deb.debian.org/debian bookworm-updates main contrib non-free non-free-firmware
SOURCES_EOF

# Cài đặt Kernel 6.1 LTS, Intel Audio Firmware, PipeWire, pavucontrol, Desktop GUI (XFCE4/LightDM/GTK4), Lightweight Apps, VLC
apt-get update -qq || true
apt-get install -y --no-install-recommends \
    linux-image-amd64 \
    firmware-sof-signed firmware-intel-sound firmware-linux-free \
    alsa-utils alsa-topology-conf alsa-ucm-conf \
    pipewire pipewire-audio pipewire-pulse wireplumber pulseaudio-utils pavucontrol \
    live-boot live-boot-initramfs-tools initramfs-tools \
    plymouth plymouth-themes plymouth-label \
    zstd xz-utils \
    systemd systemd-sysv \
    grub-pc-bin grub-efi-amd64-bin \
    locales sudo curl wget network-manager network-manager-gnome \
    htop neofetch btop tree pciutils usbutils lshw vim \
    parted gparted gdisk fdisk dosfstools e2fsprogs rsync x11-xserver-utils gdebi calamares calamares-settings-debian \
    xorg xserver-xorg lightdm lightdm-gtk-greeter \
    xfce4 xfce4-terminal thunar desktop-base \
    firefox-esr libgtk-4-1 libgtk-4-dev libadwaita-1-0 adwaita-icon-theme zenity \
    open-vm-tools open-vm-tools-desktop \
    mousepad ristretto evince gnome-calculator gnome-calendar \
    vlc gimp ffmpeg gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly gstreamer1.0-libav \
    firmware-linux firmware-realtek firmware-atheros firmware-iwlwifi \
    python3-pil \
    p7zip-full unzip zip unrar \
    xorriso isolinux syslinux-common || true

# Cấu hình Mime Type mặc định để double-click tệp .deb mở bằng GDebi Package Installer (Giống file .exe trên Windows)
mkdir -p /etc/skel/.config /home/tizen/.config /root/.config /usr/share/applications
for MIME_FILE in /etc/skel/.config/mimeapps.list /home/tizen/.config/mimeapps.list /root/.config/mimeapps.list /usr/share/applications/defaults.list; do
    mkdir -p "$(dirname "$MIME_FILE")"
    cat << 'MIME_EOF' >> "$MIME_FILE"
[Default Applications]
application/vnd.debian.binary-package=gdebi.desktop;
application/x-deb=gdebi.desktop;
application/x-debian-package=gdebi.desktop;
MIME_EOF
done

# Bật Framebuffer cho Initramfs để nạp Plymouth Boot Animation ngay từ những giây đầu boot & nạp overlayfs
mkdir -p /etc/initramfs-tools/conf.d /etc/initramfs-tools/modules
cat << 'SPLASH_CONF_EOF' > /etc/initramfs-tools/conf.d/splash
FRAMEBUFFER=y
MODULES=most
SPLASH_CONF_EOF

cat << 'MODULES_CONF_EOF' >> /etc/initramfs-tools/modules
overlay
squashfs
isofs
# === VMware Virtual Hardware Drivers ===
vmwgfx
vmw_pvscsi
vmw_vmci
vmxnet3
# === SCSI/SATA Storage Controllers (phần cứng thực) ===
mptspi
mptscsih
sd_mod
sr_mod
ata_piix
ahci
nvme
# === Filesystem & Block Layer ===
dm_mod
ext4
vfat
MODULES_CONF_EOF

# Tạo thư mục Plymouth Spinner Theme và tệp logo watermark.png tùy chỉnh
mkdir -p /usr/share/plymouth/themes/spinner

python3 -c "
from PIL import Image, ImageDraw, ImageFont
img = Image.new('RGBA', (320, 320), (0, 0, 0, 0))
draw = ImageDraw.Draw(img)
# Vẽ logo vòng xoay TizenOS 🌀
draw.ellipse([30, 30, 290, 290], outline=(137, 180, 250, 255), width=10)
draw.ellipse([70, 70, 250, 250], outline=(180, 190, 254, 220), width=6)
draw.ellipse([110, 110, 210, 210], fill=(137, 180, 250, 255))
img.save('/usr/share/plymouth/themes/spinner/watermark.png')
" 2>/dev/null || touch /usr/share/plymouth/themes/spinner/watermark.png

# Tạo Plymouth Theme TizenOS tùy chỉnh (Tích hợp Logo 🌀 + Dòng chữ Text Label)
mkdir -p /usr/share/plymouth/themes/tizenos

cat << 'PLYMOUTH_CONF_EOF' > /usr/share/plymouth/themes/tizenos/tizenos.plymouth
[Plymouth Theme]
Name=TizenOS Theme
Description=Custom TizenOS Plymouth Boot Splash with Logo and Text Label
ModuleName=script

[script]
ImageDir=/usr/share/plymouth/themes/tizenos
ScriptFile=/usr/share/plymouth/themes/tizenos/tizenos.script
PLYMOUTH_CONF_EOF

cat << 'PLYMOUTH_SCRIPT_EOF' > /usr/share/plymouth/themes/tizenos/tizenos.script
Window.SetBackgroundTopColor(0.12, 0.12, 0.18);
Window.SetBackgroundBottomColor(0.09, 0.09, 0.15);

logo.image = Image("logo.png");
logo.sprite = Sprite(logo.image);
logo.sprite.SetX(Window.GetWidth() / 2 - logo.image.GetWidth() / 2);
logo.sprite.SetY(Window.GetHeight() / 2 - logo.image.GetHeight() / 2 - 30);

progress_angle = 0;
fun refresh_callback() {
    progress_angle += 0.05;
}
Plymouth.SetRefreshFunction(refresh_callback);
PLYMOUTH_SCRIPT_EOF

cp /usr/share/plymouth/themes/spinner/watermark.png /usr/share/plymouth/themes/tizenos/logo.png 2>/dev/null || true

# Thiết lập theme Spinner chuẩn của Debian có watermark TizenOS & cập nhật initramfs
plymouth-set-default-theme -R spinner 2>/dev/null || plymouth-set-default-theme -R tizenos 2>/dev/null || true
update-initramfs -u -k all 2>/dev/null || true

# Cấu hình Alt+Tab Window Switcher & 4 Workspace Shortcuts cho XFCE4
for CONFIG_DIR in /etc/skel/.config/xfce4/xfconf/xfce-perchannel-xml /home/tizen/.config/xfce4/xfconf/xfce-perchannel-xml /root/.config/xfce4/xfconf/xfce-perchannel-xml; do
    mkdir -p "$CONFIG_DIR"
    
    cat << 'XFWM4_XML_EOF' > "$CONFIG_DIR/xfwm4.xml"
<?xml version="1.0" encoding="UTF-8"?>

<channel name="xfwm4" version="1.0">
  <property name="general" type="empty">
    <property name="workspace_count" type="int" value="4"/>
    <property name="cycle_tabwin" type="bool" value="true"/>
    <property name="cycle_draw_shortcut" type="bool" value="true"/>
    <property name="cycle_preview" type="bool" value="true"/>
    <property name="cycle_workspaces" type="bool" value="true"/>
    <property name="workspace_names" type="array">
      <value type="string" value="Chính 1"/>
      <value type="string" value="Văn Phòng 2"/>
      <value type="string" value="Duyệt Web 3"/>
      <value type="string" value="Công Việc 4"/>
    </property>
  </property>
</channel>
XFWM4_XML_EOF

    cat << 'XFCE4_KEYBOARD_EOF' > "$CONFIG_DIR/xfce4-keyboard-shortcuts.xml"
<?xml version="1.0" encoding="UTF-8"?>

<channel name="xfce4-keyboard-shortcuts" version="1.0">
  <property name="commands" type="empty">
    <property name="custom" type="empty">
      <property name="&lt;Primary&gt;&lt;Alt&gt;t" type="string" value="xfce4-terminal"/>
      <property name="&lt;Super&gt;e" type="string" value="thunar"/>
      <property name="&lt;Super&gt;w" type="string" value="firefox-esr"/>
    </property>
  </property>
  <property name="xfwm4" type="empty">
    <property name="custom" type="empty">
      <property name="&lt;Alt&gt;Tab" type="string" value="cycle_windows_key"/>
      <property name="&lt;Primary&gt;&lt;Alt&gt;Left" type="string" value="left_workspace_key"/>
      <property name="&lt;Primary&gt;&lt;Alt&gt;Right" type="string" value="right_workspace_key"/>
      <property name="&lt;Super&gt;1" type="string" value="workspace_1_key"/>
      <property name="&lt;Super&gt;2" type="string" value="workspace_2_key"/>
      <property name="&lt;Super&gt;3" type="string" value="workspace_3_key"/>
      <property name="&lt;Super&gt;4" type="string" value="workspace_4_key"/>
    </property>
  </property>
</channel>
XFCE4_KEYBOARD_EOF

done

# Cấu hình Fluent-Dark GTK Theme chuẩn vinceliuice mặc định cho hệ thống (GTK3, GTK4 & XFCE4)
for GTK_DIR in /etc/skel/.config /home/tizen/.config /root/.config; do
    mkdir -p "$GTK_DIR/gtk-3.0" "$GTK_DIR/gtk-4.0" "$GTK_DIR/xfce4/xfconf/xfce-perchannel-xml"
    
    cat << 'GTK3_SETTINGS_EOF' > "$GTK_DIR/gtk-3.0/settings.ini"
[Settings]
gtk-theme-name=Fluent-Dark
gtk-icon-theme-name=Adwaita
gtk-cursor-theme-name=Adwaita
gtk-application-prefer-dark-theme=1
GTK3_SETTINGS_EOF

    cat << 'GTK4_SETTINGS_EOF' > "$GTK_DIR/gtk-4.0/settings.ini"
[Settings]
gtk-theme-name=Fluent-Dark
gtk-application-prefer-dark-theme=1
GTK4_SETTINGS_EOF

    cat << 'XSETTINGS_EOF' > "$GTK_DIR/xfce4/xfconf/xfce-perchannel-xml/xsettings.xml"
<?xml version="1.0" encoding="UTF-8"?>
<channel name="xsettings" version="1.0">
  <property name="Net" type="empty">
    <property name="ThemeName" type="string" value="Fluent-Dark"/>
    <property name="IconThemeName" type="string" value="Adwaita"/>
  </property>
  <property name="Gtk" type="empty">
    <property name="CursorThemeName" type="string" value="Adwaita"/>
  </property>
</channel>
XSETTINGS_EOF
done

chown -R tizen:tizen /home/tizen/.config 2>/dev/null || true

# Set default systemd target to graphical.target & enable open-vm-tools service for VMware
systemctl set-default graphical.target || true
systemctl enable open-vm-tools.service 2>/dev/null || true

cat << 'FSTAB_LIVE_EOF' > /etc/fstab
tmpfs /tmp tmpfs defaults,noatime,mode=1777 0 0
tmpfs /dev/shm tmpfs defaults,nosuid,nodev 0 0
FSTAB_LIVE_EOF

# Copy compiled GTK4 tizenos-installer & tizenos-welcome binaries
if [ -f "/mnt/d/TizenOS/installer/tizenos-installer/build/tizenos-installer" ]; then
    cp "/mnt/d/TizenOS/installer/tizenos-installer/build/tizenos-installer" /usr/local/bin/tizenos-installer
    chmod +x /usr/local/bin/tizenos-installer
fi

if [ -f "/mnt/d/TizenOS/installer/tizenos-installer/build/tizenos-welcome" ]; then
    cp "/mnt/d/TizenOS/installer/tizenos-installer/build/tizenos-welcome" /usr/local/bin/tizenos-welcome
    chmod +x /usr/local/bin/tizenos-welcome
fi

# Tạo wrapper script /usr/local/bin/tizenos-installer-gui với cơ chế Fail-Safe 3 Tầng (GTK4 -> Terminal Window -> Zenity)
cat << 'GUI_WRAPPER_EOF' > /usr/local/bin/tizenos-installer-gui
#!/bin/bash
export DISPLAY="${DISPLAY:-:0}"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"
export XAUTHORITY="${XAUTHORITY:-$HOME/.Xauthority}"
export GDK_BACKEND=x11
export GTK_THEME=Adwaita:dark

xhost +local:root >/dev/null 2>&1 || true
xhost +si:localuser:root >/dev/null 2>&1 || true

# Tầng 1: Thử chạy GTK4 C Installer nếu file tồn tại
if [ -x "/usr/local/bin/tizenos-installer" ]; then
    echo "[LAUNCHER] Đang mở bộ cài GTK4 Native..."
    if sudo -E GDK_BACKEND=x11 DISPLAY="$DISPLAY" XAUTHORITY="$XAUTHORITY" XDG_RUNTIME_DIR="$XDG_RUNTIME_DIR" /usr/local/bin/tizenos-installer "$@"; then
        exit 0
    fi
    echo "[LAUNCHER] GTK4 Native chưa sẵn sàng, chuyển sang bộ cài cửa sổ Terminal..."
fi

# Tầng 2: Mở bộ cài đặt trong cửa sổ Terminal giao diện đẹp
if command -v xfce4-terminal >/dev/null 2>&1; then
    exec xfce4-terminal --title="🚀 BỘ CÀI ĐẶT TIZENOS" --geometry=95x30 -e "sudo /usr/local/bin/tizenos-install"
elif command -v xterm >/dev/null 2>&1; then
    exec xterm -title "🚀 BỘ CÀI ĐẶT TIZENOS" -geometry 95x30 -e "sudo /usr/local/bin/tizenos-install"
fi

# Tầng 3: Chạy trực tiếp qua sudo
exec sudo /usr/local/bin/tizenos-install "$@"
GUI_WRAPPER_EOF
chmod +x /usr/local/bin/tizenos-installer-gui

for DESKTOP_DIR in /etc/skel/Desktop /home/tizen/Desktop; do
    mkdir -p "$DESKTOP_DIR"
    cat << 'DESKTOP_ENTRY_EOF' > "$DESKTOP_DIR/install-tizenos.desktop"
[Desktop Entry]
Version=1.0
Type=Application
Name=Cài đặt TizenOS (GTK4 Installer)
Comment=Trình cài đặt 7 bước GTK4 TizenOS
Exec=/usr/local/bin/tizenos-installer-gui
Icon=system-software-install
Terminal=false
Categories=System;Installer;
DESKTOP_ENTRY_EOF
    chmod +x "$DESKTOP_DIR/install-tizenos.desktop"

    cat << 'WELCOME_ENTRY_EOF' > "$DESKTOP_DIR/tizenos-welcome.desktop"
[Desktop Entry]
Version=1.0
Type=Application
Name=Chào Mừng TizenOS (Welcome C)
Comment=Ứng dụng Chào mừng TizenOS GTK4
Exec=/usr/local/bin/tizenos-welcome
Icon=preferences-desktop-personal
Terminal=false
Categories=System;Utility;
WELCOME_ENTRY_EOF
    chmod +x "$DESKTOP_DIR/tizenos-welcome.desktop"
done

# Autostart GTK4 Welcome app & Installer popup immediately when Desktop opens
for AUTOSTART_DIR in /etc/skel/.config/autostart /home/tizen/.config/autostart; do
    mkdir -p "$AUTOSTART_DIR"
    cat << 'AUTOSTART_WELCOME_EOF' > "$AUTOSTART_DIR/tizenos-welcome.desktop"
[Desktop Entry]
Type=Application
Name=Chào Mừng TizenOS
Exec=/usr/local/bin/tizenos-welcome
Icon=preferences-desktop-personal
Terminal=false
X-GNOME-Autostart-enabled=true
AUTOSTART_WELCOME_EOF
    chmod +x "$AUTOSTART_DIR/tizenos-welcome.desktop"

    cat << 'AUTOSTART_ENTRY_EOF' > "$AUTOSTART_DIR/tizenos-installer.desktop"
[Desktop Entry]
Type=Application
Name=Cài đặt TizenOS (GTK4 Installer)
Exec=/usr/local/bin/tizenos-installer-gui
Icon=system-software-install
Terminal=false
X-GNOME-Autostart-enabled=true
AUTOSTART_ENTRY_EOF
    chmod +x "$AUTOSTART_DIR/tizenos-installer.desktop"
done

groupadd -r nopasswdlogin 2>/dev/null || true
id -u tizen &>/dev/null || useradd -m -s /bin/bash -G sudo,nopasswdlogin tizen 2>/dev/null || useradd -m -s /bin/bash tizen
usermod -aG sudo,nopasswdlogin tizen 2>/dev/null || true

echo "tizen:live" | chpasswd
echo "root:tizenroot" | chpasswd

# Cấu hình Sudo NOPASSWD cho user tizen để autostart installer không bị chặn password
mkdir -p /etc/sudoers.d
echo "tizen ALL=(ALL) NOPASSWD: ALL" > /etc/sudoers.d/tizenos
chmod 0440 /etc/sudoers.d/tizenos

# Cấu hình Polkit Rule Ubuntu-Style cho Live Installer (Cho phép format đĩa, pkexec, cài đặt OS không cần mật khẩu)
mkdir -p /etc/polkit-1/rules.d /usr/share/polkit-1/actions
cat << 'POLKIT_LIVE_EOF' > /etc/polkit-1/rules.d/10-tizenos-live-installer.rules
/* Ubuntu Casper / Live Installer Polkit Rule */
polkit.addRule(function(action, subject) {
    if (subject.isInGroup("sudo") || subject.isInGroup("nopasswdlogin")) {
        return polkit.Result.YES;
    }
});
POLKIT_LIVE_EOF

cat << 'POLKIT_ACTION_EOF' > /usr/share/polkit-1/actions/org.tizenos.installer.policy
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE policyconfig PUBLIC "-//freedesktop//DTD PolicyKit Policy Configuration 1.0//EN"
"http://www.freedesktop.org/standards/PolicyKit/1/policyconfig.dtd">
<policyconfig>
  <action id="org.tizenos.installer">
    <description>Install TizenOS System</description>
    <message>Authentication is required to install TizenOS</message>
    <defaults>
      <allow_any>yes</allow_any>
      <allow_inactive>yes</allow_inactive>
      <allow_active>yes</allow_active>
    </defaults>
    <annotate key="org.freedesktop.policykit.exec.path">/usr/local/bin/tizenos-installer</annotate>
    <annotate key="org.freedesktop.policykit.exec.allow_gui">true</annotate>
  </action>
</policyconfig>
POLKIT_ACTION_EOF

# Cấu hình Mặc Định Autologin cho LightDM
mkdir -p /etc/lightdm/lightdm.conf.d
cat << 'LIGHTDM_AUTOLOGIN_EOF' > /etc/lightdm/lightdm.conf.d/80-tizenos-autologin.conf
[Seat:*]
autologin-user=tizen
autologin-user-timeout=0
user-session=xfce
pam-service=lightdm-autologin
LIGHTDM_AUTOLOGIN_EOF

# Đảm bảo quyền sở hữu tất cả các file cấu hình cho user tizen
chown -R tizen:tizen /home/tizen 2>/dev/null || true

# Tạo công cụ thiết lập tài khoản tương tác (User Setup Wizard)
cat << 'SETUP_SCRIPT_EOF' > /usr/local/bin/tizenos-setup
#!/bin/bash
set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "Vui lòng chạy lệnh với quyền root: sudo tizenos-setup"
    exit 1
fi

clear
echo "======================================================================"
echo " 🚀 BỘ HƯỚNG DẪN THIẾT LẬP TÀI KHOẢN TIZENOS (USER SETUP WIZARD)"
echo "======================================================================"
echo ""

read -p "1. Nhập Tên người dùng mới (Username): " NEW_USER
if [ -z "$NEW_USER" ]; then
    echo "Tên người dùng không được để trống!"
    exit 1
fi

read -p "2. Nhập Họ và tên hiển thị (Full Name): " FULL_NAME
FULL_NAME="${FULL_NAME:-$NEW_USER}"

read -sp "3. Nhập Mật khẩu mới cho $NEW_USER: " NEW_PASS
echo ""
read -sp "   Nhập lại Mật khẩu xác nhận: " CONFIRM_PASS
echo ""

if [ "$NEW_PASS" != "$CONFIRM_PASS" ]; then
    echo "Lỗi: Mật khẩu xác nhận không khớp!"
    exit 1
fi

read -p "4. Nhập Tên máy tính (Hostname) [Default: TizenOS-PC]: " NEW_HOST
NEW_HOST="${NEW_HOST:-TizenOS-PC}"

echo ""
echo "Đang khởi tạo tài khoản $NEW_USER..."
useradd -m -c "$FULL_NAME" -s /bin/bash -G sudo "$NEW_USER" 2>/dev/null || true
echo "$NEW_USER:$NEW_PASS" | chpasswd
echo "$NEW_HOST" > /etc/hostname

if [ -d /etc/lightdm/lightdm.conf.d ]; then
cat << 'AUTOLOGIN_CONF_EOF' > /etc/lightdm/lightdm.conf.d/80-tizenos-autologin.conf
[Seat:*]
autologin-user=$NEW_USER
autologin-user-timeout=0
user-session=xfce
AUTOLOGIN_CONF_EOF
fi

echo "======================================================================"
echo " ✓ ĐÃ TẠO THÀNH CÔNG TÀI KHOẢN MỚI: $NEW_USER ($NEW_HOST)!"
echo "======================================================================"
SETUP_SCRIPT_EOF

chmod +x /usr/local/bin/tizenos-setup

# Tạo công cụ 7-Step Disk Installer tự động (/usr/local/bin/tizenos-install)
cat << 'INSTALL_SCRIPT_EOF' > /usr/local/bin/tizenos-install
#!/bin/bash
set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "Lỗi: Vui lòng chạy bộ cài đặt với quyền root (sudo tizenos-install)"
    exit 1
fi

clear
echo "======================================================================"
echo "      🚀 BỘ CÀI ĐẶT 7 BƯỚC TIZENOS AUTOMATED DISK INSTALLER            "
echo "======================================================================"
echo ""
echo "Các ổ đĩa cứng hiện có trên máy tính:"
lsblk -d -n -o NAME,SIZE,MODEL,TYPE | grep -E "disk"
echo ""

read -p "Bước 1/7 [Đĩa đích]: Nhập tên ổ đĩa cần cài (vd: sda, nvme0n1, vda): " TARGET_DISK_NAME
TARGET_DISK="/dev/$TARGET_DISK_NAME"

if [ ! -b "$TARGET_DISK" ]; then
    echo "Lỗi: Không tìm thấy thiết bị đĩa $TARGET_DISK"
    exit 1
fi

read -p "Bước 2/7 [Username]: Nhập tên tài khoản người dùng chính: " NEW_USER
if [ -z "$NEW_USER" ]; then NEW_USER="tizen"; fi

read -sp "Bước 3/7 [Password]: Nhập mật khẩu cho $NEW_USER: " NEW_PASS
echo ""

read -p "Bước 4/7 [Hostname]: Nhập tên máy tính [Default: TizenOS-PC]: " NEW_HOST
NEW_HOST="${NEW_HOST:-TizenOS-PC}"

read -p "Bước 5/7 [Timezone]: Nhập múi giờ [Default: Asia/Ho_Chi_Minh]: " NEW_TZ
NEW_TZ="${NEW_TZ:-Asia/Ho_Chi_Minh}"

echo ""
echo "======================================================================"
echo " CẢNH BÁO: BỘ CÀI ĐẶT SẼ XÓA SẠCH DỮ LIỆU CŨ VÀ TẠO MỚI PHÂN VÙNG:"
echo " - Phân vùng 1: BIOS Boot Partition (2MB) cho Legacy BIOS"
echo " - Phân vùng 2 (ESP): FAT32 512MB (/boot/efi) cho UEFI"
echo " - Phân vùng 3 (SWAP): 4GB"
echo " - Phân vùng 4 (Root OS): ext4 Ít nhất 20GB+ đến hết đĩa (/)"
echo "======================================================================"
read -p "Gõ 'YES' để xác nhận tiếp tục cài sạch đĩa $TARGET_DISK: " CONFIRM

if [ "$CONFIRM" != "YES" ]; then
    echo "Đã hủy bỏ quá trình cài đặt."
    exit 1
fi

parted -s "$TARGET_DISK" mklabel gpt
parted -s "$TARGET_DISK" mkpart primary 1MiB 3MiB
parted -s "$TARGET_DISK" set 1 bios_grub on
parted -s "$TARGET_DISK" mkpart ESP fat32 3MiB 515MiB
parted -s "$TARGET_DISK" set 2 esp on
parted -s "$TARGET_DISK" mkpart primary linux-swap 515MiB 4611MiB
parted -s "$TARGET_DISK" mkpart primary ext4 4611MiB 100%

if [[ "$TARGET_DISK" == *"nvme"* ]]; then
    PART_EFI="${TARGET_DISK}p2"
    PART_SWAP="${TARGET_DISK}p3"
    PART_ROOT="${TARGET_DISK}p4"
else
    PART_EFI="${TARGET_DISK}2"
    PART_SWAP="${TARGET_DISK}3"
    PART_ROOT="${TARGET_DISK}4"
fi

mkfs.fat -F32 "$PART_EFI"
mkswap "$PART_SWAP"
mkfs.ext4 -F -L "TIZEN_ROOT" "$PART_ROOT"

MOUNT_TARGET="/mnt/target"
mkdir -p "$MOUNT_TARGET"
mount "$PART_ROOT" "$MOUNT_TARGET"
mkdir -p "$MOUNT_TARGET/boot/efi"
mount "$PART_EFI" "$MOUNT_TARGET/boot/efi"

if [ -d "/run/live/rootfs" ]; then
    rsync -aHAX --info=progress2 --exclude='/proc/*' --exclude='/sys/*' --exclude='/dev/*' --exclude='/tmp/*' --exclude='/run/*' --exclude='/mnt/*' /run/live/rootfs/ "$MOUNT_TARGET/"
else
    rsync -aHAX --info=progress2 --exclude='/proc/*' --exclude='/sys/*' --exclude='/dev/*' --exclude='/tmp/*' --exclude='/run/*' --exclude='/mnt/*' / "$MOUNT_TARGET/"
fi

ROOT_UUID=$(blkid -s UUID -o value "$PART_ROOT")
EFI_UUID=$(blkid -s UUID -o value "$PART_EFI")
SWAP_UUID=$(blkid -s UUID -o value "$PART_SWAP")

cat << FSTAB_DISKS_EOF > "$MOUNT_TARGET/etc/fstab"
UUID=$ROOT_UUID  /               ext4    noatime,errors=remount-ro  0  1
UUID=$EFI_UUID   /boot/efi       vfat    umask=0077                 0  2
UUID=$SWAP_UUID  none            swap    sw                         0  0
tmpfs            /tmp            tmpfs   defaults,noatime,mode=1777 0  0
FSTAB_DISKS_EOF

echo "$NEW_HOST" > "$MOUNT_TARGET/etc/hostname"
ln -sf "/usr/share/zoneinfo/$NEW_TZ" "$MOUNT_TARGET/etc/localtime" 2>/dev/null || true

chroot "$MOUNT_TARGET" /bin/bash -c "useradd -m -s /bin/bash -G sudo '$NEW_USER'; echo '$NEW_USER:$NEW_PASS' | chpasswd"

mount --bind /dev "$MOUNT_TARGET/dev"
mount --bind /dev/pts "$MOUNT_TARGET/dev/pts"
mount --bind /proc "$MOUNT_TARGET/proc"
mount --bind /sys "$MOUNT_TARGET/sys"

# Mount efivarfs cho grub-install UEFI (Samsung Tizen best practice)
if [ -d /sys/firmware/efi/efivars ]; then
    mkdir -p "$MOUNT_TARGET/sys/firmware/efi/efivars" 2>/dev/null || true
    mount --bind /sys/firmware/efi/efivars "$MOUNT_TARGET/sys/firmware/efi/efivars" 2>/dev/null || true
fi

chroot "$MOUNT_TARGET" /bin/bash -c "grub-install --target=i386-pc --recheck '$TARGET_DISK'; grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=TizenOS --recheck --removable; update-grub"

umount -R "$MOUNT_TARGET"

echo "======================================================================"
echo " ✓ ĐÃ CÀI ĐẶT THÀNH CÔNG TIZENOS LÊN ĐĨA $TARGET_DISK!"
echo " Hãy tháo đĩa ISO và khởi động lại máy tính."
echo "======================================================================"
INSTALL_SCRIPT_EOF

# Cấp quyền thực thi full (755) cho tất cả các file công cụ và bộ cài
chmod -R 755 /usr/local/bin /usr/lib/tizenos 2>/dev/null || true
chmod +x /usr/local/bin/* /etc/skel/Desktop/*.desktop /home/tizen/Desktop/*.desktop 2>/dev/null || true

# Đảm bảo quyền sở hữu /home/tizen thuộc tizen:tizen 100% để XFCE4/LightDM không bị chớp nhả đăng nhập
chown -R tizen:tizen /home/tizen 2>/dev/null || true
chmod 755 /home/tizen 2>/dev/null || true

# Làm sạch apt
apt-get clean || true
rm -rf /var/lib/apt/lists/* || true
CHROOT_EOF

chmod +x "$ROOTFS_DIR/bootstrap_in_chroot.sh"
chroot "$ROOTFS_DIR" /bootstrap_in_chroot.sh || true
rm -f "$ROOTFS_DIR/bootstrap_in_chroot.sh"

# 6. Dọn dẹp Virtual Mounts sau khi hoàn tất
umount -l "$ROOTFS_DIR/dev/pts" 2>/dev/null || true
umount -l "$ROOTFS_DIR/sys" 2>/dev/null || true
umount -l "$ROOTFS_DIR/proc" 2>/dev/null || true
umount -l "$ROOTFS_DIR/dev" 2>/dev/null || true

# Đảm bảo các thư mục mount point rỗng tồn tại trong rootfs
mkdir -p "$ROOTFS_DIR/proc" "$ROOTFS_DIR/sys" "$ROOTFS_DIR/dev" "$ROOTFS_DIR/dev/pts" "$ROOTFS_DIR/run" "$ROOTFS_DIR/tmp" "$ROOTFS_DIR/mnt" "$ROOTFS_DIR/media"
chmod 1777 "$ROOTFS_DIR/tmp"

echo "======================================================================"
echo " ✓ Debootstrap hoàn tất cấu hình base RootFS!"
echo "======================================================================"
