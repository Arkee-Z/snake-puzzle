#include "snake_puzzle/ui/render_raylib.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "raylib.h"
#include "snake_puzzle/core/score.h"
#include "snake_puzzle/entity/item.h"

#define WIN_W 1360
#define WIN_H 900

static const Rectangle BOARD_PANEL = {36.0f, 110.0f, 860.0f, 650.0f};
static const Rectangle SIDE_PANEL = {920.0f, 110.0f, 404.0f, 650.0f};
static const Rectangle CMD_PANEL = {36.0f, 780.0f, 1288.0f, 88.0f};
static const Rectangle INPUT_RECT = {56.0f, 812.0f, 1000.0f, 42.0f};
static const Rectangle RUN_RECT = {1068.0f, 812.0f, 140.0f, 42.0f};
static const Rectangle CLEAR_RECT = {1220.0f, 812.0f, 84.0f, 42.0f};

static const Color COL_BG = {9, 12, 21, 255};
static const Color COL_BG_2 = {13, 18, 31, 255};
static const Color COL_PANEL = {17, 23, 38, 255};
static const Color COL_PANEL_2 = {23, 31, 50, 255};
static const Color COL_PANEL_3 = {29, 39, 61, 255};
static const Color COL_LINE = {42, 56, 82, 255};
static const Color COL_INK = {229, 237, 248, 255};
static const Color COL_MUTED = {122, 139, 165, 255};
static const Color COL_FAINT = {72, 88, 112, 255};
static const Color COL_GREEN = {92, 229, 139, 255};
static const Color COL_GREEN_DARK = {27, 104, 70, 255};
static const Color COL_RED = {255, 100, 104, 255};
static const Color COL_GOLD = {255, 200, 87, 255};
static const Color COL_CYAN = {74, 213, 236, 255};
static const Color COL_PURPLE = {197, 119, 255, 255};
static const Color COL_BLUE = {88, 145, 255, 255};
static const Color COL_ORANGE = {245, 144, 80, 255};

typedef struct BoardView {
    float tile;
    float origin_x;
    float origin_y;
} BoardView;

static float text_spacing(float size)
{
    return size * 0.045f + 0.35f;
}

static void text(const char *value, float x, float y, float size, Color color)
{
    DrawTextEx(GetFontDefault(), value, (Vector2){x, y}, size, text_spacing(size), color);
}

static void text_center(const char *value, float x, float y, float size, Color color)
{
    Vector2 measure = MeasureTextEx(GetFontDefault(), value, size, text_spacing(size));
    DrawTextEx(GetFontDefault(), value, (Vector2){x - measure.x * 0.5f, y}, size,
               text_spacing(size), color);
}

static void panel_rect(Rectangle rect, Color fill, Color border, float roundness)
{
    DrawRectangleRounded(rect, roundness, 10, fill);
    DrawRectangleRoundedLinesEx(rect, roundness, 10, 1.0f, border);
}

static void divider(float x, float y, float width)
{
    DrawRectangle((int)x, (int)y, (int)width, 1, COL_LINE);
}

static Color event_color(EventType type)
{
    switch (type) {
    case EVENT_DEATH:
        return COL_RED;
    case EVENT_SEAL_COLLECTED:
    case EVENT_SPECIAL_UNLOCKED:
        return COL_GOLD;
    case EVENT_ENTER_SPECIAL:
    case EVENT_SPECIAL_FINISHED:
    case EVENT_ORB_COLLECTED:
    case EVENT_MINI_UNLOCKED:
        return COL_CYAN;
    case EVENT_EAT_FOOD:
        return COL_ORANGE;
    case EVENT_PICKUP_ITEM:
        return COL_GREEN;
    case EVENT_GATE_OPENED:
    case EVENT_PLATE_PRESSED:
        return COL_PURPLE;
    default:
        return COL_MUTED;
    }
}

static void truncate_text(const char *source, char *out, size_t out_size, size_t max_chars)
{
    size_t length = strlen(source);

    if (length <= max_chars || out_size < 5) {
        snprintf(out, out_size, "%s", source);
        return;
    }
    if (max_chars >= out_size) {
        max_chars = out_size - 1;
    }
    memcpy(out, source, max_chars - 3);
    out[max_chars - 3] = '\0';
    strncat(out, "...", out_size - strlen(out) - 1);
}

static void draw_background(void)
{
    int x;
    int y;

    ClearBackground(COL_BG);
    for (x = 0; x < WIN_W; x += 48) {
        DrawRectangle(x, 0, 1, WIN_H, Fade(COL_LINE, 0.11f));
    }
    for (y = 0; y < WIN_H; y += 48) {
        DrawRectangle(0, y, WIN_W, 1, Fade(COL_LINE, 0.11f));
    }
    DrawCircleGradient((Vector2){1060.0f, 40.0f}, 380.0f, Fade(COL_GREEN, 0.075f),
                       Fade(COL_GREEN, 0.0f));
    DrawCircleGradient((Vector2){120.0f, 760.0f}, 340.0f, Fade(COL_PURPLE, 0.05f),
                       Fade(COL_PURPLE, 0.0f));
}

static void draw_logo(void)
{
    float x = 36.0f;
    float y = 26.0f;
    int i;

    for (i = 0; i < 4; ++i) {
        Rectangle block = {x + (float)i * 13.0f, y + 10.0f, 9.0f, 9.0f};
        DrawRectangleRounded(block, 0.32f, 4,
                             i == 3 ? COL_GREEN : Fade(COL_GREEN, 0.45f + 0.1f * (float)i));
    }
    text("SNAKE", x + 60.0f, 8.0f, 35.0f, COL_INK);
    {
        float snake_width =
            MeasureTextEx(GetFontDefault(), "SNAKE", 35.0f, text_spacing(35.0f)).x;
        text("PUZZLE", x + 60.0f + snake_width + 15.0f, 8.0f, 35.0f, COL_GREEN);
    }
    text("COMMAND-DRIVEN GRID LOGIC", x + 62.0f, 47.0f, 11.0f, COL_MUTED);
    text("ROGUELIKE / NO UNDO / NO MERCY", x + 62.0f, 64.0f, 10.0f, COL_FAINT);

    DrawRectangle(36, 98, 1288, 1, COL_LINE);
}

static void draw_header_meta(const App *app)
{
    const Game *game = app->active;
    char buffer[128];

    snprintf(buffer, sizeof(buffer), "SEED %llu", (unsigned long long)game->seed);
    text(buffer, 1100.0f, 34.0f, 13.0f, COL_MUTED);
    snprintf(buffer, sizeof(buffer), "%d x %d GRID", game->board.width, game->board.height);
    text(buffer, 1100.0f, 55.0f, 11.0f, COL_FAINT);
    text(app->in_special ? "CHAMBER LINK" : "MAIN RUN", 1100.0f, 73.0f, 10.0f,
         app->in_special ? COL_GOLD : COL_GREEN);
    DrawCircle(1086, 41, 4.0f, app->in_special ? COL_GOLD : COL_GREEN);
}

static BoardView board_view(const Game *game)
{
    BoardView view;
    float header = 34.0f;
    float pad = 16.0f;
    float inner_w = BOARD_PANEL.width - pad * 2.0f;
    float inner_h = BOARD_PANEL.height - header - pad * 2.0f;
    float tile_w = inner_w / (float)game->board.width;
    float tile_h = inner_h / (float)game->board.height;

    view.tile = floorf(fminf(tile_w, tile_h));
    if (view.tile < 8.0f) {
        view.tile = 8.0f;
    }
    view.origin_x = BOARD_PANEL.x +
                    (BOARD_PANEL.width - view.tile * (float)game->board.width) * 0.5f;
    view.origin_y = BOARD_PANEL.y + header +
                    (BOARD_PANEL.height - header - view.tile * (float)game->board.height) *
                        0.5f;
    return view;
}

static Rectangle cell_rect(BoardView view, int x, int y)
{
    return (Rectangle){view.origin_x + (float)x * view.tile,
                       view.origin_y + (float)y * view.tile, view.tile, view.tile};
}

static int snake_segment(const Game *game, int x, int y)
{
    int i;

    for (i = 0; i < game->snake.length; ++i) {
        if (game->snake.body[i].x == x && game->snake.body[i].y == y) {
            return i;
        }
    }
    return -1;
}

static void draw_wall(Rectangle tile, int x, int y)
{
    Color base = ((x + y) & 1) ? (Color){16, 22, 36, 255} : (Color){14, 20, 32, 255};
    Rectangle r = {tile.x + 1.0f, tile.y + 1.0f, tile.width - 2.0f, tile.height - 2.0f};

    DrawRectangleRounded((Rectangle){r.x + 1.0f, r.y + 2.0f, r.width, r.height}, 0.16f, 4,
                         Fade(BLACK, 0.32f));
    DrawRectangleRounded(r, 0.16f, 4, base);
    DrawRectangleRoundedLinesEx(r, 0.16f, 4, 1.0f, (Color){38, 51, 75, 220});
    DrawRectangle((int)(r.x + 5.0f), (int)r.y + 2, (int)(r.width - 10.0f), 1,
                  (Color){57, 76, 105, 90});
}

static void draw_floor(Rectangle tile, int x, int y)
{
    Color base = ((x + y) & 1) ? (Color){32, 45, 69, 255} : (Color){28, 40, 62, 255};
    Rectangle r = {tile.x + 1.0f, tile.y + 1.0f, tile.width - 2.0f, tile.height - 2.0f};

    DrawRectangleRounded(r, 0.16f, 4, base);
    DrawRectangleRoundedLinesEx(r, 0.16f, 4, 1.0f, (Color){52, 72, 101, 145});
    if (((x * 7 + y * 11) % 9) == 0) {
        DrawCircle((int)(tile.x + tile.width * 0.5f), (int)(tile.y + tile.height * 0.5f),
                   1.2f, (Color){76, 98, 132, 120});
    }
}

static void draw_diamond(Vector2 center, float radius, Color fill, Color outline)
{
    DrawPoly(center, 4, radius, 0.0f, fill);
    DrawPolyLinesEx(center, 4, radius, 0.0f, 1.2f, outline);
}

static void draw_oneway(Rectangle tile, Direction dir, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    float s = tile.width * 0.24f;
    Vector2 a = {c.x - dir_dx(dir) * s * 0.65f, c.y - dir_dy(dir) * s * 0.65f};
    Vector2 b = {c.x + dir_dx(dir) * s, c.y + dir_dy(dir) * s};
    Vector2 left = {b.x - dir_dx(dir) * s * 0.55f - dir_dy(dir) * s * 0.55f,
                    b.y - dir_dy(dir) * s * 0.55f + dir_dx(dir) * s * 0.55f};
    Vector2 right = {b.x - dir_dx(dir) * s * 0.55f + dir_dy(dir) * s * 0.55f,
                     b.y - dir_dy(dir) * s * 0.55f - dir_dx(dir) * s * 0.55f};
    float pulse = 0.7f + 0.12f * sinf(time * 2.4f + (float)(int)(c.x));
    DrawLineEx(a, b, 2.0f, Fade(COL_BLUE, pulse));
    DrawTriangle(b, left, right, Fade(COL_BLUE, pulse));
}

static void draw_locked(Rectangle tile, const Gate *gate, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    float s = tile.width;
    Color color = gate != NULL && gate->open ? COL_GREEN : COL_PURPLE;

    if (gate != NULL && gate->open) {
        DrawCircleV(c, s * 0.22f, Fade(COL_GREEN, 0.12f));
        DrawPolyLinesEx(c, 4, s * 0.25f, 0.0f, 2.0f, Fade(COL_GREEN, 0.9f));
        return;
    }
    DrawRectangleRounded((Rectangle){c.x - s * 0.22f, c.y - s * 0.04f, s * 0.44f, s * 0.29f},
                         0.24f, 5, Fade(COL_PURPLE, 0.22f));
    DrawRectangleRoundedLinesEx((Rectangle){c.x - s * 0.22f, c.y - s * 0.04f, s * 0.44f,
                                            s * 0.29f},
                                0.24f, 5, 1.4f, color);
    DrawRing((Vector2){c.x, c.y - s * 0.06f}, s * 0.12f, s * 0.16f, 190.0f, 350.0f, 18,
             color);
    DrawCircleV((Vector2){c.x, c.y + s * 0.06f}, s * 0.035f, color);
    SP_UNUSED(time);
}

static void draw_special_gate(Rectangle tile, const Gate *gate, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    float s = tile.width;
    float pulse = 0.5f + 0.5f * sinf(time * 3.0f);
    bool open = gate != NULL && gate->open;

    if (open) {
        DrawCircleV(c, s * 0.38f, Fade(COL_PURPLE, 0.10f + 0.08f * pulse));
        DrawCircleV(c, s * 0.28f, Fade(COL_PURPLE, 0.2f));
        DrawRing(c, s * 0.25f, s * 0.32f, 0.0f, 360.0f, 32, Fade(COL_PURPLE, 0.9f));
        DrawRing(c, s * 0.10f, s * 0.14f, 0.0f, 360.0f, 20, Fade(COL_CYAN, 0.9f));
        DrawCircleV(c, s * 0.04f, COL_INK);
    } else {
        draw_diamond(c, s * 0.28f, Fade(COL_PANEL_3, 0.9f), Fade(COL_PURPLE, 0.65f));
        DrawPolyLinesEx(c, 4, s * 0.19f, 0.0f, 1.0f, Fade(COL_PURPLE, 0.4f));
        DrawCircleV(c, s * 0.035f, Fade(COL_PURPLE, 0.55f));
    }
}

static void draw_plate(Rectangle tile, bool pressed, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    float r = tile.width * 0.23f;
    Color color = pressed ? COL_GREEN : COL_CYAN;

    DrawCircleV(c, r, Fade(color, 0.08f));
    DrawRing(c, r * 0.68f, r, 0.0f, 360.0f, 22, Fade(color, 0.85f));
    DrawCircleV(c, r * 0.22f, Fade(color, pressed ? 1.0f : 0.5f + 0.1f * sinf(time * 4.0f)));
}

static void draw_break_floor(Rectangle tile, bool triggered)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    Color color = triggered ? COL_RED : COL_GOLD;
    float s = tile.width;

    DrawLineEx((Vector2){c.x - s * 0.25f, c.y - s * 0.26f},
               (Vector2){c.x - s * 0.03f, c.y - s * 0.02f}, 1.5f, Fade(color, 0.65f));
    DrawLineEx((Vector2){c.x - s * 0.03f, c.y - s * 0.02f},
               (Vector2){c.x + s * 0.2f, c.y + s * 0.02f}, 1.5f, Fade(color, 0.65f));
    DrawLineEx((Vector2){c.x - s * 0.03f, c.y - s * 0.02f},
               (Vector2){c.x + s * 0.03f, c.y + s * 0.28f}, 1.5f, Fade(color, 0.5f));
    DrawLineEx((Vector2){c.x + s * 0.2f, c.y + s * 0.02f},
               (Vector2){c.x + s * 0.3f, c.y + s * 0.18f}, 1.2f, Fade(color, 0.5f));
}

static void draw_seal(Rectangle tile, bool collected, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    float radius = tile.width * 0.24f;
    float pulse = 0.5f + 0.5f * sinf(time * 2.7f);

    if (collected) {
        DrawCircleV(c, tile.width * 0.07f, Fade(COL_PURPLE, 0.22f));
        return;
    }
    DrawCircleV(c, tile.width * 0.34f, Fade(COL_PURPLE, 0.06f + 0.05f * pulse));
    draw_diamond(c, radius, Fade(COL_PURPLE, 0.35f), Fade(COL_PURPLE, 0.95f));
    draw_diamond(c, radius * 0.55f, Fade(COL_INK, 0.85f), Fade(COL_PURPLE, 0.8f));
}

static void draw_food(Rectangle tile, FoodKind kind, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    float pulse = 1.0f + 0.06f * sinf(time * 4.2f + tile.x * 0.05f);
    float r = tile.width * 0.21f * pulse;
    Color color = kind == FOOD_GOLDEN ? COL_GOLD : COL_RED;

    DrawCircleV(c, tile.width * 0.34f, Fade(color, 0.08f));
    if (kind == FOOD_GOLDEN) {
        DrawPoly(c, 5, r, -90.0f, color);
        DrawPolyLinesEx(c, 5, r, -90.0f, 1.0f, Fade(COL_INK, 0.65f));
    } else {
        DrawCircleV(c, r, color);
        DrawCircleV((Vector2){c.x - r * 0.32f, c.y - r * 0.34f}, r * 0.22f,
                    Fade(COL_INK, 0.75f));
        DrawTriangle((Vector2){c.x, c.y - r * 1.05f},
                     (Vector2){c.x + r * 0.6f, c.y - r * 1.5f},
                     (Vector2){c.x + r * 0.25f, c.y - r * 0.9f}, COL_GREEN);
    }
}

static void draw_item(Rectangle tile, ItemType type, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    const ItemDef *def = item_def(type);
    Color color = type == ITEM_KEY ? COL_GOLD
                  : type == ITEM_SHIELD ? COL_CYAN
                  : type == ITEM_PHASE ? COL_PURPLE
                                       : COL_ORANGE;
    float pulse = 0.85f + 0.15f * sinf(time * 3.0f);

    DrawCircleV(c, tile.width * 0.31f, Fade(color, 0.06f));
    draw_diamond(c, tile.width * 0.24f * pulse, Fade(color, 0.2f), color);
    {
        char glyph[2] = {def->glyph, '\0'};
        Vector2 measure = MeasureTextEx(GetFontDefault(), glyph, tile.width * 0.28f,
                                        text_spacing(tile.width * 0.28f));
        DrawTextEx(GetFontDefault(), glyph,
                   (Vector2){c.x - measure.x * 0.5f, c.y - measure.y * 0.5f},
                   tile.width * 0.28f, text_spacing(tile.width * 0.28f), COL_INK);
    }
}

static void draw_block(Rectangle tile)
{
    Rectangle r = {tile.x + tile.width * 0.14f, tile.y + tile.width * 0.16f,
                   tile.width * 0.72f, tile.width * 0.68f};

    DrawRectangleRounded((Rectangle){r.x + 2.0f, r.y + 4.0f, r.width, r.height}, 0.18f, 5,
                         Fade(BLACK, 0.3f));
    DrawRectangleRounded(r, 0.18f, 5, (Color){133, 91, 55, 255});
    DrawRectangleRoundedLinesEx(r, 0.18f, 5, 1.5f, (Color){196, 143, 88, 255});
    DrawRectangle((int)(r.x + 3.0f), (int)(r.y + 4.0f), (int)(r.width - 6.0f), 2,
                  Fade(COL_GOLD, 0.45f));
    DrawLineEx((Vector2){r.x + r.width * 0.22f, r.y + r.height * 0.55f},
               (Vector2){r.x + r.width * 0.52f, r.y + r.height * 0.72f}, 1.0f,
               Fade(BLACK, 0.25f));
}

static void draw_teleport(Rectangle tile, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    float pulse = 0.5f + 0.5f * sinf(time * 3.6f + tile.y * 0.1f);

    DrawCircleV(c, tile.width * 0.33f, Fade(COL_BLUE, 0.07f));
    DrawRing(c, tile.width * (0.11f + 0.04f * pulse), tile.width * 0.16f, 0.0f, 360.0f, 24,
             Fade(COL_BLUE, 0.9f));
    DrawRing(c, tile.width * 0.22f, tile.width * 0.25f, 0.0f, 360.0f, 24,
             Fade(COL_CYAN, 0.35f + 0.25f * pulse));
    DrawCircleV(c, tile.width * 0.045f, Fade(COL_CYAN, 0.9f));
}

static void draw_orb(Rectangle tile, bool collected, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    float pulse = 0.85f + 0.15f * sinf(time * 4.0f + tile.x * 0.1f);

    if (collected) {
        DrawCircleV(c, tile.width * 0.06f, Fade(COL_CYAN, 0.22f));
        return;
    }
    DrawCircleV(c, tile.width * 0.36f, Fade(COL_CYAN, 0.07f));
    DrawCircleV(c, tile.width * 0.17f * pulse, Fade(COL_CYAN, 0.95f));
    DrawRing(c, tile.width * 0.22f, tile.width * 0.25f, 0.0f, 360.0f, 22,
             Fade(COL_INK, 0.55f));
    DrawCircleV((Vector2){c.x - tile.width * 0.06f, c.y - tile.width * 0.07f},
                tile.width * 0.04f, Fade(COL_INK, 0.8f));
}

static void draw_mini_exit(Rectangle tile, const Gate *gate, float time)
{
    Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
    bool open = gate != NULL && gate->open;
    float pulse = 0.5f + 0.5f * sinf(time * 3.4f);

    if (open) {
        DrawCircleV(c, tile.width * 0.38f, Fade(COL_GOLD, 0.12f + 0.08f * pulse));
        DrawCircleV(c, tile.width * 0.27f, Fade(COL_GOLD, 0.18f));
        DrawRing(c, tile.width * 0.18f, tile.width * 0.27f, 0.0f, 360.0f, 28, COL_GOLD);
        DrawRing(c, tile.width * 0.07f, tile.width * 0.10f, 0.0f, 360.0f, 20,
                 Fade(COL_INK, 0.8f));
        draw_diamond(c, tile.width * 0.075f, COL_GOLD, Fade(COL_INK, 0.8f));
    } else {
        DrawCircleV(c, tile.width * 0.32f, Fade(COL_GOLD, 0.04f));
        DrawRectangleRoundedLinesEx(
            (Rectangle){c.x - tile.width * 0.22f, c.y - tile.height * 0.26f,
                        tile.width * 0.44f, tile.height * 0.52f},
            0.28f, 8, 1.2f, Fade(COL_GOLD, 0.4f));
        DrawCircleV(c, tile.width * 0.05f, Fade(COL_GOLD, 0.45f));
    }
}

static void draw_snake_segment(Vector2 center, float size, Color color, bool shadow)
{
    Rectangle r = {center.x - size * 0.5f, center.y - size * 0.5f, size, size};

    if (shadow) {
        DrawRectangleRounded(
            (Rectangle){r.x + 1.5f, r.y + 2.5f, r.width, r.height}, 0.32f, 6,
            Fade(BLACK, 0.3f));
    }
    DrawRectangleRounded(r, 0.32f, 6, color);
    DrawRectangleRoundedLinesEx(r, 0.32f, 6, 1.0f, Fade(COL_GREEN, 0.55f));
    DrawRectangle((int)(r.x + size * 0.18f), (int)(r.y + size * 0.14f),
                  (int)(size * 0.5f), (int)fmaxf(1.0f, size * 0.06f), Fade(COL_INK, 0.22f));
}

static void draw_snake(const Game *game, BoardView view)
{
    int i;
    for (i = game->snake.length - 1; i >= 0; --i) {
        Vector2 c = {
            view.origin_x + ((float)game->snake.body[i].x + 0.5f) * view.tile,
            view.origin_y + ((float)game->snake.body[i].y + 0.5f) * view.tile,
        };
        float ratio = game->snake.length > 1 ? (float)i / (float)(game->snake.length - 1)
                                            : 0.0f;
        Color color = ColorLerp(COL_GREEN_DARK, COL_GREEN, 1.0f - ratio * 0.55f);

        if (i + 1 < game->snake.length) {
            Vector2 next = {
                view.origin_x + ((float)game->snake.body[i + 1].x + 0.5f) * view.tile,
                view.origin_y + ((float)game->snake.body[i + 1].y + 0.5f) * view.tile,
            };
            if (fabsf(c.x - next.x) + fabsf(c.y - next.y) < view.tile * 1.6f) {
                DrawLineEx(c, next, view.tile * 0.56f, color);
            }
        }
        draw_snake_segment(c, view.tile * 0.72f, color, true);
    }

    if (game->snake.length > 0) {
        Vector2 head = {
            view.origin_x + ((float)game->snake.body[0].x + 0.5f) * view.tile,
            view.origin_y + ((float)game->snake.body[0].y + 0.5f) * view.tile,
        };
        Direction dir = game->snake.dir;
        Vector2 forward = {(float)dir_dx(dir), (float)dir_dy(dir)};
        Vector2 side = {-forward.y, forward.x};
        float eye_offset = view.tile * 0.19f;
        Vector2 eye_a = {head.x + side.x * eye_offset + forward.x * view.tile * 0.1f,
                         head.y + side.y * eye_offset + forward.y * view.tile * 0.1f};
        Vector2 eye_b = {head.x - side.x * eye_offset + forward.x * view.tile * 0.1f,
                         head.y - side.y * eye_offset + forward.y * view.tile * 0.1f};
        float eye_radius = fmaxf(1.8f, view.tile * 0.075f);

        DrawCircleV(head, view.tile * 0.46f, Fade(COL_GREEN, 0.09f));
        draw_snake_segment(head, view.tile * 0.84f, COL_GREEN, true);
        if (game->phase == GAME_DEAD) {
            DrawLineEx((Vector2){eye_a.x - 2, eye_a.y - 2},
                       (Vector2){eye_a.x + 2, eye_a.y + 2}, 1.5f, COL_BG);
            DrawLineEx((Vector2){eye_a.x + 2, eye_a.y - 2},
                       (Vector2){eye_a.x - 2, eye_a.y + 2}, 1.5f, COL_BG);
            DrawLineEx((Vector2){eye_b.x - 2, eye_b.y - 2},
                       (Vector2){eye_b.x + 2, eye_b.y + 2}, 1.5f, COL_BG);
            DrawLineEx((Vector2){eye_b.x + 2, eye_b.y - 2},
                       (Vector2){eye_b.x - 2, eye_b.y + 2}, 1.5f, COL_BG);
        } else {
            DrawCircleV(eye_a, eye_radius, COL_INK);
            DrawCircleV(eye_b, eye_radius, COL_INK);
            DrawCircleV((Vector2){eye_a.x + forward.x * 0.8f, eye_a.y + forward.y * 0.8f},
                        eye_radius * 0.45f, COL_BG);
            DrawCircleV((Vector2){eye_b.x + forward.x * 0.8f, eye_b.y + forward.y * 0.8f},
                        eye_radius * 0.45f, COL_BG);
        }
    }
}

static void draw_static_cell(const Game *game, int x, int y, Rectangle tile, float time)
{
    const Cell *cell = board_cell_const(&game->board, x, y);
    Food *food;
    Item *item;
    int seg;

    if (cell == NULL) {
        return;
    }
    if (cell->type == TERRAIN_WALL) {
        draw_wall(tile, x, y);
        return;
    }
    draw_floor(tile, x, y);

    seg = snake_segment(game, x, y);
    if (seg >= 0) {
        return;
    }

    switch (cell->type) {
    case TERRAIN_ONEWAY:
        draw_oneway(tile, (Direction)cell->oneway_dir, time);
        break;
    case TERRAIN_LOCKED:
        if (cell->gate >= 0 && cell->gate < game->gate_count) {
            draw_locked(tile, &game->gates[cell->gate], time);
        }
        break;
    case TERRAIN_SPECIAL_GATE:
        if (cell->gate >= 0 && cell->gate < game->gate_count) {
            draw_special_gate(tile, &game->gates[cell->gate], time);
        }
        break;
    case TERRAIN_BREAK:
        draw_break_floor(tile, cell->break_triggered);
        break;
    case TERRAIN_PIT: {
        Vector2 c = {tile.x + tile.width * 0.5f, tile.y + tile.height * 0.5f};
        DrawCircleV(c, tile.width * 0.29f, (Color){4, 6, 10, 255});
        DrawRing(c, tile.width * 0.28f, tile.width * 0.33f, 0.0f, 360.0f, 22,
                 Fade(COL_RED, 0.35f));
        break;
    }
    case TERRAIN_PLATE:
        if (cell->plate >= 0 && cell->plate < game->plate_count) {
            draw_plate(tile, game->plates[cell->plate].pressed, time);
        }
        break;
    case TERRAIN_SEAL:
        if (cell->seal >= 0 && cell->seal < game->seal_count) {
            draw_seal(tile, game->seals[cell->seal].collected, time);
        }
        break;
    case TERRAIN_TELEPORT:
        draw_teleport(tile, time);
        break;
    case TERRAIN_MINI_EXIT:
        if (cell->gate >= 0 && cell->gate < game->gate_count) {
            draw_mini_exit(tile, &game->gates[cell->gate], time);
        }
        break;
    case TERRAIN_ORB:
        if (cell->orb >= 0 && cell->orb < game->orb_count) {
            draw_orb(tile, game->orbs[cell->orb].collected, time);
        }
        break;
    case TERRAIN_FLOOR:
    default:
        break;
    }

    food = game_food_at((Game *)game, x, y);
    if (food != NULL) {
        draw_food(tile, food->kind, time);
    }
    item = game_item_at((Game *)game, x, y);
    if (item != NULL) {
        draw_item(tile, item->type, time);
    }
    if (game_block_at((Game *)game, x, y) != NULL) {
        draw_block(tile);
    }
}

static void draw_board(const App *app)
{
    const Game *game = app->active;
    BoardView view = board_view(game);
    int x;
    int y;
    float time = (float)GetTime();
    char label[128];

    panel_rect(BOARD_PANEL, COL_PANEL, COL_LINE, 0.018f);
    text(game->kind == GAME_SPECIAL ? "SPECIAL CHAMBER" : "MAIN GRID",
         BOARD_PANEL.x + 18.0f, BOARD_PANEL.y + 11.0f, 12.0f,
         game->kind == GAME_SPECIAL ? COL_GOLD : COL_MUTED);
    if (game->kind == GAME_SPECIAL) {
        snprintf(label, sizeof(label), "ORBS %d/%d  ·  EXIT %s", game->orbs_collected,
                 game->orb_count, game->mini_exit_open ? "OPEN" : "SEALED");
        text(label, BOARD_PANEL.x + BOARD_PANEL.width - 250.0f, BOARD_PANEL.y + 11.0f,
             11.0f, COL_MUTED);
    } else {
        const Gate *gate = NULL;
        int i;
        for (i = 0; i < game->gate_count; ++i) {
            if (game->gates[i].kind == GATE_SPECIAL) {
                gate = &game->gates[i];
                break;
            }
        }
        snprintf(label, sizeof(label), "SEALS %d/%d  ·  GATE %s", game->seals_collected,
                 game->seal_count, gate != NULL && gate->open ? "OPEN" : "SEALED");
        text(label, BOARD_PANEL.x + BOARD_PANEL.width - 250.0f, BOARD_PANEL.y + 11.0f,
             11.0f, gate != NULL && gate->open ? COL_GREEN : COL_MUTED);
    }
    DrawRectangle((int)(BOARD_PANEL.x + 18.0f), (int)(BOARD_PANEL.y + 30.0f),
                  (int)(BOARD_PANEL.width - 36.0f), 1, Fade(COL_LINE, 0.8f));

    DrawRectangleRounded(
        (Rectangle){view.origin_x - 5.0f, view.origin_y - 5.0f,
                    view.tile * (float)game->board.width + 10.0f,
                    view.tile * (float)game->board.height + 10.0f},
        0.012f, 4, Fade(BLACK, 0.22f));

    BeginScissorMode((int)BOARD_PANEL.x, (int)BOARD_PANEL.y,
                     (int)BOARD_PANEL.width, (int)BOARD_PANEL.height);
    for (y = 0; y < game->board.height; ++y) {
        for (x = 0; x < game->board.width; ++x) {
            draw_static_cell(game, x, y, cell_rect(view, x, y), time);
        }
    }
    draw_snake(game, view);
    EndScissorMode();

    DrawRectangleRoundedLinesEx(
        (Rectangle){view.origin_x - 2.0f, view.origin_y - 2.0f,
                    view.tile * (float)game->board.width + 4.0f,
                    view.tile * (float)game->board.height + 4.0f},
        0.01f, 4, 1.0f, Fade(COL_LINE, 0.9f));
}

static void draw_stat_block(const char *label, const char *value, float x, float y,
                            float width, Color color)
{
    text(label, x, y, 10.0f, COL_MUTED);
    text(value, x, y + 15.0f, 14.0f, color);
    SP_UNUSED(width);
}

static void draw_progress_bar(Rectangle rect, int value, int total, Color color)
{
    float ratio = total > 0 ? (float)value / (float)total : 0.0f;
    int i;

    DrawRectangleRounded(rect, 0.5f, 8, COL_PANEL_3);
    if (ratio > 0.0f) {
        DrawRectangleRounded((Rectangle){rect.x, rect.y, rect.width * ratio, rect.height},
                             0.5f, 8, Fade(color, 0.85f));
    }
    for (i = 1; i < total && total <= 8; ++i) {
        float x = rect.x + rect.width * (float)i / (float)total;
        DrawRectangle((int)x, (int)rect.y + 2, 2, (int)rect.height - 4, COL_PANEL);
    }
    DrawRectangleRoundedLinesEx(rect, 0.5f, 8, 1.0f, Fade(color, 0.3f));
}

static void draw_inventory_card(const App *app, float x, float y, ItemType type, int count)
{
    const ItemDef *def = item_def(type);
    Color color = type == ITEM_KEY ? COL_GOLD
                  : type == ITEM_SHIELD ? COL_CYAN
                  : type == ITEM_PHASE ? COL_PURPLE
                                       : COL_ORANGE;
    Rectangle card = {x, y, 80.0f, 58.0f};
    char value[16];

    panel_rect(card, COL_PANEL_2, count > 0 ? Fade(color, 0.45f) : COL_LINE, 0.16f);
    snprintf(value, sizeof(value), "%c", def->glyph);
    text(value, x + 12.0f, y + 10.0f, 18.0f, count > 0 ? color : COL_FAINT);
    snprintf(value, sizeof(value), "%d", count);
    text(value, x + 48.0f, y + 12.0f, 17.0f, count > 0 ? COL_INK : COL_FAINT);
    text(def->name, x + 9.0f, y + 39.0f, 9.0f, COL_MUTED);
    SP_UNUSED(app);
}

static void draw_chains(const App *app, float x, float y)
{
    int i;
    float cursor = x;
    int count = app->in_special ? 0 : app->main_stats.core_atoms;

    text("DEPENDENCY CHAIN", x, y, 10.0f, COL_MUTED);
    y += 17.0f;
    if (app->in_special) {
        char reward[64];
        snprintf(reward, sizeof(reward), "CHAMBER REWARD  +%d",
                 game_special_bonus(&app->special_game));
        panel_rect((Rectangle){x, y, 300.0f, 28.0f}, Fade(COL_GOLD, 0.08f),
                   Fade(COL_GOLD, 0.35f), 0.25f);
        text(reward, x + 13.0f, y + 9.0f, 10.0f, COL_GOLD);
        return;
    }
    for (i = 0; i < count; ++i) {
        const char *name = app->main_stats.atoms[i];
        float width = MeasureTextEx(GetFontDefault(), name, 10.0f, text_spacing(10.0f)).x +
                      22.0f;
        panel_rect((Rectangle){cursor, y, width, 27.0f}, COL_PANEL_2,
                   Fade(COL_GREEN, 0.35f), 0.28f);
        text(name, cursor + 11.0f, y + 9.0f, 10.0f, COL_GREEN);
        cursor += width;
        if (i + 1 < count) {
            text(">", cursor + 7.0f, y + 9.0f, 10.0f, COL_FAINT);
            cursor += 20.0f;
        }
    }
}

static void draw_log(const App *app, float x, float y, float width)
{
    int i;
    int count = app->log_count < 4 ? app->log_count : 4;
    int start = app->log_count > 4 ? app->log_count - 4 : 0;
    char line[72];

    text("EVENT LOG", x, y, 10.0f, COL_MUTED);
    y += 18.0f;
    for (i = 0; i < count; ++i) {
        int absolute = start + i;
        int index = (app->log_head + absolute) % SP_APP_LOG_LINES;
        const AppLogEntry *entry = &app->log[index];
        Color color = event_color(entry->type);

        DrawRectangle((int)x, (int)y + 3, 2, 10, Fade(color, 0.9f));
        truncate_text(entry->text, line, sizeof(line), 48);
        text(line, x + 10.0f, y, 10.0f, i == count - 1 ? COL_INK : COL_MUTED);
        y += 18.0f;
    }
    SP_UNUSED(width);
}

static void draw_sidebar(const App *app)
{
    const Game *game = app->active;
    float x = SIDE_PANEL.x + 22.0f;
    float y = SIDE_PANEL.y + 20.0f;
    float width = SIDE_PANEL.width - 44.0f;
    char buffer[128];
    char score[32];
    char best[32];

    panel_rect(SIDE_PANEL, COL_PANEL, COL_LINE, 0.018f);
    text("RUN STATUS", x, y, 10.0f, COL_MUTED);
    snprintf(score, sizeof(score), "%d", app->main_game.score);
    text(score, x, y + 20.0f, 48.0f, COL_INK);
    text("PTS", x + MeasureTextEx(GetFontDefault(), score, 48.0f, text_spacing(48.0f)).x +
                     8.0f,
         y + 47.0f, 10.0f, COL_FAINT);
    snprintf(best, sizeof(best), "BEST %llu",
             (unsigned long long)sp_max_int((int)app->best_score, app->main_game.score));
    text(best, x + width - 116.0f, y + 23.0f, 11.0f, COL_MUTED);
    if (app->new_best) {
        text("NEW BEST", x + width - 116.0f, y + 40.0f, 10.0f, COL_GOLD);
    }
    y += 84.0f;
    divider(x, y, width);
    y += 15.0f;
    draw_stat_block("LENGTH", TextFormat("%d", game->snake.length), x, y, 90.0f,
                    COL_GREEN);
    draw_stat_block("MOVES", TextFormat("%d", game->moves), x + 105.0f, y, 90.0f,
                    COL_INK);
    draw_stat_block("KEYS", TextFormat("%d", game->keys), x + 210.0f, y, 90.0f,
                    COL_GOLD);

    y += 54.0f;
    if (game->kind == GAME_SPECIAL) {
        snprintf(buffer, sizeof(buffer), "ORBS  %d / %d", game->orbs_collected,
                 game->orb_count);
        text(buffer, x, y, 10.0f, COL_CYAN);
        draw_progress_bar((Rectangle){x, y + 19.0f, width, 9.0f}, game->orbs_collected,
                          game->orb_count, COL_CYAN);
    } else {
        snprintf(buffer, sizeof(buffer), "SEALS  %d / %d", game->seals_collected,
                 game->seal_count);
        text(buffer, x, y, 10.0f, COL_PURPLE);
        draw_progress_bar((Rectangle){x, y + 19.0f, width, 9.0f}, game->seals_collected,
                          game->seal_count, COL_PURPLE);
    }

    y += 45.0f;
    panel_rect((Rectangle){x, y, width, 74.0f}, COL_PANEL_2, Fade(COL_GREEN, 0.22f),
               0.10f);
    text("OBJECTIVE", x + 14.0f, y + 11.0f, 9.0f, COL_MUTED);
    if (game->kind == GAME_SPECIAL) {
        snprintf(buffer, sizeof(buffer), "Collect every orb. Then reach the exit portal.");
    } else {
        const Gate *gate = NULL;
        int i;
        for (i = 0; i < game->gate_count; ++i) {
            if (game->gates[i].kind == GATE_SPECIAL) {
                gate = &game->gates[i];
                break;
            }
        }
        if (gate != NULL && gate->open) {
            snprintf(buffer, sizeof(buffer), "The seal is broken. Reach the special gate.");
        } else {
            snprintf(buffer, sizeof(buffer),
                     "Secure both seals in the dependency chain.");
        }
    }
    text(buffer, x + 14.0f, y + 29.0f, 11.0f, COL_INK);
    text(game->kind == GAME_SPECIAL ? "The chamber closes when you leave."
                                    : "The gate opens only when all seals are held.",
         x + 14.0f, y + 49.0f, 9.0f, COL_MUTED);

    y += 90.0f;
    text("INVENTORY", x, y, 10.0f, COL_MUTED);
    y += 17.0f;
    draw_inventory_card(app, x, y, ITEM_KEY, game->keys);
    draw_inventory_card(app, x + 88.0f, y, ITEM_SHIELD, game->shields);
    draw_inventory_card(app, x + 176.0f, y, ITEM_PHASE, game->phase_tokens);
    draw_inventory_card(app, x + 264.0f, y, ITEM_DOUBLE, game->double_next ? 1 : 0);

    y += 78.0f;
    draw_chains(app, x, y);

    y += 61.0f;
    divider(x, y, width);
    y += 15.0f;
    draw_log(app, x, y, width);

    text("F1  FIELD MANUAL", x, SIDE_PANEL.y + SIDE_PANEL.height - 54.0f, 10.0f,
         COL_FAINT);
    text("ENTER  RUN     ESC  QUIT", x, SIDE_PANEL.y + SIDE_PANEL.height - 36.0f, 10.0f,
         COL_FAINT);
}

static const char *command_placeholder(const App *app)
{
    if (app->in_special) {
        return "d 2; w 1; a 3";
    }
    return "d 3; w 1; a 2; s 1";
}

static void draw_command_bar(const App *app)
{
    bool dead = app->active->phase == GAME_DEAD;
    bool can_type = !app->executing && !dead;
    Color border = app->input_error[0] != '\0' ? COL_RED
                   : can_type                    ? Fade(COL_GREEN, 0.65f)
                                                 : COL_LINE;
    char line[SP_LINE_MAX];

    panel_rect(CMD_PANEL, COL_PANEL, COL_LINE, 0.018f);
    text(app->in_special ? "CHAMBER COMMAND SEQUENCE" : "COMMAND SEQUENCE",
         CMD_PANEL.x + 20.0f, CMD_PANEL.y + 10.0f, 10.0f, COL_MUTED);
    if (app->input_error[0] != '\0') {
        truncate_text(app->input_error, line, sizeof(line), 76);
        text(line, CMD_PANEL.x + 210.0f, CMD_PANEL.y + 10.0f, 10.0f, COL_RED);
    } else if (app->executing) {
        const Command *command =
            app->command_index < app->sequence.count
                ? &app->sequence.commands[app->command_index]
                : NULL;
        snprintf(line, sizeof(line), "EXECUTING  ·  COMMAND %d/%d  ·  %c %d  ·  %d LEFT",
                 app->command_index + 1, app->sequence.count,
                 command != NULL ? dir_to_char(command->dir) : '?',
                 command != NULL ? command->steps : 0, app->steps_left);
        text(line, CMD_PANEL.x + 210.0f, CMD_PANEL.y + 10.0f, 10.0f, COL_GOLD);
    } else if (dead) {
        text("RUN ENDED  ·  PRESS ENTER TO GENERATE A NEW MAP",
             CMD_PANEL.x + 210.0f, CMD_PANEL.y + 10.0f, 10.0f, COL_RED);
    } else {
        text("FORMAT  w 3; d 2; s 1   ·   STEPS 1-9   ·   NO UNDO",
             CMD_PANEL.x + 210.0f, CMD_PANEL.y + 10.0f, 10.0f, COL_FAINT);
    }

    DrawRectangleRounded(INPUT_RECT, 0.14f, 6, Fade(COL_BG_2, 0.85f));
    DrawRectangleRoundedLinesEx(INPUT_RECT, 0.14f, 6, 1.0f, border);
    text(">", INPUT_RECT.x + 13.0f, INPUT_RECT.y + 12.0f, 18.0f,
         can_type ? COL_GREEN : COL_FAINT);

    BeginScissorMode((int)INPUT_RECT.x + 34, (int)INPUT_RECT.y, (int)INPUT_RECT.width - 44,
                     (int)INPUT_RECT.height);
    if (app->input_len > 0) {
        text(app->input, INPUT_RECT.x + 36.0f, INPUT_RECT.y + 11.0f, 19.0f,
             can_type ? COL_INK : COL_MUTED);
    } else if (app->executing) {
        char running[512];
        command_sequence_format(&app->sequence, running, sizeof(running));
        text(running, INPUT_RECT.x + 36.0f, INPUT_RECT.y + 11.0f, 19.0f, COL_FAINT);
    } else if (!dead) {
        text(command_placeholder(app), INPUT_RECT.x + 36.0f, INPUT_RECT.y + 11.0f, 19.0f,
             COL_FAINT);
    }
    if (can_type && ((int)(GetTime() * 2.0) % 2) == 0) {
        Vector2 measure =
            MeasureTextEx(GetFontDefault(), app->input, 19.0f, text_spacing(19.0f));
        float caret_x = INPUT_RECT.x + 36.0f + (app->input_len > 0 ? measure.x : 0.0f);
        DrawRectangle((int)caret_x, (int)INPUT_RECT.y + 12, 2, 19, COL_GREEN);
    }
    EndScissorMode();

    {
        bool hover = CheckCollisionPointRec(GetMousePosition(), RUN_RECT);
        Color fill = !can_type ? COL_PANEL_3 : hover ? (Color){110, 242, 158, 255}
                                                      : COL_GREEN;
        Color ink = can_type ? COL_BG : COL_FAINT;
        DrawRectangleRounded(RUN_RECT, 0.18f, 6, fill);
        text_center(dead ? "NEW MAP" : "RUN", RUN_RECT.x + RUN_RECT.width * 0.5f,
                    RUN_RECT.y + 12.0f, 13.0f, ink);
        if (can_type && !dead) {
            text("ENTER", RUN_RECT.x + 103.0f, RUN_RECT.y + 15.0f, 8.0f, Fade(COL_BG, 0.7f));
        }
    }
    {
        bool hover = CheckCollisionPointRec(GetMousePosition(), CLEAR_RECT);
        DrawRectangleRounded(CLEAR_RECT, 0.18f, 6, hover ? COL_PANEL_3 : COL_PANEL_2);
        DrawRectangleRoundedLinesEx(CLEAR_RECT, 0.18f, 6, 1.0f, COL_LINE);
        text_center("CLR", CLEAR_RECT.x + CLEAR_RECT.width * 0.5f, CLEAR_RECT.y + 12.0f,
                    11.0f, can_type ? COL_MUTED : COL_FAINT);
    }

    if (app->executing && app->sequence.count > 0) {
        float ratio = (float)(app->command_index + 1) / (float)app->sequence.count;
        DrawRectangleRounded((Rectangle){CMD_PANEL.x + 20.0f,
                                         CMD_PANEL.y + CMD_PANEL.height - 4.0f,
                                         (CMD_PANEL.width - 40.0f) * ratio, 2.0f},
                             0.5f, 4, Fade(COL_GOLD, 0.8f));
    }
}

static void draw_legend_row(float x, float y, const char *glyph, const char *label,
                            Color color)
{
    Rectangle chip = {x, y - 4.0f, 25.0f, 25.0f};
    DrawRectangleRounded(chip, 0.18f, 5, Fade(color, 0.10f));
    DrawRectangleRoundedLinesEx(chip, 0.18f, 5, 1.0f, Fade(color, 0.4f));
    text_center(glyph, x + 12.5f, y + 3.0f, 12.0f, color);
    text(label, x + 35.0f, y + 4.0f, 11.0f, COL_MUTED);
}

static void draw_help_overlay(void)
{
    Rectangle p = {230.0f, 100.0f, 900.0f, 700.0f};
    float x = p.x + 34.0f;
    float y = p.y + 28.0f;

    DrawRectangle(0, 0, WIN_W, WIN_H, Fade(BLACK, 0.78f));
    panel_rect(p, COL_PANEL, Fade(COL_GREEN, 0.45f), 0.018f);
    text("FIELD MANUAL", x, y, 28.0f, COL_INK);
    text("Everything is deterministic except the next map.", x, y + 37.0f, 11.0f,
         COL_MUTED);
    DrawRectangle((int)x, (int)y + 64, (int)(p.width - 68.0f), 1, COL_LINE);
    y += 84.0f;
    text("COMMAND SYNTAX", x, y, 10.0f, COL_GREEN);
    DrawTextEx(GetFontDefault(), "w 3; d 2; s 1; a 4", (Vector2){x, y + 18.0f}, 22.0f,
               3.2f, COL_INK);
    text("w a s d = up left down right  ·  each step count is one digit 1-9",
         x, y + 51.0f, 11.0f, COL_MUTED);
    text("The whole sequence executes in order. You cannot interrupt, undo, or",
         x, y + 70.0f, 11.0f, COL_MUTED);
    text("cancel it. A reverse move, wall, or your own body is fatal.",
         x, y + 87.0f, 11.0f, COL_MUTED);

    y += 120.0f;
    text("SYMBOLS", x, y, 10.0f, COL_GREEN);
    y += 22.0f;
    draw_legend_row(x, y, "@", "head", COL_GREEN);
    draw_legend_row(x + 210.0f, y, "S", "seal", COL_PURPLE);
    draw_legend_row(x + 420.0f, y, "D", "special gate", COL_PURPLE);
    draw_legend_row(x + 620.0f, y, "L", "locked door", COL_PURPLE);
    y += 36.0f;
    draw_legend_row(x, y, "B", "pushable stone", COL_GOLD);
    draw_legend_row(x + 210.0f, y, "_", "pressure plate", COL_CYAN);
    draw_legend_row(x + 420.0f, y, "~", "fracture tile", COL_GOLD);
    draw_legend_row(x + 620.0f, y, "T", "teleporter", COL_BLUE);
    y += 36.0f;
    draw_legend_row(x, y, "<", "one-way gate", COL_BLUE);
    draw_legend_row(x + 210.0f, y, "K", "key", COL_GOLD);
    draw_legend_row(x + 420.0f, y, "A", "shield", COL_CYAN);
    draw_legend_row(x + 620.0f, y, "P", "phase charge", COL_PURPLE);
    y += 36.0f;
    draw_legend_row(x, y, "2", "double fruit", COL_ORANGE);
    draw_legend_row(x + 210.0f, y, "*", "fruit", COL_RED);
    draw_legend_row(x + 420.0f, y, "O", "chamber orb", COL_CYAN);
    y += 48.0f;

    text("RUN STRUCTURE", x, y, 10.0f, COL_GREEN);
    text("Every generated map joins two verified puzzle atoms into a dependency chain.",
         x, y + 20.0f, 11.0f, COL_MUTED);
    text("Optional loops hold fruit and items. Solve both atoms, take their seals, and",
         x, y + 38.0f, 11.0f, COL_MUTED);
    text("step into the special gate. The chamber inside awards a bonus and returns you",
         x, y + 56.0f, 11.0f, COL_MUTED);
    text("to the exact interrupted point. Death ends the run and rolls a new map.",
         x, y + 74.0f, 11.0f, COL_MUTED);

    {
        float col_x = p.x + 470.0f;
        float block_y = p.y + 552.0f;
        text("SCORING", x, block_y, 10.0f, COL_GREEN);
        text("fruit +10   golden +30   item +5", x, block_y + 20.0f, 11.0f, COL_MUTED);
        text("seal +25 (+15 per seal already held)", x, block_y + 38.0f, 11.0f,
             COL_MUTED);
        text("chamber clear +100 (+15 per orb)", x, block_y + 56.0f, 11.0f, COL_MUTED);
        text("THREAT MODEL", col_x, block_y, 10.0f, COL_GREEN);
        text("wall / self / reverse = death", col_x, block_y + 20.0f, 11.0f, COL_MUTED);
        text("break floors collapse after the tail leaves", col_x, block_y + 38.0f, 11.0f,
             COL_MUTED);
        text("a shield absorbs one fatal hit and stays put", col_x, block_y + 56.0f, 11.0f,
             COL_MUTED);
    }
    text("F1  CLOSE MANUAL", x, p.y + p.height - 38.0f, 10.0f, COL_FAINT);
}

static void draw_game_over(const App *app)
{
    Rectangle p = {360.0f, 270.0f, 640.0f, 330.0f};
    char cause[128];
    char score[64];
    char best[64];

    DrawRectangle(0, 0, WIN_W, WIN_H, Fade(BLACK, 0.68f));
    panel_rect(p, (Color){16, 20, 32, 250}, Fade(COL_RED, 0.55f), 0.02f);
    text_center("RUN TERMINATED", p.x + p.width * 0.5f, p.y + 29.0f, 13.0f, COL_RED);
    snprintf(cause, sizeof(cause), "%s", app->active->death_reason);
    text_center(cause, p.x + p.width * 0.5f, p.y + 58.0f, 20.0f, COL_INK);
    DrawRectangle((int)(p.x + 70.0f), (int)p.y + 100.0f, (int)p.width - 140, 1, COL_LINE);
    snprintf(score, sizeof(score), "%d", app->main_game.score);
    text_center(score, p.x + p.width * 0.5f, p.y + 118.0f, 64.0f, COL_GREEN);
    text_center("FINAL SCORE", p.x + p.width * 0.5f, p.y + 188.0f, 10.0f, COL_MUTED);
    snprintf(best, sizeof(best), "BEST  %llu", (unsigned long long)app->best_score);
    text_center(best, p.x + p.width * 0.5f, p.y + 216.0f, 12.0f, COL_MUTED);
    if (app->new_best) {
        Rectangle badge = {p.x + p.width * 0.5f - 55.0f, p.y + 238.0f, 110.0f, 24.0f};
        DrawRectangleRounded(badge, 0.5f, 8, Fade(COL_GOLD, 0.16f));
        text_center("NEW BEST", p.x + p.width * 0.5f, p.y + 244.0f, 9.0f, COL_GOLD);
    }
    DrawRectangle((int)(p.x + 70.0f), (int)p.y + 270.0f, (int)p.width - 140, 1, COL_LINE);
    text_center("ENTER  NEW MAP", p.x + p.width * 0.5f - 100.0f, p.y + 292.0f, 10.0f,
                COL_GREEN);
    text_center("ESC  QUIT", p.x + p.width * 0.5f + 112.0f, p.y + 292.0f, 10.0f, COL_MUTED);
}

void raylib_render_app(const App *app)
{
    const Game *game = app->active;

    draw_background();
    draw_logo();
    draw_header_meta(app);
    draw_board(app);
    draw_sidebar(app);
    draw_command_bar(app);
    if (game->phase == GAME_DEAD) {
        draw_game_over(app);
    }
    if (app->show_help) {
        draw_help_overlay();
    }
}

static void handle_input(App *app)
{
    int codepoint;

    if (IsKeyPressed(KEY_F1)) {
        app->show_help = !app->show_help;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (app->show_help) {
            app->show_help = false;
        } else {
            app->quit = true;
        }
        return;
    }
    if (app->show_help) {
        return;
    }
    if (app->active->phase == GAME_DEAD) {
        bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                       CheckCollisionPointRec(GetMousePosition(), RUN_RECT);
        if (IsKeyPressed(KEY_ENTER) || clicked) {
            app_handle_restart(app);
        }
        return;
    }
    if (app->executing) {
        return;
    }
    while ((codepoint = GetCharPressed()) > 0) {
        app_input_append(app, codepoint);
    }
    if (IsKeyPressed(KEY_BACKSPACE)) {
        app_input_backspace(app);
    }
    if (IsKeyPressed(KEY_ENTER)) {
        app_submit_command(app);
        return;
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, RUN_RECT)) {
            app_submit_command(app);
        } else if (CheckCollisionPointRec(mouse, CLEAR_RECT)) {
            app_input_clear(app);
        }
    }
}

int raylib_run_app(App *app)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(WIN_W, WIN_H, "Snake Puzzle");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    SetTextureFilter(GetFontDefault().texture, TEXTURE_FILTER_POINT);
    SetWindowMinSize(1100, 760);

    while (!app->quit && !WindowShouldClose()) {
        handle_input(app);
        app_tick(app, GetFrameTime());
        BeginDrawing();
        raylib_render_app(app);
        EndDrawing();

        if (app->screenshot_once && app->elapsed > 0.22) {
            TakeScreenshot(app->screenshot_path);
            app->screenshot_once = false;
        }
        if (app->auto_exit_after > 0.0 && app->elapsed >= app->auto_exit_after) {
            break;
        }
    }
    CloseWindow();
    return 0;
}
