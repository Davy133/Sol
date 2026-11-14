#include "sol.h"

#define LINE_MAX    128
#define ARGS_MAX    16

static char line[LINE_MAX];

static void read_line(void)
{
    unsigned long length = 0;
    char keys[16];

    for (;;) {
        long count = sys_read(STDIN, keys, sizeof(keys));

        for (long i = 0; i < count; i++) {
            char c = keys[i];

            if (c == '\n') {
                print("\n");
                line[length] = '\0';
                return;
            }
            if (c == '\b') {
                if (length > 0) {
                    length--;
                    print("\b \b");
                }
            } else if (c >= ' ' && c <= '~' && length < LINE_MAX - 1) {
                line[length++] = c;
                sys_write(STDOUT, &c, 1);
            }
        }
    }
}

static int split(char *text, char *argv[])
{
    int argc = 0;

    for (;;) {
        while (*text == ' ')
            *text++ = '\0';
        if (*text == '\0' || argc == ARGS_MAX)
            break;
        argv[argc++] = text;
        while (*text != ' ' && *text != '\0')
            text++;
    }
    argv[argc] = 0;
    return argc;
}

/* No slash means bin/<name>. */
static const char *program_path(const char *name, char path[LINE_MAX + 4])
{
    unsigned long i = 0;

    for (const char *c = name; *c != '\0'; c++)
        if (*c == '/')
            return name;

    for (const char *c = "bin/"; *c != '\0'; c++)
        path[i++] = *c;
    for (const char *c = name; *c != '\0'; c++)
        path[i++] = *c;
    path[i] = '\0';
    return path;
}

static void run_program(char *argv[])
{
    char path[LINE_MAX + 4];
    long id = sys_spawn(program_path(argv[0], path), argv);

    if (id < 0) {
        print_to(STDERR, argv[0]);
        print_to(STDERR, ": command not found\n");
        return;
    }
    sys_wait(id);
}

static void run(char *text)
{
    char *argv[ARGS_MAX + 1];

    if (split(text, argv) == 0)
        return;

    if (equal(argv[0], "help")) {
        print("Built-ins: help, exit\n"
              "Anything else runs bin/<name>; 'ls bin' lists them.\n");
    } else if (equal(argv[0], "exit")) {
        sys_exit(0);
    } else {
        run_program(argv);
    }
}

int main(void)
{
    run((char[]){ "cat etc/motd" });

    for (;;) {
        print("sol> ");
        read_line();
        run(line);
    }
}
