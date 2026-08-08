#ifndef CYNARA_CLIENT_H
#define CYNARA_CLIENT_H

// API Client cho Cynara: Kiểm tra quyền (privilege) của ứng dụng hoặc user
int cynara_check_privilege(const char *client_label, const char *user, const char *privilege);

#endif
