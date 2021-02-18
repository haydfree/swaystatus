#define _DEFAULT_SOURCE /* For setlintbuf */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <err.h>

#include "help.h"
#include "print_battery.h"
#include "print_time.h"
#include "print_volume.h"
#include "print_network_interfaces.h"
#include "print_brightness.h"
#include "print_memory_usage.h"
#include "print_load.h"

/**
 * @param prefix must be a literal string
 */
#define starts_with(str, prefix) (strncmp((str), (prefix), sizeof(prefix) - 1) == 0)

void print_delimiter()
{
    fputs("|", stdout);
}

int main(int argc, char* argv[])
{
    const char *format = "%Y-%m-%d %T";

    for (int i = 1; i != argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            puts(help);
            return 1;
        } else {
            const char *filename = argv[i] + (sizeof("--config=") - 1);
        }
    }

    ;

    /*
     * Make sure fflush only happens when a newline is outputed.
     * Otherwise, swaybar might misbehave.
     */
    setlinebuf(stdout); 

    init_time(format);
    init_upclient();
    init_alsa("Master", "default");
    init_network_interfaces_scanning();
    init_brightness_detection();
    init_memory_usage_collection();
    init_load();

    for ( ; ; sleep(1)) {
        print_brightness();
        print_delimiter();

        print_volume();
        print_delimiter();

        print_battery();
        print_delimiter();

        print_network_interfaces();
        print_delimiter();

        print_load();
        print_delimiter();

        print_memory_usage();
        print_delimiter();

        print_time();

        puts("");
    }

    return 0;
}
