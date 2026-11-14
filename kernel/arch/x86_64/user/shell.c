/* The first process the kernel starts: a tiny shell with line editing. */
#include "syscall.h"

#define LINE_MAX 128

static char line[LINE_MAX];

static long length_of(const char *s)
{
    long n = 0;

    while (s[n] != '\0')
        n++;
    return n;
}

static void print(const char *s)
{
    syscall2(SYS_WRITE, (long)s, length_of(s));
}

/* What follows name in command, or 0 when command is not name. */
static const char *arguments(const char *command, const char *name)
{
    while (*name != '\0')
        if (*command++ != *name++)
            return 0;
    if (*command == ' ')
        return command + 1;
    return *command == '\0' ? command : 0;
}

/* Reads a line into line, echoing keys and handling backspace as a terminal
   in canonical mode would. */
static void read_line(void)
{
    long length = 0;
    char keys[16];

    for (;;) {
        long count = syscall2(SYS_READ, (long)keys, sizeof(keys));

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
                syscall2(SYS_WRITE, (long)&c, 1);
            }
        }
    }
}

static void run(const char *command)
{
    while (*command == ' ')
        command++;

    const char *echo = arguments(command, "echo");

    if (*command == '\0')
        return;
    if (echo) {
        print(echo);
        print("\n");
    } else if (arguments(command, "help")) {
        print("Commands:\n"
              "  help         show this list\n"
              "  echo <text>  print text\n"
              "  uname        print the system name\n"
              "  exit         end the shell (and with it, init)\n");
    } else if (arguments(command, "uname")) {
        print("Sol x86_64\n");
    } else if (arguments(command, "exit")) {
        syscall2(SYS_EXIT, 0, 0);
    } else {
        print(command);
        print(": command not found\n");
    }
}

void _start(void)
{
    print("\nWelcome to the Sol shell. Type 'help' for commands.\n");

    for (;;) {
        print("sol> ");
        read_line();
        run(line);
    }
}
