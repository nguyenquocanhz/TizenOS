# Tài liệu kỹ thuật: Archive Engine & Virtual ISO Mount (TizenOS)

## 1. Engine Nén / Giải Nén
Sử dụng thư viện `libarchive` để thao tác với các định dạng phổ biến như:
- `.zip`, `.tar.gz`, `.tar.xz`, `.tar.zst`, `.7z`, `.rar`
- Hỗ trợ thêm cả các package như `.deb` và `.tpk` (Tizen package).

API cơ bản:
`extract_archive(archive_path, dest_dir)` sử dụng vòng lặp duyệt qua các Entry và ghi vào đĩa, bảo tồn quyền và timestamp của file.

## 2. ISO Mount Engine
Chịu trách nhiệm gắn kết ổ đĩa ảo (định dạng ISO, IMG, NRG, BIN).

### udisks2 và losetup
Sử dụng lệnh `udisksctl loop-setup` để tạo loop device một cách an toàn mà không cần phân quyền gốc.
Nếu thất bại, sử dụng fallback lệnh `mount -o loop` để mount trực tiếp bằng loop device (`losetup`).

### CLI Tool
- `tizenos-mount-iso mount <file> <point>`
- `tizenos-mount-iso unmount <point>`

## 3. Tích hợp Trình Quản Lý Tệp
Mã nguồn ở `shell/file-manager` tích hợp các action "Extract Here" và "Mount Virtual Drive" sử dụng các API trong Archive và ISO Mount Engine.
