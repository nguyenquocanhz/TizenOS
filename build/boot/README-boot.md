# Tài liệu cơ chế khởi động kép (Boot Mechanisms) cho TizenOS

Tài liệu này giải thích cấu trúc khởi động (Dual Boot) hỗ trợ đồng thời UEFI và Legacy MBR/BIOS cho TizenOS Live ISO và Raw disk.

## 1. Cơ chế Boot (Boot Mechanisms)

### Legacy MBR / BIOS (CSM)
- **ISOLINUX**: Khi khởi động từ CD/DVD hoặc USB ở chế độ Legacy (CSM), BIOS của hệ thống sẽ đọc Master Boot Record (MBR).
- Mã lệnh bootstrap của MBR (được xorriso chèn vào thông qua tham số `-isohybrid-mbr`) sẽ thực thi quá trình nạp `isolinux.bin`.
- File cấu hình `isolinux.cfg` hiển thị giao diện đồ họa (vesamenu) giúp người dùng chọn tùy chọn khởi động.

### UEFI (x86_64 & ARM64)
- Khi phần cứng sử dụng firmware UEFI, nó bỏ qua MBR và quét tìm phân vùng hệ thống EFI (ESP).
- Trong Hybrid ISO, `xorriso` định dạng phân vùng EFI (thông qua El Torito alternate boot) chứa GRUB EFI loader (`boot/grub/efi.img`).
- GRUB EFI sau đó được khởi chạy và tải cấu hình từ `grub-uefi.cfg`.
- Hỗ trợ Secure Boot được bao gồm bằng cách sử dụng các gói `grub-efi-*-signed` và `shim` của Debian.

## 2. Ý nghĩa cấu trúc thư mục Boot

- `grub-uefi.cfg`: Cấu hình menu GRUB riêng biệt cho UEFI Boot, nạp đồ họa gfxterm.
- `grub-bios.cfg`: Cấu hình menu GRUB fallback cho BIOS/MBR (sử dụng với grub-pc).
- `loopback.cfg`: File cấu hình tạo độ tương thích cao cho các công cụ như Ventoy / GRUB2 loopback. Cho phép boot trực tiếp file ISO mà không cần xả nén dữ liệu.
- `isolinux/isolinux.cfg`: Cấu hình menu ISOLINUX truyền thống cho MBR.

## 3. Tìm hiểu cờ Xorriso (xorriso flags)

Kịch bản `create-iso.sh` thực thi việc đóng gói thông qua các cờ quan trọng:
- `-isohybrid-mbr`: Chèn đoạn mã máy khởi động MBR vào sector 0 của ảnh ISO.
- `-b isolinux/isolinux.bin -c isolinux/boot.cat`: Cấu hình chuẩn khởi động El Torito cho BIOS sử dụng ISOLINUX.
- `-eltorito-alt-boot`: Khai báo bắt đầu phần khởi động thứ hai dành cho UEFI.
- `-e boot/grub/efi.img -isohybrid-gpt-basdat`: Chỉ định file ảnh floppy chứa bộ nạp UEFI (GRUB) và tạo bảng phân vùng GPT để firmware nhận dạng được như một phân vùng Basic Data.

## 4. Hướng dẫn Test với QEMU

Để giả lập và kiểm tra quá trình khởi động thành công, sử dụng QEMU:

### Kiểm tra Legacy BIOS/MBR
Khởi chạy mặc định với BIOS:
```bash
qemu-system-x86_64 -m 2048 -cdrom tizenos-live.iso -boot d
```

### Kiểm tra UEFI
Yêu cầu phải có tập tin OVMF (UEFI firmware image):
```bash
qemu-system-x86_64 -m 2048 -bios /usr/share/ovmf/OVMF.fd -cdrom tizenos-live.iso -boot d
```
