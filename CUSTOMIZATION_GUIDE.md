# 🚀 Hướng Dẫn Tùy Biến (Customize), Cài Thêm Ứng Dụng & Đóng Góp Phát Triển TizenOS

Chào mừng bạn đến với **TizenOS Project**! Tài liệu này hướng dẫn chi tiết cách build hệ điều hành từ con số 0, tùy biến giao diện/phần mềm, đóng gói đĩa Live ISO cá nhân hóa và tham gia bảo trì dự án.

---

## 📋 1. Môi Trường & Công Cụ Yêu Cầu

### Yêu cầu hệ thống:
- **Hệ điều hành**: Linux Native (Debian/Ubuntu) hoặc **WSL 2** trên Windows.
- **Dung lượng đĩa trống**: Tối thiểu **10 GB**.
- **Quyền hạn**: Quyền `root` hoặc `sudo`.

### Cài đặt công cụ đóng gói cần thiết:
```bash
sudo apt-get update
sudo apt-get install -y debootstrap squashfs-tools xorriso isolinux syslinux-common \
                        grub-pc-bin grub-efi-amd64-bin mtools dosfstools rsync git
```

---

## 🛠️ 2. Hướng Dẫn Build ISO Từ Đầu (Build From Scratch)

### Bước 1: Clone Repository
```bash
git clone https://github.com/nguyenquocanhz/TizenOS.git
cd TizenOS
```

### Bước 2: Tạo RootFS hệ thống bằng Debootstrap
Chạy kịch bản khởi tạo hệ thống cơ bản:
```bash
sudo bash build/scripts/bootstrap.sh
```
*Kịch bản này sẽ tải gói Debian 12 Base, cấu hình user `tizen`, cài đặt Calamares Installer, theme Fluent-Dark và nạp đầy đủ VMware/SATA drivers.*

### Bước 3: Đóng Gói Thành Đĩa Hybrid Live ISO
```bash
sudo bash build/scripts/create-iso.sh
```
Sau khi hoàn tất, tệp ISO sẽ xuất ra tại:
👉 `build/output/tizenos-live.iso` (Có thể mount trực tiếp trên VMware, VirtualBox hoặc ghi ra USB bằng Rufus/Ventoy/BalenaEtcher).

---

## 🎨 3. Hướng Dẫn Tùy Biến OS & Cài Thêm Ứng Dụng (Customization Guide)

### 3.1. Cài Đặt Thêm Phần Mềm Mặc Định Vào Đĩa ISO
Để thêm các phần mềm yêu thích (ví dụ: `code`, `vlc`, `gimp`, `steam`, `discord`, `telegram-desktop`...) vào đĩa ISO cài đặt sẵn:

1. Mở tệp [`build/scripts/bootstrap.sh`](build/scripts/bootstrap.sh).
2. Tìm đến lệnh `apt-get install` (khoảng dòng 68-80) và thêm tên gói mong muốn:
```bash
# Ví dụ: Thêm VS Code, Telegram và VLC
apt-get install -y \
    code telegram-desktop vlc obs-studio gimp \
    ...
```
3. Hoặc chroot trực tiếp vào RootFS đang build để cài gói cá nhân:
```bash
sudo chroot /var/tmp/tizenos_rootfs apt-get install -y <ten_goi_phan_mem>
```

### 3.2. Thay Đổi Theme Giao Diện, Icon & Hình Nền Mặc Định
- **Giao diện GTK3/GTK4**: Đặt thư mục theme vào `/var/tmp/tizenos_rootfs/usr/share/themes/`.
- **Bộ Icon**: Đặt thư mục biểu tượng vào `/var/tmp/tizenos_rootfs/usr/share/icons/`.
- **Hình nền mặc định (Wallpaper)**: Đặt ảnh vào `/var/tmp/tizenos_rootfs/usr/share/backgrounds/`.
- **Cấu hình mặc định cho tất cả user mới**: Chỉnh sửa các file thiết lập tại `/var/tmp/tizenos_rootfs/etc/skel/.config/`.

### 3.3. Tự Định Nghĩa Phím Tắt Hệ Thống (Global Hotkeys)
TizenOS sử dụng XFCE4 Window Manager. Để thêm phím tắt mở ứng dụng:
Mở tệp [`build/scripts/bootstrap.sh`](build/scripts/bootstrap.sh) và chỉnh sửa thẻ `xfce4-keyboard-shortcuts.xml`:
```xml
<property name="<Primary><Alt>t" type="string" value="xfce4-terminal"/>
<property name="<Super>i" type="string" value="tizenos-installer-gui"/>
<property name="<Super>c" type="string" value="code"/>
```

---

## 🛡️ 4. Quy Chuẩn Phân Quyền & Bảo Mật (Security & Polkit)

Khi tạo ứng dụng hoặc tiện ích mới chạy quyền root không cần gõ mật khẩu (như bộ cài):
1. **Thêm Polkit Rule**: Đặt file `.rules` vào `/etc/polkit-1/rules.d/10-custom-rule.rules`:
```javascript
polkit.addRule(function(action, subject) {
    if (subject.isInGroup("sudo")) {
        return polkit.Result.YES;
    }
});
```
2. **Khai báo Sudoers NOPASSWD**: Đặt vào `/etc/sudoers.d/tizenos` với quyền file `0440`.

---

## 🤝 5. Hướng Dẫn Tham Gia Bảo Trì & Đóng Góp (Contribution Guide)

Chúng tôi rất hoan nghênh sự đóng góp từ cộng đồng! Để đóng góp code cho TizenOS:

1. **Fork Repository**: Fork `nguyenquocanhz/TizenOS` về tài khoản GitHub của bạn.
2. **Tạo Nhánh (Branch)**:
   ```bash
   git checkout -b feature/ten-tinh-nang-moi
   # hoặc
   git checkout -b fix/ten-loi-can-sua
   ```
3. **Kiểm Tra Build**: Đảm bảo chạy `create-iso.sh` thành công và test đĩa ISO trên máy ảo trước khi commit.
4. **Commit Chuẩn (Conventional Commits)**:
   - `feat(installer): Thêm tính năng mới cho bộ cài`
   - `fix(boot): Khắc phục lỗi driver UEFI`
   - `docs(guide): Cập nhật tài liệu tùy biến`
5. **Tạo Pull Request (PR)**: Đẩy nhánh lên GitHub và mở Pull Request kèm mô tả chi tiết các thay đổi.

---

🎉 **Cảm ơn bạn đã đồng hành và phát triển TizenOS!**
