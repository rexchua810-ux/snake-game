// input.c — keyboard input: start menu options, W/A/S/D snake control
// AI disclosure: parts of this file were written with the help of AI tools (Claude / Claude Code).
//   The author reviewed the code, tested it in the game, and can explain how it works.
//   See "Use of Generative AI" in README.md for details.

#include "game.h"

// ================= Start menu =================

// edit a number: typed digits are appended, Backspace deletes the last digit
int editNumber(int value)
{
    int c = GetCharPressed();
    while (c > 0)
    {
        if (c >= '0' && c <= '9')
        {
            value = value * 10 + (c - '0');
            if (value > 99)              // more than two digits: start again from this digit
                value = c - '0';
        }
        c = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE))
        value = value / 10;
    if (IsKeyPressed(KEY_RIGHT))
        value++;
    if (IsKeyPressed(KEY_LEFT) && value > 0)
        value--;
    return value;
}

// keys on the start menu
void updateMenu(Game *g)
{
    // move between rows
    if (IsKeyPressed(KEY_UP) && g->menuSelected > 0)
        g->menuSelected--;
    if (IsKeyPressed(KEY_DOWN) && g->menuSelected < 2)
        g->menuSelected++;

    // change the selected row
    if (g->menuSelected == 0)
        g->width = editNumber(g->width);
    else if (g->menuSelected == 1)
        g->height = editNumber(g->height);
    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
        g->mode = 1 - g->mode;           // 0 becomes 1, 1 becomes 0

    // Enter: start only if the size is valid
    if (IsKeyPressed(KEY_ENTER))
    {
        if (g->width < MIN_SIZE || g->width > MAX_SIZE ||
            g->height < MIN_SIZE || g->height > MAX_SIZE)
            g->menuError = "Size must be 5 - 30";
        else
        {
            g->menuError = "";
            startGame(g);
        }
    }
}

// ================= Snake control =================

// direction key pressed: queue it, the snake turns on its next real step
//   compared with the last queued direction: the same direction is skipped, the opposite one (U-turn) is not allowed
void setDirection(Game *g, int dx, int dy)
{
    Snake *s = &g->snake;

    int lastX = s->dirX, lastY = s->dirY;
    if (s->queueCount > 0)
    {
        lastX = s->queueX[s->queueCount - 1];
        lastY = s->queueY[s->queueCount - 1];
    }

    if (dx == lastX && dy == lastY)       // same direction
        return;
    if (dx == -lastX && dy == -lastY)     // U-turn
        return;
    if (s->queueCount >= 2)               // queue is full
        return;

    s->queueX[s->queueCount] = dx;
    s->queueY[s->queueCount] = dy;
    s->queueCount++;
}

// before the next step: take one direction from the queue
void takeQueuedDirection(Snake *s)
{
    if (s->queueCount == 0)
        return;
    s->dirX = s->queueX[0];
    s->dirY = s->queueY[0];
    s->queueX[0] = s->queueX[1];          // shift the rest forward
    s->queueY[0] = s->queueY[1];
    s->queueCount--;
}

// read W/A/S/D
void handleInput(Game *g)
{
    if (IsKeyPressed(KEY_W)) setDirection(g, 0, -1);
    if (IsKeyPressed(KEY_S)) setDirection(g, 0, 1);
    if (IsKeyPressed(KEY_A)) setDirection(g, -1, 0);
    if (IsKeyPressed(KEY_D)) setDirection(g, 1, 0);
}
