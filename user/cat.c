#include "sol.h"

static char buffer[4096];

int main(int argc, char *argv[])
{
    int status = 0;

    if (argc < 2) {
        print_to(STDERR, "usage: cat file...\n");
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        int fd = sys_open(argv[i]);
        long count;

        if (fd < 0) {
            print_to(STDERR, "cat: ");
            print_to(STDERR, argv[i]);
            print_to(STDERR, ": no such file\n");
            status = 1;
            continue;
        }
        while ((count = sys_read(fd, buffer, sizeof(buffer))) > 0)
            sys_write(STDOUT, buffer, (unsigned long)count);
        sys_close(fd);
    }
    return status;
}
