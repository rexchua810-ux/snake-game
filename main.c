#include <stdlib.h>
#include <time.h>
#include "raylib.h"

// ===== 画面大小 =====
#define SCREEN_W 600        // 窗口宽
#define HUD_H    60         // 上方显示分数的区域高度
#define BOARD    600        // 地图区域最大 600x600
#define SCREEN_H (HUD_H + BOARD)

// ===== 游戏数值（录视频“现场改数值”可以改这些） =====
#define MOVE_TIME     0.15f // 每 0.15 秒走一格（越小越快）
#define WIN_SCORE     20    // 吃到 20 个食物就赢
#define OBSTACLE_RATE 50    // 大约每 50 格放 1 个障碍物（越大越少）
#define BOOST_TIME    5.0f  // 加速持续几秒
#define SHRINK_AMOUNT 3     // 缩短几节
#define MIN_LENGTH    3     // 蛇最短几节
#define START_LIVES   1     // 一开始有几条命
#define MESSAGE_TIME  1.5f  // 提示文字显示几秒

// ===== 地图大小的范围和默认值 =====
#define MIN_SIZE     5
#define MAX_SIZE     30
#define DEFAULT_W    15
#define DEFAULT_H    12

// 地图格子里的东西
#define EMPTY    0
#define FOOD     1
#define OBSTACLE 2
#define BOOSTER  3

// 游戏状态
#define PLAYING 0
#define LOSE    1
#define WIN     2
#define MENU    3           // ★ 开始画面

// ★ 游戏模式
#define CLASSIC  0          // 经典模式：撞墙会失去一条命
#define NO_WALLS 1          // 无边界模式：从对面出来

// 道具种类
#define SPEED_BOOST  0
#define SIZE_REDUCER 1
#define EXTRA_LIFE   2

// 蛇的资料
typedef struct {
    int *x;          // 每一节的 x，x[0] 是头
    int *y;          // 每一节的 y
    int length;      // 现在有几节
    int dirX, dirY;  // 移动方向
} Snake;

// 整个游戏的资料都放在这里
typedef struct {
    int **map;           // 地图（malloc 来的二维数组），还没开始时是 NULL
    int width, height;   // 地图大小（格数）
    int cell;            // 每一格画多大（像素）
    int offsetX, offsetY;// ★ 地图画在窗口的哪里（让地图置中）
    int mode;            // ★ CLASSIC / NO_WALLS
    Snake snake;
    int score;
    int state;           // PLAYING / LOSE / WIN / MENU
    float timer;
    int lives;
    float speedTimer;
    const char *message;
    float messageTimer;
    int menuSelected;    // ★ 开始画面目前选到第几项（0 宽、1 高、2 模式）
    const char *menuError; // ★ 开始画面的错误提示
} Game;

// ================= 地图 =================

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

// 检查地图上还有没有可以放东西的空格（避免地图满了卡死）
int hasFreeCell(Game *g)
{
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            if (g->map[y][x] == EMPTY && !isOnSnake(&g->snake, x, y))
                return 1;
    return 0;
}

// 在随机的空格上放一个东西
void placeItem(Game *g, int type)
{
    if (!hasFreeCell(g))
        return;

    int x, y;
    do {
        x = rand() % g->width;
        y = rand() % g->height;
    } while (g->map[y][x] != EMPTY
             || isOnSnake(&g->snake, x, y)
             || (type == OBSTACLE && y == g->height / 2));  // 障碍物不放在蛇出发的那一行
    g->map[y][x] = type;
}

// ================= 游戏的开始与结束 =================

// 显示一段提示文字（例如 "Speed Up!"）
void showMessage(Game *g, const char *text)
{
    g->message = text;
    g->messageTimer = MESSAGE_TIME;
}

// 把蛇放回起点（长度 3，往右走）
void resetSnake(Game *g)
{
    g->snake.length = 3;
    g->snake.dirX = 1;
    g->snake.dirY = 0;
    for (int i = 0; i < g->snake.length; i++)
    {
        g->snake.x[i] = g->width / 2 - i;
        g->snake.y[i] = g->height / 2;
    }
}

// 开始新的一局（地图大小不变）
void resetGame(Game *g)
{
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            g->map[y][x] = EMPTY;

    resetSnake(g);

    g->score = 0;
    g->state = PLAYING;
    g->timer = 0;
    g->lives = START_LIVES;
    g->speedTimer = 0;
    g->message = "";
    g->messageTimer = 0;

    int obstacleCount = g->width * g->height / OBSTACLE_RATE;
    for (int i = 0; i < obstacleCount; i++)
        placeItem(g, OBSTACLE);
    placeItem(g, FOOD);
    placeItem(g, BOOSTER);
}

// ★ 从开始画面按下 Enter：依照玩家选的大小要内存，然后开始游戏
void startGame(Game *g)
{
    // 算出每格多大，让地图塞进 600x600 的区域
    if (g->width > g->height)
        g->cell = BOARD / g->width;
    else
        g->cell = BOARD / g->height;

    // 让地图在区域里置中
    g->offsetX = (SCREEN_W - g->width * g->cell) / 2;
    g->offsetY = HUD_H + (BOARD - g->height * g->cell) / 2;

    g->map = createMap(g->width, g->height);
    g->snake.x = malloc(g->width * g->height * sizeof(int));  // 蛇最长不会超过整张地图
    g->snake.y = malloc(g->width * g->height * sizeof(int));

    resetGame(g);
}

// 释放所有 malloc 来的内存
void freeGame(Game *g)
{
    if (g->map == NULL)      // 还没开始游戏，没有东西要释放
        return;
    freeMap(g->map, g->height);
    free(g->snake.x);
    free(g->snake.y);
    g->map = NULL;
}

// ★ 程序一开始：设定默认值，停在开始画面
void initGame(Game *g)
{
    g->map = NULL;
    g->width = DEFAULT_W;
    g->height = DEFAULT_H;
    g->mode = CLASSIC;
    g->state = MENU;
    g->menuSelected = 0;
    g->menuError = "";
}

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

// ★ 把整条蛇倒过来：第一节和最后一节交换、第二节和倒数第二节交换……
void reverseSnake(Snake *s)
{
    for (int i = 0; i < s->length / 2; i++)
    {
        int j = s->length - 1 - i;
        int tempX = s->x[i];
        int tempY = s->y[i];
        s->x[i] = s->x[j];
        s->y[i] = s->y[j];
        s->x[j] = tempX;
        s->y[j] = tempY;
    }
}

// ★ 改方向：如果是往“脖子”（第 2 节）的方向按，就把蛇整条倒过来
void setDirection(Game *g, int dx, int dy)
{
    Snake *s = &g->snake;

    // 头往这个方向走一格会到哪里（无边界模式要绕到对面，所以用 %）
    int nextX = (s->x[0] + dx + g->width) % g->width;
    int nextY = (s->y[0] + dy + g->height) % g->height;

    if (nextX == s->x[1] && nextY == s->y[1])
    {
        reverseSnake(s);
        // 新的头（原本的尾巴）往“远离第 2 节”的方向走
        dx = s->x[0] - s->x[1];
        dy = s->y[0] - s->y[1];
        // 无边界模式下，两节可能隔着地图边缘（例如 0 和 width-1），换算回 1 或 -1
        if (dx > 1)  dx = -1;
        if (dx < -1) dx = 1;
        if (dy > 1)  dy = -1;
        if (dy < -1) dy = 1;
    }
    s->dirX = dx;
    s->dirY = dy;
}

// 读 W/A/S/D
void handleInput(Game *g)
{
    if (IsKeyPressed(KEY_W)) setDirection(g, 0, -1);
    if (IsKeyPressed(KEY_S)) setDirection(g, 0, 1);
    if (IsKeyPressed(KEY_A)) setDirection(g, -1, 0);
    if (IsKeyPressed(KEY_D)) setDirection(g, 1, 0);
}

// ================= 道具与生命 =================

// 吃到道具：用 rand() 决定是哪一种，然后套用效果
void applyBooster(Game *g)
{
    int type = rand() % 3;

    if (type == SPEED_BOOST)
    {
        g->speedTimer = BOOST_TIME;
        showMessage(g, "Speed Up!");
    }
    else if (type == SIZE_REDUCER)
    {
        g->snake.length -= SHRINK_AMOUNT;
        if (g->snake.length < MIN_LENGTH)
            g->snake.length = MIN_LENGTH;
        showMessage(g, "Shrink!");
    }
    else  // EXTRA_LIFE
    {
        g->lives++;
        showMessage(g, "Extra Life!");
    }
}

// 撞到东西：扣一条命。还有命就回到起点继续，没命了才输
void loseLife(Game *g)
{
    g->lives--;
    if (g->lives <= 0)
    {
        g->state = LOSE;
    }
    else
    {
        resetSnake(g);
        g->speedTimer = 0;
        showMessage(g, "Lost a life!");
    }
}

// ================= 游戏进行 =================

// 蛇走一格：先检查会不会撞到，再看有没有吃到东西
void stepGame(Game *g)
{
    Snake *s = &g->snake;
    int newX = s->x[0] + s->dirX;
    int newY = s->y[0] + s->dirY;

    // 1. 走出地图边缘
    if (newX < 0 || newX >= g->width || newY < 0 || newY >= g->height)
    {
        if (g->mode == CLASSIC)
        {
            loseLife(g);
            return;
        }
        // ★ 无边界模式：从对面出来
        newX = (newX + g->width) % g->width;
        newY = (newY + g->height) % g->height;
    }

    // 2. 撞障碍物
    if (g->map[newY][newX] == OBSTACLE)
    {
        loseLife(g);
        return;
    }

    int ate = (g->map[newY][newX] == FOOD);
    int gotBooster = (g->map[newY][newX] == BOOSTER);

    // 3. 撞到自己（没吃到食物时尾巴会移开，所以尾巴那一格不算）
    int checkLength = s->length;
    if (!ate)
        checkLength = s->length - 1;
    for (int i = 0; i < checkLength; i++)
    {
        if (s->x[i] == newX && s->y[i] == newY)
        {
            loseLife(g);
            return;
        }
    }

    // 4. 吃到食物：长度 +1、分数 +1
    if (ate)
    {
        s->length++;
        g->score++;
        g->map[newY][newX] = EMPTY;
    }

    // 5. 移动
    for (int i = s->length - 1; i > 0; i--)
    {
        s->x[i] = s->x[i - 1];
        s->y[i] = s->y[i - 1];
    }
    s->x[0] = newX;
    s->y[0] = newY;

    // 6. 够分数就赢，不够就再放一个新食物
    if (ate)
    {
        if (g->score >= WIN_SCORE)
            g->state = WIN;
        else
            placeItem(g, FOOD);
    }

    // 7. 吃到道具
    if (gotBooster)
    {
        g->map[newY][newX] = EMPTY;
        applyBooster(g);
        placeItem(g, BOOSTER);
    }
}

// 游戏进行中每一帧要做的事
void updatePlaying(Game *g)
{
    handleInput(g);

    float dt = GetFrameTime();
    if (g->speedTimer > 0)
        g->speedTimer -= dt;
    if (g->messageTimer > 0)
        g->messageTimer -= dt;

    float moveTime = MOVE_TIME;
    if (g->speedTimer > 0)
        moveTime = MOVE_TIME / 2;

    g->timer += dt;
    if (g->timer >= moveTime)
    {
        stepGame(g);
        g->timer = 0;
    }
}

// ★ 每一帧的更新：看现在在哪个画面，做对应的事。回传 1 表示要退出
int updateGame(Game *g)
{
    if (g->state == MENU)
    {
        updateMenu(g);
    }
    else if (g->state == PLAYING)
    {
        updatePlaying(g);
    }
    else  // LOSE 或 WIN
    {
        if (IsKeyPressed(KEY_R))
            resetGame(g);
        if (IsKeyPressed(KEY_M))     // ★ 回开始画面（会重新选大小，所以先释放内存）
        {
            freeGame(g);
            g->state = MENU;
        }
        if (IsKeyPressed(KEY_Q))
            return 1;
    }
    return 0;
}

// ================= 画面 =================

// 画一格（会加上地图的位置偏移）
void drawCell(Game *g, int x, int y, Color color)
{
    DrawRectangle(g->offsetX + x * g->cell, g->offsetY + y * g->cell, g->cell, g->cell, color);
}

// 在画面左右置中的位置写字
void drawCenterText(const char *text, int y, int size, Color color)
{
    int textWidth = MeasureText(text, size);
    DrawText(text, (SCREEN_W - textWidth) / 2, y, size, color);
}

// ★ 画开始画面
void drawMenu(Game *g)
{
    drawCenterText("SNAKE", 80, 60, LIME);

    const char *modeName = "Classic";
    if (g->mode == NO_WALLS)
        modeName = "No Walls";

    const char *rows[3];
    rows[0] = TextFormat("Width:  < %d >", g->width);
    rows[1] = TextFormat("Height: < %d >", g->height);
    rows[2] = TextFormat("Mode:   < %s >", modeName);

    for (int i = 0; i < 3; i++)
    {
        Color color = GRAY;
        if (i == g->menuSelected)
            color = YELLOW;              // 选到的那一项变黄色
        drawCenterText(rows[i], 230 + i * 60, 30, color);
    }

    if (g->mode == CLASSIC)
        drawCenterText("Hit the wall = lose a life", 420, 20, LIGHTGRAY);
    else
        drawCenterText("Go through walls to the other side", 420, 20, LIGHTGRAY);

    drawCenterText(g->menuError, 470, 20, RED);

    drawCenterText(TextFormat("Map size: %d - %d", MIN_SIZE, MAX_SIZE), 540, 20, DARKGRAY);
    drawCenterText("UP/DOWN: select   LEFT/RIGHT or 0-9: change", 570, 20, DARKGRAY);
    drawCenterText("ENTER: start", 600, 20, WHITE);
}

// 画游戏画面（地图、蛇、分数、提示、结束画面）
void drawPlaying(Game *g)
{
    // 地图
    for (int y = 0; y < g->height; y++)
    {
        for (int x = 0; x < g->width; x++)
        {
            if (g->map[y][x] == FOOD)
                drawCell(g, x, y, RED);
            else if (g->map[y][x] == OBSTACLE)
                drawCell(g, x, y, GRAY);
            else if (g->map[y][x] == BOOSTER)
                drawCell(g, x, y, PURPLE);
            DrawRectangleLines(g->offsetX + x * g->cell, g->offsetY + y * g->cell,
                               g->cell, g->cell, DARKGRAY);
        }
    }

    // ★ 地图外框：经典模式白色（墙），无边界模式蓝色（可以穿过）
    Color border = WHITE;
    if (g->mode == NO_WALLS)
        border = SKYBLUE;
    DrawRectangleLines(g->offsetX - 2, g->offsetY - 2,
                       g->width * g->cell + 4, g->height * g->cell + 4, border);

    // 蛇：头亮绿色，身体绿色，加速中身体变黄色
    for (int i = 0; i < g->snake.length; i++)
    {
        if (i == 0)
            drawCell(g, g->snake.x[i], g->snake.y[i], LIME);
        else if (g->speedTimer > 0)
            drawCell(g, g->snake.x[i], g->snake.y[i], YELLOW);
        else
            drawCell(g, g->snake.x[i], g->snake.y[i], GREEN);
    }

    // 上方显示分数和生命
    DrawText(TextFormat("Score: %d / %d   Lives: %d", g->score, WIN_SCORE, g->lives),
             10, 20, 20, WHITE);

    // 提示文字
    if (g->messageTimer > 0)
        drawCenterText(g->message, HUD_H + 20, 30, YELLOW);

    // 结束画面
    if (g->state != PLAYING)
    {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.6f));
        int midY = SCREEN_H / 2;
        if (g->state == WIN)
            drawCenterText("You Win!", midY - 40, 40, GOLD);
        else
            drawCenterText("Game Over", midY - 40, 40, RED);
        drawCenterText("R: Restart   M: Menu   Q: Quit", midY + 20, 20, WHITE);
    }
}

// 画出整个画面
void drawGame(Game *g)
{
    BeginDrawing();
    ClearBackground(BLACK);
    if (g->state == MENU)
        drawMenu(g);
    else
        drawPlaying(g);
    EndDrawing();
}

int main(void)
{
    srand(time(NULL));

    Game game;
    initGame(&game);

    InitWindow(SCREEN_W, SCREEN_H, "Snake");
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