/* ls [directory]: lists the initrd's files, or those under directory. The
   initrd stores full paths, so directories are only path prefixes. */
#include "sol.h"

#define NAME_MAX 256

int main(int argc, char *argv[])
{
    const char *directory = argc > 1 ? argv[1] : 0;
    char name[NAME_MAX];
    int found = 0;

    for (long i = 0; sys_list(i, name, sizeof(name)) >= 0; i++) {
        const char *rest = name;

        if (directory != 0) {
            rest = skip_prefix(name, directory);
            if (rest == 0 || *rest != '/')
                continue;
            rest++;
        }
        print(rest);
        print("\n");
        found = 1;
    }

    if (directory != 0 && !found) {
        print_to(STDERR, "ls: ");
        print_to(STDERR, directory);
        print_to(STDERR, ": no such directory\n");
        return 1;
    }
    return 0;
}
