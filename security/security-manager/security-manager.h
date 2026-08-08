#ifndef SECURITY_MANAGER_H
#define SECURITY_MANAGER_H

// Quản lý ứng dụng: Gắn kết App ID, thiết lập label Smack, đăng ký privileges
int install_app_security(const char *app_id);
int set_process_credentials(int pid, const char *app_id);

#endif
