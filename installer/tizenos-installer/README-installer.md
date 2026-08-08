# Tài liệu Hướng dẫn Cài đặt TizenOS (TizenOS Installer)

Chào mừng bạn đến với bộ cài đặt **TizenOS Installer**, được xây dựng dựa trên giao diện GTK4 cho hệ điều hành TizenOS (Debian 12 base). Bộ cài đặt bao gồm 7 bước đơn giản và trực quan:

## 7 Bước Cài Đặt (7-Step Wizard)

1. **Bước 1: Welcome / Lang (Chào mừng & Ngôn ngữ)**
   - Lựa chọn ngôn ngữ hiển thị cho bộ cài đặt và hệ điều hành sau khi cài đặt.

2. **Bước 2: Keyboard (Bố cục Bàn phím)**
   - Chọn sơ đồ bàn phím (Keyboard Layout) phù hợp với người dùng.

3. **Bước 3: Timezone (Múi giờ)**
   - Cấu hình múi giờ (Timezone) tự động hoặc thủ công dựa trên vị trí địa lý.

4. **Bước 4: Disk Partitioning (Phân vùng Ổ đĩa)**
   - Cho phép xóa toàn bộ dữ liệu ổ đĩa (Full disk erase).
   - Thiết lập mã hóa toàn bộ ổ đĩa (LUKS encryption).
   - Tạo phân vùng EFI System Partition (ESP) tại `/boot/efi`.
   - Tạo phân vùng Root (rootfs) định dạng `ext4` gắn tại `/`.
   - Sử dụng các công cụ mạnh mẽ như `parted`, `sfdisk`, và `mkfs`.

5. **Bước 5: User Setup (Thiết lập Người dùng)**
   - Tạo tài khoản người dùng chính.
   - Băm mật khẩu (Password hashing) an toàn.
   - Thiết lập Tên máy tính (Hostname).
   - Tùy chọn tự động đăng nhập (Autologin).
   - Thêm người dùng vào các nhóm quyền quản trị (`sudo`, `wheel`).

6. **Bước 6: Third-Party Software (Phần mềm Bên thứ ba)**
   - Cung cấp tùy chọn cài đặt driver đồ họa độc quyền (NVIDIA proprietary drivers).
   - Cài đặt các Codec đa phương tiện (FFmpeg, GStreamer restricted).
   - Tích hợp Flatpak và Web apps sẵn sàng sử dụng.

7. **Bước 7: Installation Progress (Tiến trình Cài đặt)**
   - Sao chép toàn bộ hệ thống tệp từ Squashfs (`/run/live/rootfs`) sang phân vùng đích.
   - Tự động sinh file cấu hình `/etc/fstab` bằng UUID.
   - Cài đặt bootloader GRUB2 hỗ trợ cả UEFI và BIOS/MBR (Legacy).
   - Hoàn tất cài đặt (Chroot finalization) và khởi động lại.

## Cấu trúc mã nguồn

- `src/installer-ui.c`: Chứa logic giao diện người dùng GTK4.
- `src/partman.c`: Quản lý phân vùng, xóa đĩa, và định dạng.
- `src/user-setup.c`: Thiết lập tài khoản và quyền.
- `src/third-party.c`: Quản lý cài đặt phần mềm ngoài (NVIDIA, Codecs).
- `src/target-install.c`: Xử lý việc sao chép rootfs, cấu hình GRUB và fstab.
- `debian/`: Chứa các tệp đóng gói tiêu chuẩn cho Debian/TizenOS.
