# TizenOS Standalone X11 Session & Window Manager

Tài liệu hướng dẫn và mô tả kỹ thuật chi tiết về TizenOS X11 Session cho Debian 12.

## Kiến trúc (Architecture)
- **session-x11**: Launcher chính. Khởi tạo kết nối với X server bằng Xlib. Thiết lập hình nền và khởi động các thành phần giao diện như GTK4 panel.
- **wm-x11**: Window Manager siêu nhẹ, tuân thủ tiêu chuẩn EWMH (Extended Window Manager Hints) và ICCCM. Quản lý việc focus, dịch chuyển, và thay đổi kích thước cửa sổ X11. Hỗ trợ thuộc tính như `_NET_ACTIVE_WINDOW` để thông báo cho các ứng dụng và thanh panel.

## Tính tương thích XWayland (XWayland Compatibility)
- Mặc dù đây là một session X11 độc lập (standalone), kiến trúc mã nguồn của `wm-x11.c` được thiết kế tương đồng để có thể hỗ trợ các fallback triggers một cách mượt mà. 
- Xorg driver được load và tối ưu để có độ trễ thấp thông qua script `xserverrc`. Khi cần thiết, tiến trình này có thể đóng vai trò làm máy chủ trung gian tương thích với các ứng dụng cũ.

## Cấu hình (Configuration)
1. **xinitrc**: Thiết lập con trỏ chuột mặc định, nạp cấu hình `Xresources` và bắt đầu phiên TizenOS.
2. **xserverrc**: Ngăn chặn Xorg mở port TCP (`-nolisten tcp`) giúp tăng cường bảo mật.

## Đóng gói (Packaging)
- Cung cấp sẵn các file cho `dpkg-buildpackage` trong thư mục `debian/` để dễ dàng tạo tệp `.deb` cho Debian 12.

## Dịch và cài đặt (Build & Install)
```bash
mkdir build && cd build
cmake ..
make
sudo make install
```
