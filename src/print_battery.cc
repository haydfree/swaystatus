#define _DEFAULT_SOURCE /* For macro constants of struct dirent::d_type and struct timespec */
#ifndef  _GNU_SOURCE
# define _GNU_SOURCE     /* For strchrnul */
#endif

#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <err.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>     /* For O_RDONLY */
#include <unistd.h>    /* For close, lseek and fstat */

#include <string_view>

#include "utility.h"
#include "printer.hpp"
#include "Conditional.hpp"
#include "LazyEval.hpp"
#include "print_battery.h"

using swaystatus::Conditional;
using swaystatus::LazyEval;

static const char * const path = "/sys/class/power_supply/";

static const char *format;

/**
 * If battery not found, battery_device is left unchanged, which by default is NULL
 *
 * Since having multiple batteries for laptop/desktop/workstation doesn't make much sense,
 * swaystatus now only supports single battery device.
 */
static const char *battery_device;
static int uevent_fd;

static char *buffer;
static size_t buffer_sz;

extern "C" {
int add_battery(int path_fd, const char *device)
{
    battery_device = strdup_checked(device);

    size_t device_name_len = strlen(battery_device);
    char relative_path[device_name_len + 1 + sizeof("uevent")];

    memcpy(relative_path, device, device_name_len);
    relative_path[device_name_len] = '/';
    memcpy(relative_path + device_name_len + 1, "type", sizeof("type"));

    int fd = openat_checked(path, path_fd, relative_path, O_RDONLY);

    char type[sizeof("Battery")];
    ssize_t bytes = readall(fd, type, sizeof(type) - 1);
    if (bytes < 0)
        err(1, "%s on %s%s failed", "readall", path, buffer);
    if (bytes == 0)
        errx(1, "%s on %s%s failed", "Assumption", path, buffer);
    type[bytes] = '\0';

    close(fd);

    if (strncmp("Battery", type, sizeof("Battery") - 1) != 0)
        return 0;

    memcpy(relative_path + device_name_len + 1, "uevent", sizeof("uevent"));
    uevent_fd = openat_checked(path, path_fd, relative_path, O_RDONLY);

    return 1;
}
void init_battery_monitor(const char *format_str)
{
    format = format_str;

    DIR *dir = opendir(path);
    if (!dir)
        err(1, "%s on %s failed", "opendir", path);

    const int path_fd = dirfd(dir);

    errno = 0;
    for (struct dirent *ent; (ent = readdir(dir)); errno = 0) {
        switch (ent->d_type) {
            case DT_UNKNOWN:
            case DT_LNK:
                if (!isdir(path, path_fd, ent->d_name))
                    break;

            case DT_DIR:
                if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
                    if (add_battery(path_fd, ent->d_name))
                        goto done;
                }

            default:
                break;
        }
    }

    if (errno != 0)
        err(1, "%s on %s failed", "readdir", path);

done:
    closedir(dir);
}

static void read_battery_uevent()
{
    ssize_t cnt = asreadall(uevent_fd, &buffer, &buffer_sz);
    if (cnt < 0)
        err(1, "%s on %s%s/%s failed", "read", path, battery_device, "uevent");

    if (lseek(uevent_fd, 0, SEEK_SET) == (off_t) -1)
        err(1, "%s on %s failed", "lseek", path);
}

static auto get_property(std::string_view name) -> std::string_view
{
    char *substr = strcasestr(buffer, name.data());
    if (!substr)
        return "nullptr";

    const char *value = substr + name.size() + 1;
    const char *end = strchrnul(substr, '\n');

    return {value, static_cast<std::size_t>(end - value)};
}
static auto get_property_lazy(std::string_view name) noexcept
{
    return LazyEval{[=]() noexcept
    {
        return get_property(name);
    }};
}
static auto get_conditional_lazy(std::string_view name, std::string_view val) noexcept
{
    return LazyEval{[=]() noexcept
    {
        return Conditional{get_property(name) == val};
    }};
}

void print_battery()
{
    read_battery_uevent();

    swaystatus::print(
        format,
        fmt::arg("type", "battery"),

#define ARG(literal) fmt::arg((literal), get_property_lazy(literal))
        ARG("name"),
        ARG("present"),
        ARG("technology"),

        ARG("model_name"),
        ARG("manufacturer"),
        ARG("serial_number"),

        ARG("status"),

        ARG("cycle_count"),

        ARG("voltage_min_design"),
        ARG("voltage_now"),

        ARG("charge_full_design"),
        ARG("charge_full"),
        ARG("charge_now"),

        ARG("capacity"),
        ARG("capacity_level"),
#undef ARG

        fmt::arg("is_charging", get_conditional_lazy("status", "Charging")),
        fmt::arg("is_discharging", get_conditional_lazy("status", "Discharging")),
        fmt::arg("is_not_charging", get_conditional_lazy("status", "Not charging")),
        fmt::arg("is_full", get_conditional_lazy("status", "Full"))
    );
}
}
