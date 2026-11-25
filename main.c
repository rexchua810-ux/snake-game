#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "raylib.h"

#define WINDOW_SIZE 600     // 窗口最长的那一边大约 600 像素
#define MOVE_TIME   0.15f   // 每 0.15 秒走一格（越小越快）
#define WIN_SCORE   20      // 吃到 20 个食物就赢

// 地图格子里的东西
#define EMPTY    0
#define FOOD     1
#define OBSTACLE 2

// 游戏状态
#define PLAYING 0
#define LOSE    1
#define WIN     2

// 蛇的资料
typedef struct {
    int *x;          // 每一节的 x，x[0] 是头
    int *y;          // 每一节的 y
    int length;      // 现在有几节
    int dirX, dirY;  // 移动方向
} Snake;

// 整个游戏的资料都放在这里
typedef struct {
    int **map;           // 地图（malloc 来的二维数组）
    int width, height;   // 地图大小（格数）
    int cell;            // 每一格画多大（像素）
    Snake snake;
    int score;           // 吃了几个食物
    int state;           // PLAYING / LOSE / WIN
    float timer;         // 计时器，控制蛇多久走一格
} Game;

// 让玩家输入地图大小（宽和高都至少 5）
void getMapSize(int *width, int *height)
{
    do {
        printf("Map width (>= 5): ");
        scanf("%d", width);
        printf("Map height (>= 5): ");
        scanf("%d", height);
    } while (*width < 5 || *height < 5);
}

// 用 malloc 建立 height 行、width 格的地图
int **createMap(int width, int height)
{
    int **map = malloc(height * sizeof(int *));
    for (int y = 0; y < height; y++)
        map[y] = malloc(width * sizeof(int));
    return map;
}

// 释放地图：先还每一行，再还整个 map
void freeMap(int **map, int height)
{
    for (int y = 0; y < height; y++)
        free(map[y]);
    free(map);
}

// 检查 (x, y) 这一格有没有蛇，有就回传 1
int isOnSnake(Snake *s, int x, int y)
{
    for (int i = 0; i < s->length; i++)
        if (s->x[i] == x && s->y[i] == y)
            return 1;
    return 0;
}

// 在随机的空格上放一个东西
void placeItem(Game *g, int type)
{
    int x, y;
    do {
        x = rand() % g->width;
        y = rand() % g->height;
    } while (g->map[y][x] != EMPTY
             || isOnSnake(&g->snake, x, y)
             || (type == OBSTACLE && y == g->height / 2));  // 障碍物不放在蛇出发的那一行
    g->map[y][x] = type;
}

// 开始新的一局：清空地图、蛇回到起点、重新放障碍物和食物
void resetGame(Game *g)
{
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            g->map[y][x] = EMPTY;

    g->snake.length = 3;
    g->snake.dirX = 1;
    g->snake.dirY = 0;
    for (int i = 0; i < g->snake.length; i++)
    {
        g->snake.x[i] = g->width / 2 - i;
        g->snake.y[i] = g->height / 2;
    }

    g->score = 0;
    g->state = PLAYING;
    g->timer = 0;

    int obstacleCount = g->width * g->height / 20;
    for (int i = 0; i < obstacleCount; i++)
        placeItem(g, OBSTACLE);
    placeItem(g, FOOD);
}

// 建立游戏：要内存（只做一次），然后开始第一局
void initGame(Game *g, int width, int height)
{
    g->width = width;
    g->height = height;

    // 算出每格多大，让窗口最长的那一边约 600 像素
    if (width > height)
        g->cell = WINDOW_SIZE / width;
    else
        g->cell = WINDOW_SIZE / height;

    g->map = createMap(width, height);
    g->snake.x = malloc(width * height * sizeof(int));  // 蛇最长不会超过整张地图
    g->snake.y = malloc(width * height * sizeof(int));

    resetGame(g);
}

// 释放所有 malloc 来的内存
void freeGame(Game *g)
{
    freeMap(g->map, g->height);
    free(g->snake.x);
    free(g->snake.y);
}

// 改方向：如果新方向会走到“脖子”（第 2 节）的位置，就是掉头，不允许
void setDirection(Snake *s, int dx, int dy)
{
    if (s->x[0] + dx == s->x[1] && s->y[0] + dy == s->y[1])
        return;
    s->dirX = dx;
    s->dirY = dy;
}

// 读 W/A/S/D
void handleInput(Snake *s)
{
    if (IsKeyPressed(KEY_W)) setDirection(s, 0, -1);
    if (IsKeyPressed(KEY_S)) setDirection(s, 0, 1);
    if (IsKeyPressed(KEY_A)) setDirection(s, -1, 0);
    if (IsKeyPressed(KEY_D)) setDirection(s, 1, 0);
}

// 蛇走一格：先检查会不会撞到，再看有没有吃到食物
void stepGame(Game *g)
{
    Snake *s = &g->snake;
    int newX = s->x[0] + s->dirX;   // 头下一步的位置
    int newY = s->y[0] + s->dirY;

    // 1. 撞墙
    if (newX < 0 || newX >= g->width || newY < 0 || newY >= g->height)
    {
        g->state = LOSE;
        return;
    }

    // 2. 撞障碍物
    if (g->map[newY][newX] == OBSTACLE)
    {
        g->state = LOSE;
        return;
    }

    int ate = (g->map[newY][newX] == FOOD);   // 有没有吃到食物（1 或 0）

    // 3. 撞到自己（没吃到食物时尾巴会移开，所以尾巴那一格不算）
    int checkLength = s->length;
    if (!ate)
        checkLength = s->length - 1;
    for (int i = 0; i < checkLength; i++)
    {
        if (s->x[i] == newX && s->y[i] == newY)
        {
            g->state = LOSE;
            return;
        }
    }

    // 4. 吃到食物：长度 +1、分数 +1、把食物从地图上拿掉
    if (ate)
    {
        s->length++;
        g->score++;
        g->map[newY][newX] = EMPTY;
    }

    // 5. 移动：从尾巴开始，每一节移到前一节的位置，头移到新位置
    for (int i = s->length - 1; i > 0; i--)
    {
        s->x[i] = s->x[i - 1];
        s->y[i] = s->y[i - 1];
    }
    s->x[0] = newX;
    s->y[0] = newY;

    // 6. 吃到食物后：够分数就赢，不够就再放一个新食物
    if (ate)
    {
        if (g->score >= WIN_SCORE)
            g->state = WIN;
        else
            placeItem(g, FOOD);
    }
}

// 每一帧的更新；回传 1 表示玩家按 Q 要退出
int updateGame(Game *g)
{
    // 游戏结束时：R 重新开始、Q 退出
    if (g->state != PLAYING)
    {
        if (IsKeyPressed(KEY_R))
            resetGame(g);
        if (IsKeyPressed(KEY_Q))
            return 1;
        return 0;
    }

    handleInput(&g->snake);

    g->timer += GetFrameTime();
    if (g->timer >= MOVE_TIME)
    {
        stepGame(g);
        g->timer = 0;
    }
    return 0;
}

// 画一格
void drawCell(int x, int y, int cell, Color color)
{
    DrawRectangle(x * cell, y * cell, cell, cell, color);
}

// 在画面左右置中的位置写字
void drawCenterText(const char *text, int y, int size, Color color)
{
    int textWidth = MeasureText(text, size);
    DrawText(text, (GetScreenWidth() - textWidth) / 2, y, size, color);
}

// 画出整个画面
void drawGame(Game *g)
{
    BeginDrawing();
    ClearBackground(BLACK);

    // 地图
    for (int y = 0; y < g->height; y++)
    {
        for (int x = 0; x < g->width; x++)
        {
            if (g->map[y][x] == FOOD)
                drawCell(x, y, g->cell, RED);
            else if (g->map[y][x] == OBSTACLE)
                drawCell(x, y, g->cell, GRAY);
            DrawRectangleLines(x * g->cell, y * g->cell, g->cell, g->cell, DARKGRAY);
        }
    }

    // 蛇：头亮绿色，身体绿色
    for (int i = 0; i < g->snake.length; i++)
    {
        if (i == 0)
            drawCell(g->snake.x[i], g->snake.y[i], g->cell, LIME);
        else
            drawCell(g->snake.x[i], g->snake.y[i], g->cell, GREEN);
    }

    // 左上角显示分数
    DrawText(TextFormat("Score: %d / %d", g->score, WIN_SCORE), 10, 10, 20, WHITE);

    // 结束画面
    if (g->state != PLAYING)
    {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.6f));  // 半透明黑底
        int midY = GetScreenHeight() / 2;
        if (g->state == WIN)
            drawCenterText("You Win!", midY - 40, 40, GOLD);
        else
            drawCenterText("Game Over", midY - 40, 40, RED);
        drawCenterText("R: Restart   Q: Quit", midY + 20, 20, WHITE);
    }

    EndDrawing();
}

int main(void)
{
    int width, height;
    getMapSize(&width, &height);
    srand(time(NULL));

    Game game;
    initGame(&game, width, height);

    InitWindow(width * game.cell, height * game.cell, "Snake");
    SetTargetFPS(60);

    int quit = 0;
    while (!WindowShouldClose() && !quit)
    {
        quit = updateGame(&game);
        drawGame(&game);
    }

    CloseWindow();
    freeGame(&game);
    return 0;
}