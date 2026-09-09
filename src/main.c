/** @file main.c @brief Boot ESP32-NUT services and launch read-only UPS tasks. @see wifi-provisioning.h, management.h, ota.h, drivers/espusb.h */
#include "nut_common.h"
#include "nut_version.h"
#include <pwd.h>
#include <grp.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "esp_check.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "esp_vfs_fat.h"
#include "esp_task_wdt.h"
#include "management.h"
#include "management-log.h"
#include "ota.h"
#include "wifi-provisioning.h"

#define TAG PACKAGE
#define NUT_FAT_MAX_FILES 4
#define NUT_DIRECTORY_MODE 0755

static const char NUT_UPSD_USERS_CONTENT[] =
    "# Read-only server: no authenticated NUT users are configured.\n";
static const char NUT_UPSD_CONF_CONTENT[] =
    "ALLOW_NO_DEVICE true\n"
    "LISTEN 0.0.0.0 3493\n"
    "MAXCONN 4\n";
static const char NUT_UPS_CONF_CONTENT[] =
    "[cyberpower]\n"
    "  driver = usbhid-ups\n"
    "  port = auto\n"
    "  desc = \"Read-only USB HID UPS\"\n"
    "  pollonly\n"
    "\n";
static const char NUT_CONF_CONTENT[] = "MODE=netserver\n";

extern int main(int, char **);

extern int drivers_main(int, char **);

static bool write_default_file(const char *path, const char *content)
{
    FILE *file = fopen(path, "wb");
    if (file == NULL)
    {
        perror("fopen");
        ESP_LOGE(TAG, "Failed to open file for writing");
        return false;
    }

    fseek(file, 0, SEEK_END);
    if (ftell(file) == 0)
    {
        fputs(content, file);
        fflush(file);
    }
    fclose(file);
    return true;
}

static bool create_directories(const char *first, const char *second, const char *third)
{
    if (mkdir(first, NUT_DIRECTORY_MODE) < 0 ||
        mkdir(second, NUT_DIRECTORY_MODE) < 0 ||
        (third != NULL && mkdir(third, NUT_DIRECTORY_MODE) < 0))
    {
        if (errno != EEXIST)
        {
            ESP_LOGE(TAG, "Failed to create directory: %s", strerror(errno));
            return false;
        }
    }
    return true;
}

static bool mount_filesystem(
    const char *mount_path,
    const char *partition_label,
    const esp_vfs_fat_mount_config_t *mount_config,
    wl_handle_t *handle)
{
    esp_err_t err = esp_vfs_fat_spiflash_mount_rw_wl(mount_path, partition_label, mount_config, handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount FATFS (%s)", esp_err_to_name(err));
        return false;
    }
    return true;
}

void mountFS(void)
{
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = NUT_FAT_MAX_FILES,                 // Number of files that can be open at a time
        .format_if_mount_failed = true,                // If true, try to format the partition if mount fails
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE, // Size of allocation unit, cluster size.
        .use_one_fat = false,                          // Use only one FAT table (reduce memory usage), but decrease reliability of file system in case of power failure.
    };

    // Handle of the wear levelling library instance
    static wl_handle_t s_var_wl_handle = WL_INVALID_HANDLE;
    static wl_handle_t s_usr_wl_handle = WL_INVALID_HANDLE;

    if (!mount_filesystem("/var", "var", &mount_config, &s_var_wl_handle))
    {
        return;
    }

    if (!create_directories("/var/db", "/var/db/nut", NULL))
    {
        return;
    }

    if (!mount_filesystem("/usr", "usr", &mount_config, &s_usr_wl_handle))
    {
        return;
    }

    if (!create_directories("/usr/local", "/usr/local/etc", "/usr/local/etc/nut"))
    {
        return;
    }

    if (!write_default_file("/usr/local/etc/nut/upsd.users", NUT_UPSD_USERS_CONTENT) ||
        !write_default_file("/usr/local/etc/nut/upsd.conf", NUT_UPSD_CONF_CONTENT) ||
        !write_default_file("/usr/local/etc/nut/ups.conf", NUT_UPS_CONF_CONTENT) ||
        !write_default_file("/usr/local/etc/nut/nut.conf", NUT_CONF_CONTENT))
    {
        return;
    }
}

extern void hidHostInstall(void);
extern bool usb_hid_device_ready(void);

extern void usb_host_lib_task(void *);
extern void class_driver_task(void *);

extern void esp_vfs_af_unix_register(void);

static void nut_main(void *pvParameter)
{
    ESP_LOGI(TAG, "Starting read-only NUT network server");
    optind = 0;
    char *args[2] = {PACKAGE_NAME, "-F"};
    int result = main(2, args);
    ESP_LOGE(TAG, "NUT network server stopped unexpectedly with result %d", result);
    vTaskSuspend(NULL);
}

static void drv_main(void *pvParameter)
{
    while (!usb_hid_device_ready())
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    ESP_LOGI(TAG, "Starting read-only NUT HID driver for USB HID UPS");
    optind = 0;
    char *args[3] = {"usbhid-ups", "-F", "-acyberpower"};
    int result = drivers_main(3, args);
    ESP_LOGE(TAG, "NUT HID driver stopped unexpectedly with result %d", result);
    vTaskSuspend(NULL);
}

void rtos_yield(void)
{
    vTaskDelay(1);
}

void app_main()
{
    management_log_capture_start();
    /* Production operation keeps NUT diagnostics at normal log severity. */
    nut_debug_level = 0;

    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 60000,
        .idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1, // Bitmask of all cores
        .trigger_panic = false,
    };

    ESP_ERROR_CHECK(esp_task_wdt_reconfigure(&twdt_config));

    wifi_provisioning_init();

    mountFS();

    hidHostInstall();

    BaseType_t task_created = xTaskCreatePinnedToCore(
        drv_main, "drv_main", 8192 * 2, NULL, 5, NULL, 0);
    assert(task_created == pdTRUE);

    vTaskDelay(pdMS_TO_TICKS(1000));

    task_created = xTaskCreatePinnedToCore(
        nut_main, "nut_main", 8192 * 2, NULL, 5, NULL, 0);
    assert(task_created == pdTRUE);

    ESP_LOGI(TAG, "Read-only USB discovery, NUT driver, and network server active");
    ota_mark_running_image_valid();

    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

const char *gai_strerror(int ecode)
{
    static const char gai_strerror_msgs[] =
        "Invalid flags\0"
        "Name does not resolve\0"
        "Try again\0"
        "Non-recoverable error\0"
        "Name has no usable address\0"
        "Unrecognized address family or invalid length\0"
        "Unrecognized socket type\0"
        "Unrecognized service\0"
        "Unknown error\0"
        "Out of memory\0"
        "System error\0"
        "Overflow\0"
        "\0Unknown error";

    const char *s;
    for (s = gai_strerror_msgs, ecode++; ecode && *s; ecode++, s++)
        for (; *s; s++)
            ;
    if (!*s)
        s++;
    return s;
}

/*
 * ESP32 Platform Stubs
 * 
 * The following functions are stubs for POSIX functions that are not
 * applicable or implemented on the ESP32 platform. They return success
 * to allow the NUT codebase to compile and run, but do not perform
 * actual operations.
 * 
 * WARNING: Code expecting these functions to enforce security or modify
 * system state will not work as expected on ESP32.
 */

int sigaction(int, const struct sigaction *, struct sigaction *)
{
    // ESP32 stub: Signal handling not implemented
    return 0;
}

_sig_func_ptr signal(int, _sig_func_ptr)
{
    // ESP32 stub: Signal handling not implemented
    return 0;
}

int upsconf_driver = 0;

struct passwd *getpwuid(uid_t)
{
    static struct passwd p = {
        "nut",
        "espdonut",
        0,
        0,
        "NUT User",
        "",
        "/var/lib/nut",
        "/bin/false"};
    return &p;
}

struct passwd *getpwnam(const char *name)
{
    static struct passwd p = {
        "nut",
        "espdonut",
        0,
        0,
        "NUT User",
        "",
        "/var/lib/nut",
        "/bin/false"};
    return &p;
}

struct group *getgrnam(const char *)
{
    static struct group g = {
        "nut",
        "espdonut",
        0,
        NULL};
    return &g;
}

int fchmod(int __fd, mode_t __mode)
{
    return 0;
}

int fchown(int __fildes, uid_t __owner, gid_t __group)
{
    return 0;
}

int chown(const char *__path, uid_t __owner, gid_t __group)
{
    return 0;
}

int chroot(const char *__path)
{
    return 0;
}

int setuid(uid_t __uid)
{
    return 0;
}

int setgid(gid_t __gid)
{
    return 0;
}

int initgroups(const char *, gid_t)
{
    return 0;
}

int seteuid(uid_t __uid)
{
    return 0;
}

uid_t getuid(void)
{
    return 0;
}

uid_t geteuid(void)
{
    return 0;
}

gid_t getgid(void)
{
    return 0;
}

pid_t setsid(void)
{
    return 0;
}

int dup(int __fildes)
{
    return 0;
}

mode_t umask(mode_t __mask)
{
    return 0;
}

void clean_dir(const char *path)
{
    ESP_LOGI(TAG, "Deleting everything in %s:", path);

    DIR *dir = opendir(path);
    if (!dir)
    {
        ESP_LOGE(TAG, "Failed to open directory: %s", strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        char full_path[256] = {0};
        int written = snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        if (written < 0 || written >= sizeof(full_path))
        {
            ESP_LOGE(TAG, "Path too long: %s/%s", path, entry->d_name);
            continue;
        }
        if (entry->d_type == DT_DIR)
            clean_dir(full_path);
        if (remove(full_path) != 0)
        {
            ESP_LOGE(TAG, "Failed to remove %s: %s", full_path, strerror(errno));
        }
    }

    closedir(dir);
}
