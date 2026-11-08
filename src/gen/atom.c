#include "snake_puzzle/gen/atom.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#define sp_stat _stat
typedef struct _stat sp_stat_t;
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & _S_IFMT) == _S_IFREG)
#endif
#else
#include <dirent.h>
#define sp_stat stat
typedef struct stat sp_stat_t;
#endif

#include "snake_puzzle/core/score.h"

static char *trim_line(char *line)
{
    char *end;

    while (*line != '\0' && isspace((unsigned char)*line)) {
        ++line;
    }
    end = line + strlen(line);
    while (end > line && isspace((unsigned char)end[-1])) {
        --end;
    }
    *end = '\0';
    return line;
}

static bool has_suffix(const char *text, const char *suffix)
{
    size_t text_len = strlen(text);
    size_t suffix_len = strlen(suffix);

    return text_len >= suffix_len && strcmp(text + text_len - suffix_len, suffix) == 0;
}

static bool copy_name(char *out, size_t out_size, const char *source)
{
    size_t length = strlen(source);

    if (length == 0 || length >= out_size) {
        return false;
    }
    memcpy(out, source, length + 1);
    return true;
}

static bool atom_validate_layout(Atom *atom, char *error, size_t error_size)
{
    int x;
    int y;
    int entries = 0;
    int exits = 0;

    atom->entry_x = -1;
    atom->entry_y = -1;
    atom->exit_x = -1;
    atom->exit_y = -1;
    for (y = 0; y < atom->height; ++y) {
        if ((int)strlen(atom->cells[y]) != atom->width) {
            snprintf(error, error_size, "%s: layout row %d has inconsistent width",
                     atom->name, y + 1);
            return false;
        }
        for (x = 0; x < atom->width; ++x) {
            char c = atom->cells[y][x];
            if (c == 'E') {
                entries += 1;
                atom->entry_x = x;
                atom->entry_y = y;
            } else if (c == 'X') {
                exits += 1;
                atom->exit_x = x;
                atom->exit_y = y;
            } else if (strchr("#.B_LT<>^vKAP2*$~", c) == NULL) {
                snprintf(error, error_size, "%s: unknown atom marker '%c' at %d,%d",
                         atom->name, c, x, y);
                return false;
            }
        }
    }
    if (entries != 1 || exits != 1) {
        snprintf(error, error_size, "%s: atom needs exactly one E and one X", atom->name);
        return false;
    }
    if (atom->entry_x != 0 || atom->exit_x != atom->width - 1 ||
        atom->entry_y != atom->exit_y) {
        snprintf(error, error_size,
                 "%s: E must be on the west edge and X on the east edge at the same row",
                 atom->name);
        return false;
    }
    return true;
}

static bool atom_parse_single(FILE *file, const char *path, AtomLibrary *library, char *error,
                              size_t error_size)
{
    char line[SP_LINE_MAX];
    char solution_text[SP_LINE_MAX];
    char name[SP_ATOM_NAME] = "";
    int difficulty = 1;
    int weight = 2;
    int line_no = 0;
    int state = 0; /* 0 meta, 1 layout, 2 solution, 3 done */
    Atom *atom;
    bool saw_layout = false;

    if (library->count >= SP_ATOM_MAX) {
        snprintf(error, error_size, "atom library is full");
        return false;
    }
    atom = &library->atoms[library->count];
    memset(atom, 0, sizeof(*atom));
    atom->difficulty = 1;
    atom->weight = 2;
    solution_text[0] = '\0';

    while (fgets(line, sizeof(line), file) != NULL) {
        char *text;
        char *value;

        ++line_no;
        if (strchr(line, '\n') == NULL && !feof(file)) {
            snprintf(error, error_size, "%s:%d: line is too long", path, line_no);
            return false;
        }
        text = trim_line(line);
        if (state == 0) {
            if (*text == '\0' || *text == '#') {
                continue;
            }
            if (strcmp(text, "layout") == 0) {
                state = 1;
                saw_layout = true;
                continue;
            }
            value = strchr(text, ' ');
            if (value == NULL) {
                snprintf(error, error_size, "%s:%d: expected 'key value'", path, line_no);
                return false;
            }
            *value++ = '\0';
            value = trim_line(value);
            if (strcmp(text, "name") == 0) {
                if (!copy_name(name, sizeof(name), value)) {
                    snprintf(error, error_size, "%s:%d: invalid atom name", path, line_no);
                    return false;
                }
            } else if (strcmp(text, "difficulty") == 0) {
                difficulty = atoi(value);
            } else if (strcmp(text, "weight") == 0) {
                weight = atoi(value);
            } else if (strcmp(text, "atom") == 0) {
                if (!copy_name(name, sizeof(name), value)) {
                    snprintf(error, error_size, "%s:%d: invalid atom name", path, line_no);
                    return false;
                }
            } else {
                snprintf(error, error_size, "%s:%d: unknown metadata '%s'", path, line_no,
                         text);
                return false;
            }
        } else if (state == 1) {
            if (strcmp(text, "end") == 0) {
                state = 2;
                continue;
            }
            if ((int)strlen(text) > SP_ATOM_MAX_W || atom->height >= SP_ATOM_MAX_H) {
                snprintf(error, error_size, "%s:%d: layout exceeds %dx%d", path, line_no,
                         SP_ATOM_MAX_W, SP_ATOM_MAX_H);
                return false;
            }
            if (atom->width == 0) {
                atom->width = (int)strlen(text);
            }
            if ((int)strlen(text) != atom->width) {
                snprintf(error, error_size, "%s:%d: inconsistent layout row width", path,
                         line_no);
                return false;
            }
            memcpy(atom->cells[atom->height], text, (size_t)atom->width + 1);
            atom->height += 1;
        } else if (state == 2) {
            if (strcmp(text, "end") == 0) {
                state = 3;
                continue;
            }
            if (strcmp(text, "solution") == 0 || *text == '\0') {
                continue;
            }
            if (solution_text[0] != '\0') {
                strncat(solution_text, " ", sizeof(solution_text) - strlen(solution_text) - 1);
            }
            strncat(solution_text, text, sizeof(solution_text) - strlen(solution_text) - 1);
        } else if (*text != '\0' && *text != '#') {
            snprintf(error, error_size, "%s:%d: unexpected content after solution", path,
                     line_no);
            return false;
        }
    }

    if (!saw_layout || atom->width < 3 || atom->height < 3) {
        snprintf(error, error_size, "%s: missing or invalid layout", path);
        return false;
    }
    if (state < 2) {
        snprintf(error, error_size, "%s: missing solution section", path);
        return false;
    }
    if (name[0] == '\0') {
        snprintf(error, error_size, "%s: missing atom name", path);
        return false;
    }
    if (difficulty < 1) {
        difficulty = 1;
    }
    if (weight < 1) {
        weight = 1;
    }
    if (atom->width > SP_ATOM_MAX_W || atom->height > SP_ATOM_MAX_H) {
        snprintf(error, error_size, "%s: atom is too large", path);
        return false;
    }
    if (!atom_validate_layout(atom, error, error_size)) {
        return false;
    }
    if (!command_parse(solution_text, &atom->solution, error, error_size)) {
        char detail[256];
        snprintf(detail, sizeof(detail), "%s: invalid solution: %s", path,
                 error != NULL ? error : "parse error");
        snprintf(error, error_size, "%s", detail);
        return false;
    }
    if (atom->solution.count == 0) {
        snprintf(error, error_size, "%s: atom solution is empty", path);
        return false;
    }
    snprintf(atom->name, sizeof(atom->name), "%s", name);
    atom->difficulty = difficulty;
    atom->weight = weight;
    snprintf(atom->source_path, sizeof(atom->source_path), "%s", path);
    library->count += 1;
    return true;
}

static bool atom_parse_spa(const char *path, const char *content, AtomLibrary *library,
                           char *error, size_t error_size)
{
    char *copy = malloc(strlen(content) + 1);
    char *cursor;
    int line_no = 0;
    Atom *atom = NULL;

    if (copy == NULL) {
        snprintf(error, error_size, "out of memory");
        return false;
    }
    memcpy(copy, content, strlen(content) + 1);
    cursor = copy;
    while (cursor != NULL) {
        char *line = cursor;
        char *next_line = strchr(cursor, '\n');
        char *text;
        char *bar1;
        char *bar2;
        char *bar3;

        if (next_line != NULL) {
            *next_line = '\0';
            cursor = next_line + 1;
        } else {
            cursor = NULL;
        }
        ++line_no;
        text = trim_line(line);
        if (line_no == 1) {
            if (strcmp(text, "SPA1") != 0) {
                snprintf(error, error_size, "%s: not an SPA1 file", path);
                free(copy);
                return false;
            }
            continue;
        }
        if (text[0] == '\0' || (atom == NULL && text[0] == '#')) {
            continue;
        }
        if (strncmp(text, "BEGIN ", 6) == 0) {
            if (library->count >= SP_ATOM_MAX) {
                snprintf(error, error_size, "%s:%d: atom library is full", path, line_no);
                free(copy);
                return false;
            }
            atom = &library->atoms[library->count];
            memset(atom, 0, sizeof(*atom));
            atom->difficulty = 1;
            atom->weight = 2;
            bar1 = strchr(text + 6, '|');
            if (bar1 == NULL) {
                goto spa_error;
            }
            *bar1++ = '\0';
            bar2 = strchr(bar1, '|');
            if (bar2 == NULL) {
                goto spa_error;
            }
            *bar2++ = '\0';
            bar3 = strchr(bar2, '|');
            if (bar3 == NULL) {
                goto spa_error;
            }
            *bar3++ = '\0';
            if (!copy_name(atom->name, sizeof(atom->name), text + 6)) {
                goto spa_error;
            }
            atom->difficulty = atoi(bar1);
            atom->weight = atoi(bar2);
            if (!command_parse(bar3, &atom->solution, error, error_size)) {
                free(copy);
                return false;
            }
            snprintf(atom->source_path, sizeof(atom->source_path), "%s", path);
            if (atom->difficulty < 1) {
                atom->difficulty = 1;
            }
            if (atom->weight < 1) {
                atom->weight = 1;
            }
            continue;
        }
        if (strcmp(text, "END") == 0) {
            if (atom == NULL || atom->width < 3 || atom->height < 3) {
                snprintf(error, error_size, "%s:%d: atom ended without a layout", path,
                         line_no);
                free(copy);
                return false;
            }
            if (!atom_validate_layout(atom, error, error_size)) {
                free(copy);
                return false;
            }
            library->count += 1;
            atom = NULL;
            continue;
        }
        if (atom == NULL) {
            snprintf(error, error_size, "%s:%d: layout row outside BEGIN/END", path, line_no);
            free(copy);
            return false;
        }
        if ((int)strlen(text) > SP_ATOM_MAX_W || atom->height >= SP_ATOM_MAX_H) {
            snprintf(error, error_size, "%s:%d: layout exceeds atom bounds", path, line_no);
            free(copy);
            return false;
        }
        if (atom->width == 0) {
            atom->width = (int)strlen(text);
        }
        if ((int)strlen(text) != atom->width) {
            snprintf(error, error_size, "%s:%d: inconsistent layout row width", path, line_no);
            free(copy);
            return false;
        }
        memcpy(atom->cells[atom->height], text, (size_t)atom->width + 1);
        atom->height += 1;
        continue;

    spa_error:
        snprintf(error, error_size, "%s:%d: malformed BEGIN header", path, line_no);
        free(copy);
        return false;
    }
    if (atom != NULL) {
        snprintf(error, error_size, "%s: unterminated BEGIN block", path);
        free(copy);
        return false;
    }
    free(copy);
    if (library->count == 0) {
        snprintf(error, error_size, "%s: no atoms in SPA1 file", path);
        return false;
    }
    return true;
}

bool atom_load_file(AtomLibrary *library, const char *path, char *error, size_t error_size)
{
    FILE *file;
    char first[SP_LINE_MAX];
    long size;
    char *content;
    size_t read_size;
    bool ok;

    file = fopen(path, "rb");
    if (file == NULL) {
        snprintf(error, error_size, "cannot open %s: %s", path, strerror(errno));
        return false;
    }
    if (fgets(first, sizeof(first), file) == NULL) {
        fclose(file);
        snprintf(error, error_size, "%s: file is empty", path);
        return false;
    }
    rewind(file);
    if (strncmp(trim_line(first), "SPA1", 4) == 0) {
        if (fseek(file, 0, SEEK_END) != 0 || (size = ftell(file)) < 0 ||
            fseek(file, 0, SEEK_SET) != 0) {
            fclose(file);
            snprintf(error, error_size, "%s: cannot read file", path);
            return false;
        }
        content = malloc((size_t)size + 1);
        if (content == NULL) {
            fclose(file);
            snprintf(error, error_size, "out of memory");
            return false;
        }
        read_size = fread(content, 1, (size_t)size, file);
        fclose(file);
        content[read_size] = '\0';
        ok = atom_parse_spa(path, content, library, error, error_size);
        free(content);
        return ok;
    }
    fclose(file);
    file = fopen(path, "rb");
    if (file == NULL) {
        snprintf(error, error_size, "cannot open %s: %s", path, strerror(errno));
        return false;
    }
    ok = atom_parse_single(file, path, library, error, error_size);
    fclose(file);
    return ok;
}

static int compare_strings(const void *a, const void *b)
{
    return strcmp((const char *)a, (const char *)b);
}

static int collect_atom_files(const char *path, char paths[][SP_ATOM_PATH], int max_paths)
{
    int count = 0;

#ifdef _WIN32
    {
        char pattern[SP_ATOM_PATH];
        WIN32_FIND_DATAA data;
        HANDLE handle;

        if (snprintf(pattern, sizeof(pattern), "%s/*", path) >= (int)sizeof(pattern)) {
            return -1;
        }
        handle = FindFirstFileA(pattern, &data);
        if (handle == INVALID_HANDLE_VALUE) {
            return -1;
        }
        do {
            char full[SP_ATOM_PATH];
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                continue;
            }
            if (!has_suffix(data.cFileName, ".txt") && !has_suffix(data.cFileName, ".atom")) {
                continue;
            }
            if (snprintf(full, sizeof(full), "%s/%s", path, data.cFileName) >=
                (int)sizeof(full)) {
                continue;
            }
            if (count < max_paths) {
                snprintf(paths[count], SP_ATOM_PATH, "%s", full);
                count += 1;
            }
        } while (FindNextFileA(handle, &data));
        FindClose(handle);
    }
#else
    {
        DIR *dir = opendir(path);
        struct dirent *entry;

        if (dir == NULL) {
            return -1;
        }
        while ((entry = readdir(dir)) != NULL) {
            char full[SP_ATOM_PATH];
            sp_stat_t entry_info;

            if (entry->d_name[0] == '.') {
                continue;
            }
            if (!has_suffix(entry->d_name, ".txt") && !has_suffix(entry->d_name, ".atom")) {
                continue;
            }
            if (snprintf(full, sizeof(full), "%s/%s", path, entry->d_name) >=
                (int)sizeof(full)) {
                continue;
            }
            if (sp_stat(full, &entry_info) != 0 || !S_ISREG(entry_info.st_mode)) {
                continue;
            }
            if (count < max_paths) {
                snprintf(paths[count], SP_ATOM_PATH, "%s", full);
                count += 1;
            }
        }
        closedir(dir);
    }
#endif
    return count;
}

bool atom_load_path(AtomLibrary *library, const char *path, char *error, size_t error_size)
{
    sp_stat_t info;

    memset(library, 0, sizeof(*library));
    if (path == NULL || *path == '\0') {
        path = "assets/atoms";
    }
    if (sp_stat(path, &info) != 0) {
        snprintf(error, error_size, "atom path not found: %s", path);
        return false;
    }
    if (!S_ISDIR(info.st_mode)) {
        bool ok = atom_load_file(library, path, error, error_size);
        if (ok) {
            snprintf(library->source, sizeof(library->source), "%s", path);
        }
        return ok;
    }

    {
        char paths[SP_ATOM_MAX * 2][SP_ATOM_PATH];
        int path_count = collect_atom_files(path, paths, SP_ATOM_MAX * 2);
        int i;

        if (path_count < 0) {
            snprintf(error, error_size, "cannot open atom directory %s", path);
            return false;
        }
        if (path_count == 0) {
            snprintf(error, error_size, "no atom files found in %s", path);
            return false;
        }
        qsort(paths, (size_t)path_count, sizeof(paths[0]), compare_strings);
        for (i = 0; i < path_count; ++i) {
            if (!atom_load_file(library, paths[i], error, error_size)) {
                return false;
            }
        }
        snprintf(library->source, sizeof(library->source), "%s", path);
    }
    return true;
}

static bool stamp_marker(const Atom *atom, int x, int y, char marker, Game *game, int *gate_ids,
                         int *gate_count, int *plate_ids, int *plate_count, Point *teleports,
                         int *teleport_count)
{
    switch (marker) {
    case '#':
        if (!game_set_cell(game, x, y, TERRAIN_WALL)) {
            return false;
        }
        break;
    case '.':
    case 'E':
    case 'X':
        if (!game_set_cell(game, x, y, TERRAIN_FLOOR)) {
            return false;
        }
        break;
    case 'B':
        if (!game_set_cell(game, x, y, TERRAIN_FLOOR) ||
            game_add_block(game, x, y) < 0) {
            return false;
        }
        break;
    case '_': {
        int plate = game_add_plate(game, x, y, 0);
        if (plate < 0 || *plate_count >= 64) {
            return false;
        }
        plate_ids[(*plate_count)++] = plate;
        break;
    }
    case 'L': {
        int gate = game_add_gate(game, GATE_LOCKED, x, y, 0, 0);
        if (gate < 0 || *gate_count >= 64) {
            return false;
        }
        gate_ids[(*gate_count)++] = gate;
        break;
    }
    case '<':
    case '>':
    case '^':
    case 'v': {
        Cell *cell = board_cell(&game->board, x, y);
        Direction dir = marker == '<' ? DIR_LEFT : marker == '>' ? DIR_RIGHT
                       : marker == '^' ? DIR_UP
                                       : DIR_DOWN;
        if (cell == NULL) {
            return false;
        }
        cell->type = TERRAIN_ONEWAY;
        cell->oneway_dir = (uint8_t)dir;
        break;
    }
    case '~':
        if (!game_set_cell(game, x, y, TERRAIN_BREAK)) {
            return false;
        }
        break;
    case 'K':
    case 'A':
    case 'P':
    case '2': {
        ItemType type = marker == 'K' ? ITEM_KEY
                        : marker == 'A' ? ITEM_SHIELD
                        : marker == 'P' ? ITEM_PHASE
                                        : ITEM_DOUBLE;
        if (!game_set_cell(game, x, y, TERRAIN_FLOOR) || game_add_item(game, x, y, type) < 0) {
            return false;
        }
        break;
    }
    case '*':
    case '$':
        if (!game_set_cell(game, x, y, TERRAIN_FLOOR) ||
            game_add_food(game, x, y, marker == '$' ? FOOD_GOLDEN : FOOD_NORMAL) < 0) {
            return false;
        }
        break;
    case 'T':
        if (*teleport_count >= SP_MAX_TELEPORTS * 2) {
            return false;
        }
        teleports[*teleport_count].x = x;
        teleports[*teleport_count].y = y;
        *teleport_count += 1;
        if (!game_set_cell(game, x, y, TERRAIN_FLOOR)) {
            return false;
        }
        break;
    default:
        return false;
    }
    SP_UNUSED(atom);
    return true;
}

bool atom_stamp(const Atom *atom, int origin_x, int origin_y, Game *game, char *error,
                size_t error_size)
{
    int gate_ids[64];
    int gate_count = 0;
    int plate_ids[64];
    int plate_count = 0;
    Point teleports[SP_MAX_TELEPORTS * 2];
    int teleport_count = 0;
    int x;
    int y;
    int i;

    if (origin_x < 0 || origin_y < 0 || origin_x + atom->width > game->board.width ||
        origin_y + atom->height > game->board.height) {
        snprintf(error, error_size, "atom %s does not fit at %d,%d", atom->name, origin_x,
                 origin_y);
        return false;
    }
    for (y = 0; y < atom->height; ++y) {
        for (x = 0; x < atom->width; ++x) {
            char marker = atom->cells[y][x];
            if (!stamp_marker(atom, origin_x + x, origin_y + y, marker, game, gate_ids,
                              &gate_count, plate_ids, &plate_count, teleports,
                              &teleport_count)) {
                snprintf(error, error_size, "atom %s failed while stamping '%c' at %d,%d",
                         atom->name, marker, x, y);
                return false;
            }
        }
    }
    for (i = 0; i < plate_count; ++i) {
        uint64_t mask = 0;
        int j;
        for (j = 0; j < gate_count && gate_ids[j] < 64; ++j) {
            mask |= UINT64_C(1) << gate_ids[j];
        }
        game->plates[plate_ids[i]].gate_mask = mask;
    }
    for (i = 0; i + 1 < teleport_count; i += 2) {
        if (game_add_teleport(game, teleports[i].x, teleports[i].y, teleports[i + 1].x,
                              teleports[i + 1].y) < 0) {
            snprintf(error, error_size, "atom %s could not create teleport pair", atom->name);
            return false;
        }
    }
    if (teleport_count % 2 != 0) {
        snprintf(error, error_size, "atom %s has an odd number of teleporters", atom->name);
        return false;
    }
    return true;
}

const Atom *atom_find(const AtomLibrary *library, const char *name)
{
    int i;

    for (i = 0; i < library->count; ++i) {
        if (strcmp(library->atoms[i].name, name) == 0) {
            return &library->atoms[i];
        }
    }
    return NULL;
}

void atom_library_summary(const AtomLibrary *library, char *out, size_t out_size)
{
    int i;
    size_t used = 0;

    out[0] = '\0';
    for (i = 0; i < library->count; ++i) {
        int written = snprintf(out + used, out_size - used, "%s%s",
                               i == 0 ? "" : ", ", library->atoms[i].name);
        if (written < 0 || (size_t)written >= out_size - used) {
            break;
        }
        used += (size_t)written;
    }
}
