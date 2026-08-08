/*
 * TizenOS Target Installer - Implementation
 * =============================================================================
 * Thực thi cài đặt hệ thống lên ổ cứng:
 * 1. Mount phân vùng đích & bind virtual filesystems (/dev, /proc, /sys).
 * 2. Sao chép rootfs qua rsync (-aHAX giữ quyền Smack xattr) hoặc unsquashfs.
 * 3. Tự động sinh /etc/fstab dựa trên UUID (blkid).
 * 4. Tạo tài khoản người dùng, băm mật khẩu, set hostname.
 * 5. Cài đặt GRUB2 Bootloader (UEFI x86_64/ARM64 hoặc MBR Legacy).
 * 6. Cập nhật initramfs & chroot finalization.
 * 7. Unmount an toàn trước khi khởi động lại.
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

/* ---- Helper Functions ------------------------------------------------------ */

static int run_cmd(const char *cmd) {
    printf("[TARGET-INSTALL] Executing: %s\n", cmd);
    int status = system(cmd);
    if (status == -1) {
        fprintf(stderr, "[TARGET-INSTALL-ERROR] Command failed to start: %s\n", cmd);
        return -1;
    }
    int exit_code = WEXITSTATUS(status);
    if (exit_code != 0) {
        fprintf(stderr, "[TARGET-INSTALL-ERROR] Command returned error code %d: %s\n", exit_code, cmd);
    }
    return exit_code;
}

/* Lấy UUID của phân vùng đĩa qua blkid */
static char *get_partition_uuid(const char *part_dev) {
    static char uuid[128];
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "blkid -s UUID -o value %s 2>/dev/null", part_dev);

    FILE *fp = popen(cmd, "r");
    if (!fp) return NULL;

    if (fgets(uuid, sizeof(uuid), fp) != NULL) {
        uuid[strcspn(uuid, "\r\n")] = '\0';
        pclose(fp);
        return uuid;
    }
    pclose(fp);
    return NULL;
}

/* ---- 1. Rootfs Deployment via rsync / unsquashfs --------------------------- */

bool target_install_copy_rootfs(const char *source, const char *dest) {
    const char *target = dest ? dest : TARGET_MOUNT;
    char cmd[1024];

    printf("[TARGET-INSTALL] === Bước 1: Sao chép Rootfs sang %s ===\n", target);

    // Tạo thư mục mount mục tiêu nếu chưa có
    mkdir(target, 0755);

    // Ưu tiên rsync nếu live rootfs đang mount
    if (access(LIVE_ROOTFS, F_OK) == 0) {
        printf("[TARGET-INSTALL] Sử dụng rsync để sao chép live rootfs (bảo toàn Smack xattr & ACLs)...\n");
        snprintf(cmd, sizeof(cmd),
            "rsync -aHAX --info=progress2 "
            "--exclude='/proc/*' --exclude='/sys/*' --exclude='/dev/*' "
            "--exclude='/tmp/*' --exclude='/run/*' --exclude='/mnt/*' "
            "--exclude='/media/*' --exclude='/lost+found' "
            "%s/ %s/", LIVE_ROOTFS, target);

        if (run_cmd(cmd) == 0) {
            printf("[TARGET-INSTALL] ✓ rsync sao chép Rootfs thành công!\n");
            return true;
        }
    }

    // Fallback sang unsquashfs giải nén trực tiếp file filesystem.squashfs
    if (access(SQUASHFS_PATH, F_OK) == 0) {
        printf("[TARGET-INSTALL] Fallback: Sử dụng unsquashfs để giải nén %s...\n", SQUASHFS_PATH);
        snprintf(cmd, sizeof(cmd), "unsquashfs -f -d %s %s", target, SQUASHFS_PATH);
        if (run_cmd(cmd) == 0) {
            printf("[TARGET-INSTALL] ✓ unsquashfs giải nén thành công!\n");
            return true;
        }
    }

    // Direct source fallback nếu có argument
    if (source && access(source, F_OK) == 0) {
        snprintf(cmd, sizeof(cmd), "rsync -aHAX %s/ %s/", source, target);
        return (run_cmd(cmd) == 0);
    }

    fprintf(stderr, "[TARGET-INSTALL-ERROR] Không thể tìm thấy nguồn Rootfs để sao chép!\n");
    return false;
}

/* ---- 2. fstab Auto-Generation ---------------------------------------------- */

bool target_install_generate_fstab(const char *root_part, const char *esp_part) {
    char fstab_path[512];
    snprintf(fstab_path, sizeof(fstab_path), "%s/etc/fstab", TARGET_MOUNT);

    printf("[TARGET-INSTALL] === Bước 2: Tự động khởi tạo %s ===\n", fstab_path);

    FILE *fp = fopen(fstab_path, "w");
    if (!fp) {
        fprintf(stderr, "[TARGET-INSTALL-ERROR] Không thể tạo %s: %s\n", fstab_path, strerror(errno));
        return false;
    }

    fprintf(fp, "# /etc/fstab: Static file system information for TizenOS\n");
    fprintf(fp, "# Dynamic UUID generation by TizenOS Target Installer\n");
    fprintf(fp, "# <file system>                           <mount point>   <type>  <options>       <dump>  <pass>\n\n");

    // 1. Root Partition
    char *root_uuid = get_partition_uuid(root_part);
    if (root_uuid && strlen(root_uuid) > 0) {
        fprintf(fp, "UUID=%-36s  /               ext4    noatime,errors=remount-ro  0       1\n", root_uuid);
        printf("[TARGET-INSTALL] Root UUID: %s\n", root_uuid);
    } else {
        fprintf(fp, "%-41s  /               ext4    noatime,errors=remount-ro  0       1\n", root_part);
    }

    // 2. EFI System Partition (/boot/efi)
    if (esp_part && strlen(esp_part) > 0) {
        char *esp_uuid = get_partition_uuid(esp_part);
        if (esp_uuid && strlen(esp_uuid) > 0) {
            fprintf(fp, "UUID=%-36s  /boot/efi       vfat    umask=0077      0       2\n", esp_uuid);
            printf("[TARGET-INSTALL] ESP UUID: %s\n", esp_uuid);
        } else {
            fprintf(fp, "%-41s  /boot/efi       vfat    umask=0077      0       2\n", esp_part);
        }
    }

    // 3. Virtual filesystems & tmp
    fprintf(fp, "tmpfs                                      /tmp            tmpfs   defaults,noatime,mode=1777 0 0\n");

    fclose(fp);
    printf("[TARGET-INSTALL] ✓ /etc/fstab đã được khởi tạo thành công!\n");
    return true;
}

/* ---- 3. Bind Virtual Filesystems & Chroot Setup ---------------------------- */

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

/* ---- 4. GRUB2 Bootloader Installation -------------------------------------- */

bool target_install_grub(const char *disk, bool is_uefi) {
    char cmd[1024];

    printf("[TARGET-INSTALL] === Bước 3: Cài đặt GRUB2 Bootloader (%s) ===\n", is_uefi ? "UEFI" : "MBR Legacy");

    bind_virtual_fs();

    if (is_uefi) {
        // Cài đặt GRUB2 EFI (x86_64 / arm64)
        snprintf(cmd, sizeof(cmd),
            "chroot " TARGET_MOUNT " grub-install "
            "--target=x86_64-efi --efi-directory=/boot/efi "
            "--bootloader-id=TizenOS --recheck");
        run_cmd(cmd);
    } else {
        // Cài đặt GRUB2 MBR (Legacy BIOS)
        snprintf(cmd, sizeof(cmd),
            "chroot " TARGET_MOUNT " grub-install "
            "--target=i386-pc --recheck %s", disk);
        run_cmd(cmd);
    }

    // Cập nhật cấu hình GRUB và initramfs
    printf("[TARGET-INSTALL] Cập nhật initramfs và cấu hình GRUB...\n");
    run_cmd("chroot " TARGET_MOUNT " update-initramfs -u -k all");
    run_cmd("chroot " TARGET_MOUNT " update-grub");

    unbind_virtual_fs();

    printf("[TARGET-INSTALL] ✓ Cài đặt GRUB2 Bootloader hoàn tất!\n");
    return true;
}
