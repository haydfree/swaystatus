#define _GNU_SOURCE     /* For RTLD_DEFAULT */
#define _DEFAULT_SOURCE /* For nice */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include <malloc.h>

#include <unistd.h>
#include <signal.h>
#include <dlfcn.h>

#include <execinfo.h>
#include <err.h>
#include <errno.h>

#include "help.h"
#include "utility.h"
#include "printer.hpp"
#include "process_configuration.h"
#include "poller.h"

#define starts_with(str, prefix) (strncmp((str), (prefix), sizeof(prefix) - 1) == 0)

static void *bt_buffer[20];
static void stack_bt()
{
    fputs("\n\n", stderr);

    int sz = backtrace(bt_buffer, sizeof(bt_buffer) / sizeof(void*));
    backtrace_symbols_fd(bt_buffer, sz, 2);
}
static void terminate_handler()
{
    stack_bt();
    _exit(1);
}
static void sigabort_handler(int sig)
{
    stack_bt();
}

static bool reload_requested;
static void handle_reload_request(int sig)
{
    reload_requested = true;
}

static uintmax_t parse_cmdline_arg_and_initialize(
    int argc, char* argv[],
    bool *is_reload, const char **config_filename,
    struct Blocks *blocks
)
{
    /* Default interval is 1 second */
    uintmax_t interval = 1000;

    void *config = NULL;

    for (int i = 1; i != argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            fputs(help, stderr);
            exit(1);
        } else if (starts_with(argv[i], "--interval=")) {
            char *endptr;
            errno = 0;
            interval = strtoumax(argv[i] + sizeof("--interval=") - 1, &endptr, 10);
            if (errno == ERANGE)
                err(1, "Invalid argument %s%s", argv[i], "");
            else if (*endptr != '\0')
                errx(1, "Invalid argument %s%s", argv[i], ": Contains non-digit character");
        } else if (strcmp(argv[i], "--reload") == 0) {
            *is_reload = true;
        } else {
            if (config)
                errx(1, "Error: configuration file is specified twice");
            *config_filename = realpath_checked(argv[i]);
            config = load_config(argv[i]);
        }
    }

    init_poller();

    struct Inits inits;
    parse_inits_config(config, &inits);

    for (size_t i = 0; inits.inits[i]; ++i)
        inits.inits[i](config);

    parse_block_printers_config(config, inits.order, blocks);

    free_config(config);

    return interval;
}

static void print_block(void (*print)(), const char *json_element_str)
{
    print_literal_str("{\"full_text\":\"");
    print();
    print_literal_str("\",");
    print_str(json_element_str);
    print_literal_str("}");
}
static void print_delimiter()
{
    print_literal_str(",");
}

int main(int argc, char* argv[])
{
    close_all();

    /* Force dynamic linker to load function backtrace */
    if (dlsym(RTLD_DEFAULT, "backtrace") == NULL)
        err(1, "%s on %s failed", "dlsym", "backtrace");

    nice(19);

    set_terminate_handler(terminate_handler);
    sigaction_checked(SIGABRT, sigabort_handler);
    sigaction_checked(SIGUSR1, handle_reload_request);

    struct Blocks blocks;

    bool is_reload = false;
    const char *config_filename = NULL;

    const uintmax_t interval = parse_cmdline_arg_and_initialize(
        argc, argv,
        &is_reload, &config_filename,
        &blocks
    );

    if (chdir("/") < 0)
        err(1, "%s failed", "chdir(\"/\")");

    if (!is_reload) {
        /* Print header */
        print_literal_str("{\"version\":1}\n");
        flush();

        /* Begin an infinite array */
        print_literal_str("[\n");
        flush();
    }

    for (size_t sec = 0; !reload_requested; msleep(interval), ++sec) {
        perform_polling(0);

        print_literal_str("[");

        for (size_t i = 0; blocks.full_text_printers[i]; ++i) {
            print_block(blocks.full_text_printers[i], blocks.JSON_elements_strs[i]);
            print_delimiter();
        }

        /* Print dummy */
        print_literal_str("{}],\n");
        flush();

        if (sec == 3660) {
            malloc_trim(4096 * 3);
            sec = 0;
        }
    }

    if (reload_requested) {
        char buffer[4096];
        if (snprintf(buffer, sizeof(buffer), "--interval=%" PRIuMAX, interval) < 0)
            err(1, "%s on %s failed", "snprintf", "char buffer[4096]");

        execl("/proc/self/exe", "/proc/self/exe", buffer, "--reload", config_filename, NULL);
        err(1, "%s on %s failed", "execv", "/proc/self/exe");
    }

    return 0;
}
