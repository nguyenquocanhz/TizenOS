#ifndef SMACK_UTIL_H
#define SMACK_UTIL_H

// Khai báo các hàm tiện ích cho việc quản lý Smack MAC
// Smack là hệ thống Mandatory Access Control trong nhân Linux

int load_smack_rules(const char *rule_file);
int apply_smack_label(int fd, const char *label);

#endif // SMACK_UTIL_H
