#include "snake_puzzle/platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "snake_puzzle/common.h"

#ifndef SP_DEFAULT_ATOMS_DIR
#define SP_DEFAULT_ATOMS_DIR "assets/atoms"
#endif

#ifdef _WIN32
#include <windows.h>
typedef struct _stat sp_stat_t;
#define sp_stat _stat
#else
#include <unistd.h>
typedef struct stat sp_stat_t;
#define sp_stat stat
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

static void copy_path(char *out, size_t out_size, const char *value)
{
    if (out_size == 0) {
        return;
    }
    snprintf(out, out_size, "%s", value != NULL ? value : "");
}

static bool path_is_dir(const char *path)
{
    sp_stat_t info;

    if (path == NULL || path[0] == '\0') {
        return false;
    }
    if (sp_stat(path, &info) != 0) {
        return false;
    }
#ifdef _WIN32
    return (info.st_mode & _S_IFDIR) != 0;
#else
    return S_ISDIR(info.st_mode);
#endif
}

bool sp_executable_path(char *out, size_t out_size)
{
    if (out == NULL || out_size == 0) {
        return false;
    }
    out[0] = '\0';
#ifdef _WIN32
    {
        DWORD length = GetModuleFileNameA(NULL, out, (DWORD)out_size);
        if (length == 0 || length >= (DWORD)out_size) {
            out[0] = '\0';
            return false;
        }
        return true;
    }
#elif defined(__APPLE__)
    {
        char buffer[4096];
        uint32_t size = (uint32_t)sizeof(buffer);
        if (_NSGetExecutablePath(buffer, &size) != 0) {
            return false;
        }
        copy_path(out, out_size, buffer);
        return true;
    }
#else
    {
        ssize_t length = readlink("/proc/self/exe", out, out_size - 1);
        if (length <= 0 || (size_t)length >= out_size - 1) {
            out[0] = '\0';
            return false;
        }
        out[length] = '\0';
        return true;
    }
#endif
}

bool sp_executable_dir(char *out, size_t out_size)
{
    char path[SP_ATOM_PATH];
    char *slash;

    if (!sp_executable_path(path, sizeof(path))) {
        if (out_size > 0) {
            out[0] = '\0';
        }
        return false;
    }
    slash = strrchr(path, '/');
#ifdef _WIN32
    {
        char *backslash = strrchr(path, '\\');
        if (backslash != NULL && (slash == NULL || backslash > slash)) {
            slash = backslash;
        }
    }
#endif
    if (slash == NULL) {
        copy_path(out, out_size, ".");
        return true;
    }
    *slash = '\0';
    copy_path(out, out_size, path);
    return true;
}

static bool try_candidate(const char *candidate, char *out, size_t out_size)
{
    if (!path_is_dir(candidate)) {
        return false;
    }
    copy_path(out, out_size, candidate);
    return true;
}

static bool join_path(char *out, size_t out_size, const char *base, const char *suffix)
{
    int written = snprintf(out, out_size, "%s/%s", base, suffix);

    return written >= 0 && (size_t)written < out_size;
}

bool sp_resolve_atoms_path(const char *requested, bool requested_explicit, char *out,
                           size_t out_size)
{
    const char *env = getenv("SNAKE_PUZZLE_ATOMS");
    char exe_dir[SP_ATOM_PATH];
    char candidate[SP_ATOM_PATH];
    int i;

    if (out == NULL || out_size == 0) {
        return false;
    }
    out[0] = '\0';
    if (requested_explicit && requested != NULL && requested[0] != '\0') {
        copy_path(out, out_size, requested);
        return true;
    }
    if (env != NULL && env[0] != '\0') {
        copy_path(out, out_size, env);
        return true;
    }
    if (try_candidate("assets/atoms", out, out_size)) {
        return true;
    }
    if (sp_executable_dir(exe_dir, sizeof(exe_dir))) {
        static const char *const CANDIDATE_SUFFIXES[] = {
            "assets/atoms",
            "../Resources/assets/atoms",
            "../share/snake-puzzle/assets/atoms",
            "../assets/atoms",
        };
        for (i = 0; i < (int)(sizeof(CANDIDATE_SUFFIXES) / sizeof(CANDIDATE_SUFFIXES[0]));
             ++i) {
            if (join_path(candidate, sizeof(candidate), exe_dir, CANDIDATE_SUFFIXES[i]) &&
                try_candidate(candidate, out, out_size)) {
                return true;
            }
        }
    }
    if (requested != NULL && requested[0] != '\0') {
        copy_path(out, out_size, requested);
        return true;
    }
    copy_path(out, out_size, SP_DEFAULT_ATOMS_DIR);
    return true;
}
