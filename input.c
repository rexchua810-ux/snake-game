// input.c — 键盘输入：开始画面的选项、W/A/S/D 控制蛇

#include "game.h"

// ================= 开始画面 =================

// ★ 修改一个数字：打数字会接在后面，Backspace 删掉最后一位
int editNumber(int value)
{
    int c = GetCharPressed();
    while (c > 0)
    {
        if (c >= '0' && c <= '9')
        {
            value = value * 10 + (c - '0');
            if (value > 99)              // 超过两位数就从这个数字重新开始
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

// ★ 开始画面的按键
void updateMenu(Game *g)
{
    // 上下选项目
    if (IsKeyPressed(KEY_UP) && g->menuSelected > 0)
        g->menuSelected--;
    if (IsKeyPressed(KEY_DOWN) && g->menuSelected < 2)
        g->menuSelected++;

    // 修改目前选到的项目
    if (g->menuSelected == 0)
        g->width = editNumber(g->width);
    else if (g->menuSelected == 1)
        g->height = editNumber(g->height);
    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_RIGHT))
        g->mode = 1 - g->mode;           // 0 变 1、1 变 0

    // 按 Enter：大小合格才开始
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

// ================= 蛇的操控 =================

// ★ 按下方向键：先放进排队，等蛇真的走下一步时才转弯
//   跟“排在最后的方向”比较：一样的不用排，相反的（掉头）不允许
void setDirection(Game *g, int dx, int dy)
{
    Snake *s = &g->snake;

    int lastX = s->dirX, lastY = s->dirY;
    if (s->queueCount > 0)
    {
        lastX = s->queueX[s->queueCount - 1];
        lastY = s->queueY[s->queueCount - 1];
    }

    if (dx == lastX && dy == lastY)       // 同方向
        return;
    if (dx == -lastX && dy == -lastY)     // 掉头
        return;
    if (s->queueCount >= 2)               // 排满了
        return;

    s->queueX[s->queueCount] = dx;
    s->queueY[s->queueCount] = dy;
    s->queueCount++;
}

// ★ 走下一步之前：从排队里拿出一个方向来用
void takeQueuedDirection(Snake *s)
{
    if (s->queueCount == 0)
        return;
    s->dirX = s->queueX[0];
    s->dirY = s->queueY[0];
    s->queueX[0] = s->queueX[1];          // 后面的往前移
    s->queueY[0] = s->queueY[1];
    s->queueCount--;
}

// 读 W/A/S/D
void handleInput(Game *g)
{
    if (IsKeyPressed(KEY_W)) setDirection(g, 0, -1);
    if (IsKeyPressed(KEY_S)) setDirection(g, 0, 1);
    if (IsKeyPressed(KEY_A)) setDirection(g, -1, 0);
    if (IsKeyPressed(KEY_D)) setDirection(g, 1, 0);
}
