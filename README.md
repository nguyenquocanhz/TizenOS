# 🚀 TizenOS (Debian Edition) — System Architecture & Design Guide

Official Repository: [https://github.com/nguyenquocanhz/TizenOS](https://github.com/nguyenquocanhz/TizenOS)

---

## 💡 1. Cảm hứng Xây dựng (Inspiration & Vision)

### ❓ Vấn đề của các Hệ điều hành Linux Desktop hiện tại:
- **Xung đột & Thiếu chuẩn hóa Kiến trúc App**: Các bản phân phối Linux phổ biến (Ubuntu, Fedora, Arch) phụ thuộc hoàn toàn vào các Desktop Environment truyền thống (GNOME, KDE) vốn không được thiết kế có sẵn một **App Framework** di động/bảo mật cấp Kernel.
- **Tiềm năng chưa khai phá của Tizen OS**: Tizen OS (do Samsung và Linux Foundation phát triển) đã chứng minh tính hiệu quả vượt trội trên hàng trăm triệu thiết bị Smart TV và Smartwatch nhờ khả năng **khởi động app cực nhanh (Launchpad pre-fork)**, **bảo mật kiểm soát truy cập bắt buộc (Smack MAC)**, và **Cynara Policy Engine**. Tuy nhiên, Tizen OS gốc lại không thể chạy mượt mà trên máy tính PC/Laptop do thiếu kho phần mềm PC.

### 🎯 Sứ mệnh của TizenOS (Debian Edition):
TizenOS ra đời để kết hợp **hai thế giới đỉnh cao**:
> **"Sự ổn định và kho phần mềm 60,000+ gói khổng lồ của Debian 12 (Bookworm)"**  
> ➕  
> **"Kiến trúc Bảo mật Cấp Kernel & Khung Ứng dụng Siêu nhẹ của Tizen OS"**

Tạo nên một Hệ điều hành Desktop hiện đại, đẹp mắt với ngôn ngữ thiết kế Glassmorphism, bảo mật tuyệt đối, khởi động app tức thì nhưng vẫn tương thích 100% với hệ sinh thái phần mềm Linux truyền thống.

---

## 🏗️ 2. TizenOS được Build từ những gì? (Technology Stack)

TizenOS được xây dựng từ 8 Phase kiến trúc C/C++ chuẩn mực:

1. **Lõi Hệ thống (Base OS)**: Debian 12 (Bookworm) Stable (`x86_64` & `ARM64`), `systemd`, `linux-kernel 6.x`.
2. **Hiển thị & Đồ họa (Display Layer)**:
   - **Main**: `tizenos-compositor` (Trình quản lý cửa sổ Wayland native phát triển trên `wlroots` + XWayland HW Acceleration).
   - **Fallback**: `tizenos-session-x11` (Standalone Native X11 Session & Window Manager C cho card NVIDIA legacy).
3. **Quản lý GPU & Phần cứng**: Tự động nhận diện GPU (`lspci`/`sysfs`), tự động cấu hình `nvidia-drm.modeset=1` & GBM backend, hỗ trợ Wi-Fi 5/6/7, Bluetooth LDAC/aptX, NVMe APST/TRIM, và Intel VMD / Software RAID `mdadm`.
4. **Bảo mật 7 Lớp (7-Layer Security)**:
   - Kernel **Smack MAC** (Simple Mandatory Access Control).
   - **Cynara Policy Engine** (SQLite Policy DB thời gian thực).
   - **Security Manager** (Phân quyền tiến trình ứng dụng).
   - **PAM Stack** (`pam_unix`, `pam_fprintd`, `pam_u2f`).
   - **Polkit JS Rules** (`50-tizenos.rules`).
   - **Secret Keyring AES-256-GCM** (`libtizen-keyring`).
   - **D-Bus Auth Consent Prompt Daemon** (`tizen-authd`).
5. **Trình Quản lý Gói Kép (Dual Package Manager)**:
   - Định dạng gói **`.tpk` Native** (Tizen Package Format).
   - Tích hợp **APT Bridge** (`deb-bridge.c`) cho phép lệnh `tpkg` cài đặt mượt mà cả gói `.tpk` lẫn kho gói `.deb` Debian 12.
6. **Giao diện Desktop (Desktop Shell)**: GTK4 + `gtk4-layer-shell` với giao diện Kính mờ Glassmorphism sắc xanh Tizen Blue (`#0069B4`).

---

## ⚡ 3. Sự Khác biệt Đột phá của TizenOS (Unique Selling Points)

| Tiêu chí So sánh | **Ubuntu / Fedora / Arch** | **Samsung Tizen OS (Gốc)** | **TizenOS (Debian Edition)** |
|-------------------|----------------------------|----------------------------|------------------------------|
| **Nền tảng Lõi** | Ubuntu / Fedora / Arch Base | Tizen OS (TV / Smartwatch) | **Debian 12 Bookworm Stable Base** |
| **Kho Ứng dụng** | Chỉ dùng `.deb`/`.rpm`/Snap/Flatpak | Chỉ dùng `.tpk` (Rất ít app PC) | **Gói Kép: TPK Native + 60,000+ `.deb` Debian Repo + Flatpak** |
| **Bảo mật Cấp Kernel** | AppArmor / SELinux (Khó cấu hình) | Smack MAC + Cynara | **7 Lớp Bảo mật: Smack MAC + Cynara Policy DB + D-Bus Auth Prompt** |
| **Tốc độ Mở App** | Trung bình (Đọc đĩa trực tiếp) | Nhanh | **Cực nhanh nhờ Tiến trình Pre-fork Pool (`launchpad`)** |
| **Hỗ trợ Card NVIDIA** | Thường bị xé hình / lỗi Wayland | Không hỗ trợ | **Tự động cấu hình GBM / DRM Modeset hoặc tự chuyển X11 Session C** |
| **Bảo vệ Tính toàn vẹn** | Checksum cơ bản | Chữ ký Tizen | **Kiểm định 3 cấp: SHA256/512, GPG Signatures, & Công cụ `signFile` (Ed25519/RSA/ECDSA)** |
| **Bộ Cài đặt (Installer)** | Calamares / Ubiquity | Không có bản PC | **Calamares Universal Installer + `tizenos-installer` GTK4 7-Step (`rsync -aHAX` bảo toàn Smack xattr)** |

---

## 📖 4. Hướng Dẫn Tùy Biến (Customize), Cài Thêm Apps & Tham Gia Phát Triển

Bạn muốn tự build đĩa ISO cho riêng mình, cài thêm các ứng dụng mặc định (VS Code, Telegram, Steam...), tùy biến giao diện hoặc tham gia đóng góp cho TizenOS?

👉 **Xem ngay Hướng Dẫn Chi Tiết tại**: [**`CUSTOMIZATION_GUIDE.md`**](CUSTOMIZATION_GUIDE.md)

---

### 🔨 Lệnh Build Nhanh (Quick Build Commands):

```bash
# 1. Clone repository
git clone https://github.com/nguyenquocanhz/TizenOS.git
cd TizenOS

# 2. Khởi tạo Debian 12 Base RootFS & Calamares Installer
sudo bash build/scripts/bootstrap.sh

# 3. Đóng gói đĩa Hybrid Live ISO (tizenos-live.iso)
sudo bash build/scripts/create-iso.sh
```

Tệp ISO sẽ được xuất ra tại `build/output/tizenos-live.iso` sẵn sàng thử nghiệm trên VMware/VirtualBox hoặc máy thật!
