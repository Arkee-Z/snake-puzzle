#include "term.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <conio.h>
#include <process.h>
#include <windows.h>
static DWORD saved_out_mode;
static DWORD saved_in_mode;
static bool out_mode_saved;
static bool in_mode_saved;
static bool raw_mode;
#else
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
static struct termios saved_termios;
static bool termios_saved;
static bool raw_mode;
#endif

static void term_restore_raw(void)
{
#ifdef _WIN32
    if (raw_mode && in_mode_saved) {
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), saved_in_mode);
    }
    raw_mode = false;
#else
    if (raw_mode && termios_saved) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
        raw_mode = false;
    }
#endif
}

static void term_restore_all(void)
{
    term_restore_raw();
#ifdef _WIN32
    if (out_mode_saved) {
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), saved_out_mode);
        out_mode_saved = false;
    }
#endif
}

static void term_signal_handler(int sig)
{
    (void)sig;
    term_restore_all();
    fputs("\x1b[?25h", stdout);
    fflush(stdout);
    _exit(130);
}

void term_init(void)
{
#ifdef _WIN32
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    if (GetConsoleMode(out, &saved_out_mode)) {
        out_mode_saved = true;
        SetConsoleMode(out, saved_out_mode | ENABLE_PROCESSED_OUTPUT |
                               ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
    if (GetConsoleMode(in, &saved_in_mode)) {
        in_mode_saved = true;
    }
#else
    if (isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &saved_termios) == 0) {
        termios_saved = true;
    }
#endif
    signal(SIGINT, term_signal_handler);
#ifdef SIGTERM
    signal(SIGTERM, term_signal_handler);
#endif
    atexit(term_shutdown);
    term_hide_cursor();
}

void term_shutdown(void)
{
    term_restore_all();
    term_show_cursor();
    fflush(stdout);
}

void term_enter_raw(void)
{
#ifdef _WIN32
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    if (in_mode_saved && GetConsoleMode(in, &mode)) {
        SetConsoleMode(in, mode & ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT));
        raw_mode = true;
    }
#else
    struct termios raw;
    if (!termios_saved) {
        return;
    }
    raw = saved_termios;
    raw.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
        raw_mode = true;
    }
#endif
}

void term_leave_raw(void)
{
    term_restore_raw();
}

int term_read_key(void)
{
#ifdef _WIN32
    if (_kbhit()) {
        return _getch();
    }
    return -1;
#else
    unsigned char c;
    ssize_t n;
    if (!raw_mode) {
        return -1;
    }
    do {
        n = read(STDIN_FILENO, &c, 1);
    } while (n < 0 && errno == EINTR);
    if (n == 1) {
        return (int)c;
    }
    return -1;
#endif
}

uint64_t term_now_ms(void)
{
#ifdef _WIN32
    return (uint64_t)GetTickCount64();
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)ts.tv_nsec / 1000000u;
#endif
}

void term_sleep_ms(int ms)
{
    if (ms <= 0) {
        return;
    }
#ifdef _WIN32
    Sleep((DWORD)ms);
#else
    {
        struct timespec ts;
        ts.tv_sec = ms / 1000;
        ts.tv_nsec = (long)(ms % 1000) * 1000000L;
        while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {
        }
    }
#endif
}

void term_clear(void)
{
    fputs("\x1b[2J\x1b[H", stdout);
}

void term_hide_cursor(void)
{
    fputs("\x1b[?25l", stdout);
    fflush(stdout);
}

void term_show_cursor(void)
{
    fputs("\x1b[?25h", stdout);
    fflush(stdout);
}
