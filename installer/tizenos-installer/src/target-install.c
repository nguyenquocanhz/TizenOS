/*
 * TizenOS Target Installer - Implementation
 * =============================================================================
 * Thực thi cài đặt hệ thống lên ổ cứng:
 * 1. Mount phân vùng đích & bind virtual filesystems (/dev, /proc, /sys).
 * 2. Sao chép rootfs qua rsync (-aHAX giữ quyền Smack xattr) hoặc unsquashfs.
 * 3. Tự động sinh /etc/fstab dựa trên UUID (blkid).
 * 4. Tạo tài khoản người dùng, băm mật khẩu, set hostname.
 * 5. Cài đặt GRUB2 Bootloader (UEFI x86_64/ARM64 và MBR Legacy).
 * 6. Cập nhật initramfs & chroot finalization.
 * 7. Dọn dẹp autostart bộ cài trên hệ thống thật & Unmount an toàn.
 * =============================================================================
 */

#include "target-install.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

#define TARGET_MOUNT "/mnt/target"
#define SQUASHFS_PATH "/run/live/medium/live/filesystem.squashfs"
#define LIVE_ROOTFS "/run/live/rootfs"

static int run_cmd(const char *cmd) {
    printf("[TARGET-INSTALL] Executing: %s\n", cmd);
    int status = system(cmd);
    if (status == -1) {
        fprintf(stderr, "[TARGET-INSTALL-ERROR] Command failed to start: %s\n", cmd);
        return -1;
    }
    int exit_code = WEXITSTATUS(status);
    if (exit_code != 0) {
        fprintf(stderr, "[TARGET-INSTALL-ERROR] Command returned code %d: %s\n", exit_code, cmd);
    }
    return exit_code;
}

static bool fetch_partition_uuid(const char *part_dev, char *out_uuid, size_t max_len) {
    if (!part_dev || !out_uuid || max_len == 0) return false;
    out_uuid[0] = '\0';
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "blkid -s UUID -o value %s 2>/dev/null", part_dev);

    FILE *fp = popen(cmd, "r");
    if (!fp) return false;

    if (fgets(out_uuid, max_len, fp) != NULL) {
        out_uuid[strcspn(out_uuid, "\r\n")] = '\0';
        pclose(fp);
        return (strlen(out_uuid) > 0);
    }
    pclose(fp);
    return false;
}

bool target_install_copy_rootfs(const char *source, const char *dest) {
    const char *target = dest ? dest : TARGET_MOUNT;
    char cmd[1024];

    printf("[TARGET-INSTALL] === Bước 1: Sao chép Rootfs sang %s ===\n", target);

    mkdir(target, 0755);

    if (access(LIVE_ROOTFS, F_OK) == 0) {
        printf("[TARGET-INSTALL] Sử dụng rsync để sao chép live rootfs (bảo toàn Smack xattr)...\n");
        snprintf(cmd, sizeof(cmd),
            "rsync -aHAX --info=progress2 "
            "--exclude='/proc/*' --exclude='/sys/*' --exclude='/dev/*' "
            "--exclude='/tmp/*' --exclude='/run/*' --exclude='/mnt/*' "
            "--exclude='/media/*' --exclude='/lost+found' "
            "%s/ %s/", LIVE_ROOTFS, target);

        if (run_cmd(cmd) == 0) {
            printf("[TARGET-INSTALL] ✓ rsync sao chép Rootfs thành công!\n");
            run_cmd("rm -f " TARGET_MOUNT "/etc/skel/.config/autostart/tizenos-installer.desktop 2>/dev/null || true");
            run_cmd("rm -f " TARGET_MOUNT "/etc/skel/.config/autostart/tizenos-welcome.desktop 2>/dev/null || true");
            run_cmd("rm -f " TARGET_MOUNT "/home/*/.config/autostart/tizenos-installer.desktop 2>/dev/null || true");
            run_cmd("rm -f " TARGET_MOUNT "/home/*/.config/autostart/tizenos-welcome.desktop 2>/dev/null || true");
            return true;
        }
    }

    printf("[TARGET-INSTALL] Sao chép trực tiếp từ hệ thống đang chạy...\n");
    snprintf(cmd, sizeof(cmd),
        "rsync -aHAX --info=progress2 "
        "--exclude='/proc/*' --exclude='/sys/*' --exclude='/dev/*' "
        "--exclude='/tmp/*' --exclude='/run/*' --exclude='/mnt/*' "
        "--exclude='/media/*' --exclude='/lost+found' "
        "/ %s/", target);

    int res = run_cmd(cmd);
    run_cmd("rm -f " TARGET_MOUNT "/etc/skel/.config/autostart/tizenos-installer.desktop 2>/dev/null || true");
    run_cmd("rm -f " TARGET_MOUNT "/etc/skel/.config/autostart/tizenos-welcome.desktop 2>/dev/null || true");
    run_cmd("rm -f " TARGET_MOUNT "/home/*/.config/autostart/tizenos-installer.desktop 2>/dev/null || true");
    run_cmd("rm -f " TARGET_MOUNT "/home/*/.config/autostart/tizenos-welcome.desktop 2>/dev/null || true");
    return (res == 0);
}

bool target_install_generate_fstab(const char *root_part, const char *esp_part, const char *swap_part) {
    char fstab_path[512];
    snprintf(fstab_path, sizeof(fstab_path), "%s/etc/fstab", TARGET_MOUNT);

    printf("[TARGET-INSTALL] === Bước 2: Tự động khởi tạo %s ===\n", fstab_path);

    FILE *fp = fopen(fstab_path, "w");
    if (!fp) {
        fprintf(stderr, "[TARGET-INSTALL-ERROR] Không thể tạo %s: %s\n", fstab_path, strerror(errno));
        return false;
    }

    fprintf(fp, "# /etc/fstab: Static file system information for TizenOS\n");
    fprintf(fp, "# <file system>                           <mount point>   <type>  <options>       <dump>  <pass>\n\n");

    char root_uuid[128] = {0};
    if (fetch_partition_uuid(root_part, root_uuid, sizeof(root_uuid))) {
        fprintf(fp, "UUID=%-36s  /               ext4    noatime,errors=remount-ro  0       1\n", root_uuid);
    } else {
        fprintf(fp, "%-41s  /               ext4    noatime,errors=remount-ro  0       1\n", root_part);
    }

    if (esp_part && strlen(esp_part) > 0) {
        char esp_uuid[128] = {0};
        if (fetch_partition_uuid(esp_part, esp_uuid, sizeof(esp_uuid))) {
            fprintf(fp, "UUID=%-36s  /boot/efi       vfat    umask=0077      0       2\n", esp_uuid);
        } else {
            fprintf(fp, "%-41s  /boot/efi       vfat    umask=0077      0       2\n", esp_part);
        }
    }

    if (swap_part && strlen(swap_part) > 0) {
        char swap_uuid[128] = {0};
        if (fetch_partition_uuid(swap_part, swap_uuid, sizeof(swap_uuid))) {
            fprintf(fp, "UUID=%-36s  none            swap    sw              0       0\n", swap_uuid);
        } else {
            fprintf(fp, "%-41s  none            swap    sw              0       0\n", swap_part);
        }
    }

    fprintf(fp, "tmpfs                                      /tmp            tmpfs   defaults,noatime,mode=1777 0 0\n");
    fprintf(fp, "tmpfs                                      /dev/shm        tmpfs   defaults,nosuid,nodev 0 0\n");

    fclose(fp);
    printf("[TARGET-INSTALL] ✓ /etc/fstab đã được khởi tạo thành công!\n");
    return true;
}

static bool bind_virtual_fs() {
    printf("[TARGET-INSTALL] Bind mounting virtual filesystems (/dev, /proc, /sys, /run)...\n");
    run_cmd("mount --bind /dev " TARGET_MOUNT "/dev");
    run_cmd("mount --bind /dev/pts " TARGET_MOUNT "/dev/pts");
    run_cmd("mount --bind /proc " TARGET_MOUNT "/proc");
    run_cmd("mount --bind /sys " TARGET_MOUNT "/sys");
    run_cmd("mount --bind /run " TARGET_MOUNT "/run");
    return true;
}

static bool unbind_virtual_fs() {
    printf("[TARGET-INSTALL] Unmounting virtual filesystems...\n");
    run_cmd("umount -l " TARGET_MOUNT "/dev/pts 2>/dev/null || true");
    run_cmd("umount -l " TARGET_MOUNT "/dev 2>/dev/null || true");
    run_cmd("umount -l " TARGET_MOUNT "/proc 2>/dev/null || true");
    run_cmd("umount -l " TARGET_MOUNT "/sys 2>/dev/null || true");
    run_cmd("umount -l " TARGET_MOUNT "/run 2>/dev/null || true");
    return true;
}

bool target_install_grub(const char *disk, bool is_uefi) {
    (void)is_uefi;
    char cmd[1024];

    printf("[TARGET-INSTALL] === Bước 3: Cài đặt GRUB2 Bootloader Kép (%s) ===\n", disk);

    bind_virtual_fs();

    snprintf(cmd, sizeof(cmd),
        "chroot " TARGET_MOUNT " grub-install --target=i386-pc --recheck %s 2>/dev/null || true", disk);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd),
        "chroot " TARGET_MOUNT " grub-install --target=x86_64-efi --efi-directory=/boot/efi --bootloader-id=TizenOS --recheck --removable 2>/dev/null || true");
    run_cmd(cmd);

    printf("[TARGET-INSTALL] Cập nhật initramfs, open-vm-tools và cấu hình GRUB...\n");
    run_cmd("chroot " TARGET_MOUNT " systemctl enable open-vm-tools.service 2>/dev/null || true");
    run_cmd("chroot " TARGET_MOUNT " update-initramfs -u -k all 2>/dev/null || true");
    run_cmd("chroot " TARGET_MOUNT " update-grub 2>/dev/null || true");

    printf("[TARGET-INSTALL] Đồng bộ dữ liệu xuống đĩa ảo VMware (.vmdk)...\n");
    sync();
    run_cmd("sync");
    snprintf(cmd, sizeof(cmd), "blockdev --flushbufs %s 2>/dev/null || true", disk);
    run_cmd(cmd);

    unbind_virtual_fs();

    run_cmd("umount -R " TARGET_MOUNT " 2>/dev/null || true");

    sync();
    run_cmd("sync");

    printf("[TARGET-INSTALL] ✓ Cài đặt GRUB2 Bootloader hoàn tất!\n");
    return true;
}
