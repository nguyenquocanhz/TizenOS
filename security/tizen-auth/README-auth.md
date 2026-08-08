# Kiến Trúc Bảo Mật & Xác Thực TizenOS (TizenOS Authentication Framework)

Hệ thống bảo mật xác thực của TizenOS được thiết kế theo tiêu chuẩn an toàn cao cho môi trường Debian 12, hỗ trợ xác thực nhiều yếu tố, quản lý quyền hệ thống và lưu trữ thông tin bí mật an toàn.

## 1. Chuỗi Xác Thực PAM (Pluggable Authentication Modules)
Cấu hình PAM (`config/pam/`) được thiết kế để ưu tiên tính tiện dụng và an toàn:
- **`tizen-auth`**: Tệp trung tâm khai báo luồng xác thực. Ưu tiên kiểm tra sinh trắc học (vân tay qua `pam_fprintd.so`) và khóa bảo mật FIDO2 (`pam_u2f.so`). Nếu cả hai không khả dụng, hệ thống chuyển về mật khẩu thông thường (`pam_unix.so`).
- **`greetd`**: Display manager (ví dụ Greetd) tái sử dụng luồng xác thực `tizen-auth` cho quá trình đăng nhập giao diện đồ họa (GUI).
- **`tizen-security`**: Thiết lập module trong pha `session` nhằm áp đặt các gán nhãn bắt buộc. `pam_cynara.so` sẽ thiết lập môi trường chính sách quyền Cynara cho session, và `pam_smack.so` gán nhãn Smack MAC (Mandatory Access Control) bảo vệ không gian bộ nhớ tiến trình.

## 2. Quản Lý Quyền Bằng Polkit & Cynara
- **Polkit Rules (`50-tizenos.rules`)**: Cấp quyền ưu tiên cho thành viên nhóm `wheel` (Quản trị viên) trong các thao tác thay đổi mạng (NetworkManager), nguồn điện (UPower) mà không cần nhập mật khẩu liên tục.
- **Polkit Actions (`org.tizen.security.policy`)**: Định nghĩa các action tùy chỉnh của hệ thống. Ví dụ: `org.tizen.security.policy.install` yêu cầu xác thực (`auth_admin`) khi muốn cài đặt các gói dpkg/apt.

## 3. Trình Nền Xác Thực (Auth Daemon)
**`auth-daemon.c`** chạy ngầm dưới vai trò D-Bus service (`org.tizen.Auth`).
- Xử lý các tín hiệu D-Bus yêu cầu xác thực từ ứng dụng.
- Kích hoạt cửa sổ prompt yêu cầu người dùng xác nhận cấp quyền (Permission Consent) đối với các hành động bị giới hạn bởi Cynara.

## 4. Trình Quản Lý Chứng Chỉ (Tizen Keyring)
- **`libtizen-keyring` (`keyring.c` / `tizen-keyring.h`)**: Cung cấp API C cho phép các ứng dụng mã hóa và lưu trữ credential/token an toàn.
- Dữ liệu được mã hóa ở mức chuẩn AES-256-GCM.
- Tương thích ngược với Secret Service API (freedesktop.org), cho phép tích hợp dễ dàng với GNOME Keyring hoặc KWallet.
