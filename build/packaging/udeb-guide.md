# Hướng dẫn về Udeb Packages trong TizenOS

## Udebs là gì?
Udebs (micro-debs) là các gói Debian đặc biệt được thiết kế để sử dụng trong `debian-installer`. Chúng có kích thước siêu nhỏ, loại bỏ những thành phần không cần thiết cho quá trình cài đặt.

## Sự khác biệt giữa .deb và .udeb
- **Không có tài liệu**: Không chứa các file tài liệu (docs)
- **Không có man pages**: Hướng dẫn sử dụng bị loại bỏ
- **Không có changelogs**: Lịch sử thay đổi không được bao gồm
- **Dependencies tối thiểu**: Chỉ phụ thuộc vào các gói thực sự thiết yếu

## TizenOS sử dụng udebs như thế nào?
Trong TizenOS, udebs được sử dụng để thực hiện việc phát hiện GPU và thiết lập driver cơ bản ngay trong quá trình cài đặt (debian-installer). Điều này giúp hệ điều hành nhận diện phần cứng đồ họa (Intel, AMD, NVIDIA) sớm nhất có thể.

## Cách tạo một gói udeb
Để tạo một udeb, bạn cần thêm các khai báo sau vào file `debian/control`:
- `Package-Type: udeb`
- `Section: debian-installer`

Ví dụ:
```debian-control
Package: tizenos-installer-udeb
Package-Type: udeb
Architecture: all
Section: debian-installer
```

## Cách kiểm tra udebs
Bạn có thể kiểm thử các gói udeb bằng cách:
1. Đóng gói lại `mini.iso` với udeb của bạn.
2. Chạy thử bằng `QEMU` để kiểm tra trình cài đặt có nhận diện đúng gói không.

## Cấu hình reprepro cho Udebs
Trong file cấu hình `conf/distributions`, bạn cần khai báo:
`UDebComponents: main`
để APT repository có khả năng quản lý thư mục `debian-installer/` dành riêng cho udebs.
