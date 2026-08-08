#ifndef CYNARA_ADMIN_H
#define CYNARA_ADMIN_H

// API Admin cho Cynara: Thiết lập chính sách (ALLOW/DENY)
int cynara_set_policy(const char *client_label, const char *user, const char *privilege, int result);

#endif
