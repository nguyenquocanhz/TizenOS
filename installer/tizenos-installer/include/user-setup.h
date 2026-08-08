#ifndef USER_SETUP_H
#define USER_SETUP_H

#include <stdbool.h>

// Thiết lập tài khoản người dùng
bool user_setup_create_account(const char *username, const char *password, const char *fullname);

// Thiết lập hostname
bool user_setup_set_hostname(const char *hostname);

// Bật tự động đăng nhập (autologin)
bool user_setup_enable_autologin(const char *username);

#endif
