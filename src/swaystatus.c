#define _DEFAULT_SOURCE /* For setlintbuf */

#include <stdio.h>
#include <unistd.h>

#include "print_battery.h"
#include "print_time.h"
#include "print_volume.h"

void print_delimiter()
{
    fputs(" | ", stdout);
}

int main(int argc, char* argv[])
{
    const char * const format = argc == 1 ? "%Y-%m-%d %T" : argv[1];

    /*
     * Make sure fflush only happens when a newline is outputed.
     * Otherwise, swaybar might misbehave.
     */
    setlinebuf(stdout); 

    init_upclient();
    init_alsa("Master", "default");

    for ( ; ; sleep(1)) {
        print_volume();
        print_delimiter();

        print_battery();
        print_delimiter();

        print_time(format);

        puts("");
    }

    return 0;
}
