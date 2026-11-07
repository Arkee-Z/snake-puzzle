#ifndef SNAKE_PUZZLE_UI_RENDER_RAYLIB_H
#define SNAKE_PUZZLE_UI_RENDER_RAYLIB_H

#include "snake_puzzle/app.h"

struct Rectangle;

void raylib_render_app(const App *app);
int raylib_run_app(App *app);

#endif
