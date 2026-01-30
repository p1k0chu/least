/*
 * least.c - pager that does the least a pager
 * needs to do. it does less than less
 * Copyright (C) 2026 p1k0chu
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

static ssize_t readline(FILE *file, char **buf, size_t *bufsize);
static char get_esc(int fd);
static void print_usage(FILE *file);

char *progname;

const char *license_text =
    "Copyright (C) 2026 p1k0chu\n"
    "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n"
    "This is free software: you are free to change and redistribute it.\n"
    "There is NO WARRANTY, to the extent permitted by law.\n\n"
    "Written by p1k0chu (https://github.com/p1k0chu)";

#define write_literal(fd, esc)                                      \
    if (write((fd), (esc), sizeof((esc)) - 1) != sizeof((esc)) - 1) \
        err(EXIT_FAILURE, "write");

int main(int argc, char **argv) {
    int tty;
    FILE *file;

    progname = strrchr(*argv, '/');
    if (progname == NULL)
        progname = *argv;
    else
        ++progname;

    tty = open("/dev/tty", O_RDONLY);
    if (tty < 0)
        err(EXIT_FAILURE, "couldn't open tty");

    if (argc < 2) {
        if (isatty(STDIN_FILENO)) {
            print_usage(stderr);
            return EXIT_FAILURE;
        }
        file = stdin;
    } else if (strcmp(argv[1], "--help") == 0) {
        print_usage(stdout);
        return EXIT_SUCCESS;
    } else if (strcmp(argv[1], "--version") == 0) {
        printf("%s 0.1\n%s\n", progname, license_text);
        return EXIT_SUCCESS;
    } else {
        file = fopen(argv[1], "r");
        if (file == NULL)
            err(EXIT_FAILURE, "fopen");
    }

    struct termios tios;
    if (tcgetattr(tty, &tios) < 0)
        err(EXIT_FAILURE, "tcgetattr");

    tios.c_lflag &= ~(ICANON | ECHO);
    tios.c_lflag |= ISIG;

    if (tcsetattr(tty, TCSANOW, &tios) < 0)
        err(EXIT_FAILURE, "tcsetattr");

    struct winsize ws;
    char c;
    ssize_t n;
    int p;

    if (ioctl(tty, TIOCGWINSZ, &ws) < 0)
        err(EXIT_FAILURE, "ioctl");

    struct pollfd pfds = {.fd = tty, .events = POLLIN};
    const nfds_t nfds = 1;

    write_literal(STDOUT_FILENO, "\033[2J");
    write_literal(STDOUT_FILENO, "\033[H");

    char *buf = NULL;
    size_t bufsize = 0;

    for (unsigned short i = 0; i < ws.ws_row - 1; ++i) {
        n = readline(file, &buf, &bufsize);
        if (write(STDOUT_FILENO, buf, n) != n)
            err(EXIT_FAILURE, "write");
    }
    write_literal(STDOUT_FILENO, "\r:");

    for (;;) {
        p = poll(&pfds, nfds, 100);
        if (p < 0) {
            err(EXIT_FAILURE, "poll");
        } else if (p > 0) {
            if (read(tty, &c, 1) < 0)
                err(EXIT_FAILURE, "read");

            if (c == 'q') {
                putchar('\r');
                return EXIT_SUCCESS;
            }

            unsigned short advance_lines;

            if (c == 'j' || (c == '\033' && get_esc(tty) == 'B')) {
                advance_lines = 1;
            } else if (c == 'd') {
                advance_lines = ws.ws_row / 2;
            } else if (c == 'f') {
                advance_lines = ws.ws_row;
            } else {
                advance_lines = 0;
            }

            if (advance_lines > 0) {
                write_literal(STDOUT_FILENO, "\033[2K\r");
                for (; advance_lines > 0; --advance_lines) {
                    n = readline(file, &buf, &bufsize);
                    if (write(STDOUT_FILENO, buf, n - 1) != n - 1)
                        err(EXIT_FAILURE, "write");
                    write_literal(STDOUT_FILENO, "\033[1S\r");
                }
                write_literal(STDOUT_FILENO, ":");
            }
        }

        struct winsize new_ws;
        if (ioctl(tty, TIOCGWINSZ, &new_ws) < 0)
            err(EXIT_FAILURE, "ioctl");

        if (memcmp(&ws, &new_ws, sizeof(ws)) == 0)
            continue;

        if (ws.ws_row < new_ws.ws_row) {
            write_literal(STDOUT_FILENO, "\033[2K\r");
            for (unsigned short i = ws.ws_row; i < new_ws.ws_row; ++i) {
                n = readline(file, &buf, &bufsize);
                if (write(STDOUT_FILENO, buf, n) != n)
                    err(EXIT_FAILURE, "write");
            }
            write_literal(STDOUT_FILENO, ":");
        }
        memcpy(&ws, &new_ws, sizeof(ws));
    }
    // unreachable
    return EXIT_SUCCESS;
}

static ssize_t readline(FILE *file, char **buf, size_t *bufsize) {
    ssize_t n;
    if ((n = getline(buf, bufsize, file)) == -1) {
        if (!feof(file))
            err(EXIT_FAILURE, "getline");
        else
            exit(EXIT_SUCCESS);
    }
    return n;
}

static char get_esc(int fd) {
    char c;
    if (read(fd, &c, 1) < 0)
        err(EXIT_FAILURE, "read");
    if (c != '[')
        return 0;

    // ignore modifiers
    ssize_t n;
    while (!isalpha(c) && (n = read(fd, &c, 1)) > 0)
        ;

    if (n < 0)
        err(EXIT_FAILURE, "read");
    if (n == 0)
        errx(EXIT_FAILURE, "eof on stdin");

    return c;
}

static void print_usage(FILE *file) {
    fprintf(file,
            "Usage:\n"
            "\t%1$s < file\n"
            "\tcommand | %1$s\n"
            "\t%1$s file\n"
            "\n\n"
            "%1$s --help\t\tshow this text\n"
            "%1$s --version\t\tshow version and license\n",
            progname);
}

