#include <stdio.h>    // snprintf
#include <stdlib.h>
#include <time.h>
#include <math.h>     // sinf
#include "raylib.h"

// ===== 画面大小 =====
#define SCREEN_W 600        // 窗口宽
#define HUD_H    60         // 上方显示分数的区域高度
#define BOARD    600        // 地图区域最大 600x600
#define SCREEN_H (HUD_H + BOARD)
#define BOARD_MARGIN 10     // ★ 地图四周留几像素给外框

// ===== 游戏数值（录视频“现场改数值”可以改这些） =====
#define MOVE_TIME     0.15f // 每 0.15 秒走一格（越小越快）
#define WIN_SCORE     20    // 吃到 20 个食物就赢
#define OBSTACLE_RATE 50    // 大约每 50 格放 1 个障碍物（越大越少）
#define BOOST_TIME    5.0f  // 加速持续几秒
#define SHRINK_AMOUNT 3     // 缩短几节
#define MIN_LENGTH    3     // 蛇最短几节
#define START_LIVES   1     // 一开始有几条命
#define MESSAGE_TIME  1.5f  // 提示文字显示几秒
#define SNAKE_SCALE   1.5f  // ★ 蛇的图片画成格子的几倍大（越大蛇越粗）
#define HEAD_SCALE    1.0f  // ★ 蛇头画成格子的几倍大（头比较宽，所以小一点才不会像一颗球）

// ★ 关卡
#define FOODS_PER_LEVEL 5      // 每吃 5 个食物升一级
#define LEVEL_SPEEDUP   0.015f // 每升一级，每步少等 0.015 秒（变快）
#define MIN_MOVE_TIME   0.06f  // 最快不超过这个速度
#define LEVEL_OBSTACLES 2      // 每升一级多 2 个障碍物
#define SAFE_DISTANCE   4      // 新障碍物至少离蛇头这么远，避免突然出现在面前

// ===== 地图大小的范围和默认值 =====
#define MIN_SIZE     5
#define MAX_SIZE     30
#define DEFAULT_W    15
#define DEFAULT_H    12

// ===== ★ 赛博朋克配色 =====
#define BG_COLOR     (Color){ 12, 6, 28, 255 }     // 深紫黑背景
#define NEON_PINK    (Color){ 255, 45, 200, 255 }
#define NEON_CYAN    (Color){ 0, 240, 255, 255 }
#define NEON_GREEN   (Color){ 0, 255, 170, 255 }   // ★ 青绿色：地图外框（跟紫色背景是对比色）
#define NEON_PURPLE  (Color){ 170, 80, 255, 255 }
#define NEON_YELLOW  (Color){ 255, 230, 0, 255 }
#define NEON_RED     (Color){ 255, 50, 80, 255 }
#define NEON_ORANGE  (Color){ 255, 140, 0, 255 }
#define DIM_TEXT     (Color){ 120, 100, 160, 255 }  // 不重要的文字

// ===== ★ 金属地板配色 =====
#define PLATE_COLOR  (Color){ 34, 22, 54, 255 }    // 钢板底色（★ 偏紫色）
#define PLATE_LIGHT  (Color){ 58, 40, 86, 255 }    // 钢板左上亮边
#define PLATE_DARK   (Color){ 16, 10, 28, 255 }    // 钢板右下暗边
#define RIVET_COLOR  (Color){ 88, 66, 122, 255 }   // 螺丝钉

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
    int dirX, dirY;  // 移动方向（上一步真正走的方向）

    // ★ 按键排队：一步之内按了两个键（例如快速 W 再 A），两个都会照顺序用到，不会被吃掉
    int queueX[2], queueY[2];
    int queueCount;

    // ★ 平滑移动要用的资料
    int prevTailX, prevTailY;     // 尾巴上一步在哪一格（画的时候从这里滑过去）
    int tailDirX, tailDirY;       // 尾巴这一步往哪个方向滑
    int oldTailDirX, oldTailDirY; // 尾巴上一步的方向（转弯时用来慢慢转过去）
} Snake;

// ★ 所有音效
typedef struct {
    Sound eat;       // 吃到食物
    Sound booster;   // 吃到道具
    Sound crash;     // 撞到东西
    Sound levelUp;   // 升级
    Sound win;       // 赢了
} Sounds;

// ★ 所有图片（你画的像素图）
typedef struct {
    Texture2D head;      // 蛇头（画成往右看）
    Texture2D body;      // 蛇身（直的，画成横的：左右两边接起来）
    Texture2D corner;    // ★ 蛇身转角（画成接右边和下边）
    Texture2D tail;      // 蛇尾（画成往右走：尖端在左边）
    Texture2D apple;     // 苹果
    Texture2D booster;   // 道具
    Texture2D obstacle;  // 障碍物
    Texture2D floor;     // ★ 地板（可以不画，没有的话程序会自己画金属钢板）
    Texture2D cover;     // ★ 开始画面的封面（600x660，中间的面板留空，文字由程序画上去）
} Textures;

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
    char message[32];    // ★ 改成字符数组：把文字复制进来存着
    float messageTimer;
    int level;           // ★ 现在第几关
    int paused;          // 1 = 暂停中
    Sounds sounds;       // 音效
    Textures textures;   // ★ 图片
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

// ★ 从格子 (ax, ay) 往相邻的格子 (bx, by) 是哪个方向，结果放进 *dx、*dy
void neighborDir(int ax, int ay, int bx, int by, int *dx, int *dy)
{
    *dx = bx - ax;
    *dy = by - ay;
    // 无边界模式：两格隔着地图边缘（例如 0 和 width-1）时，换算回 1 或 -1
    if (*dx > 1)  *dx = -1;
    if (*dx < -1) *dx = 1;
    if (*dy > 1)  *dy = -1;
    if (*dy < -1) *dy = 1;
}

// ★ 第 i 节往第 j 节是哪个方向
void segmentDir(Game *g, int i, int j, int *dx, int *dy)
{
    Snake *s = &g->snake;
    neighborDir(s->x[i], s->y[i], s->x[j], s->y[j], dx, dy);
}

// ★ 检查 (x, y) 能不能放这种东西
int isValidSpot(Game *g, int x, int y, int type)
{
    if (g->map[y][x] != EMPTY || isOnSnake(&g->snake, x, y))
        return 0;
    if (type == OBSTACLE)
    {
        if (y == g->height / 2)        // 障碍物不放在蛇出发的那一行
            return 0;
        int distance = abs(x - g->snake.x[0]) + abs(y - g->snake.y[0]);
        if (distance < SAFE_DISTANCE)  // 障碍物不放在蛇头附近
            return 0;
    }
    return 1;
}

// ★ 改过：先数有几个能放的格子，再从里面随机挑一个（不会因为找不到空格而卡住）
void placeItem(Game *g, int type)
{
    // 第 1 轮：数一数有几个格子可以放
    int count = 0;
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            if (isValidSpot(g, x, y, type))
                count++;

    if (count == 0)    // 一个都没有就不放
        return;

    // 第 2 轮：随机决定放在第几个可以放的格子
    int target = rand() % count;
    for (int y = 0; y < g->height; y++)
    {
        for (int x = 0; x < g->width; x++)
        {
            if (isValidSpot(g, x, y, type))
            {
                if (target == 0)
                {
                    g->map[y][x] = type;
                    return;
                }
                target--;
            }
        }
    }
}

// ================= 音效 =================

// ★ 用程序“算”出一个音效：频率从 startFreq 慢慢变到 endFreq，长度 duration 秒
//   频率 = 每秒震动几次，越高声音越尖
Sound makeSound(float startFreq, float endFreq, float duration)
{
    int sampleRate = 44100;                      // 每秒 44100 个点（CD 音质）
    int count = (int)(sampleRate * duration);    // 总共要几个点
    short *samples = malloc(count * sizeof(short));

    float phase = 0;                             // 现在在一次震动的哪个位置（0 ~ 1）
    for (int i = 0; i < count; i++)
    {
        float progress = (float)i / count;       // 进度 0 → 1
        float freq = startFreq + (endFreq - startFreq) * progress;
        phase += freq / sampleRate;
        if (phase >= 1)
            phase -= 1;

        float volume = 0.3f * (1 - progress);    // 越后面越小声（淡出）

        // 方波：前半次震动往上推、后半次往下拉，听起来像复古游戏机
        if (phase < 0.5f)
            samples[i] = (short)(volume * 32767);
        else
            samples[i] = (short)(-volume * 32767);
    }

    Wave wave = { count, sampleRate, 16, 1, samples };  // 点数、每秒几点、16 位、单声道、资料
    Sound sound = LoadSoundFromWave(wave);              // Raylib 会复制一份
    free(samples);                                      // 所以自己这份可以释放了
    return sound;
}

// ★ 一次做好所有音效
void loadSounds(Sounds *s)
{
    s->eat     = makeSound(600, 900, 0.08f);   // 短短的“哔”往上
    s->booster = makeSound(400, 1200, 0.25f);  // 往上滑
    s->crash   = makeSound(300, 60, 0.4f);     // 往下掉
    s->levelUp = makeSound(500, 1000, 0.35f);
    s->win     = makeSound(400, 1600, 0.8f);
}

// ★ 释放所有音效
void unloadSounds(Sounds *s)
{
    UnloadSound(s->eat);
    UnloadSound(s->booster);
    UnloadSound(s->crash);
    UnloadSound(s->levelUp);
    UnloadSound(s->win);
}

// ================= 游戏的开始与结束 =================

// 显示一段提示文字（例如 "Speed Up!"）
void showMessage(Game *g, const char *text)
{
    snprintf(g->message, sizeof(g->message), "%s", text);  // 把文字复制到 message 里
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
    g->snake.queueCount = 0;
    g->snake.prevTailX = g->snake.x[2];   // 尾巴不动
    g->snake.prevTailY = g->snake.y[2];
    g->snake.tailDirX = 1;
    g->snake.tailDirY = 0;
    g->snake.oldTailDirX = 1;
    g->snake.oldTailDirY = 0;
    g->timer = 0;
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
    g->message[0] = '\0';     // ★ 清空文字
    g->messageTimer = 0;
    g->level = 1;             // ★
    g->paused = 0;            // ★

    int obstacleCount = g->width * g->height / OBSTACLE_RATE;
    for (int i = 0; i < obstacleCount; i++)
        placeItem(g, OBSTACLE);
    placeItem(g, FOOD);
    placeItem(g, BOOSTER);
}

// ★ 从开始画面按下 Enter：依照玩家选的大小要内存，然后开始游戏
void startGame(Game *g)
{
    // 算出每格多大，让地图塞进 600x600 的区域（★ 四周留 BOARD_MARGIN，外框才不会跑出画面）
    int space = BOARD - 2 * BOARD_MARGIN;
    if (g->width > g->height)
        g->cell = space / g->width;
    else
        g->cell = space / g->height;

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
        // ★ 尾巴直接变到新位置，这一步不用滑
        g->snake.prevTailX = g->snake.x[g->snake.length - 1];
        g->snake.prevTailY = g->snake.y[g->snake.length - 1];
        segmentDir(g, g->snake.length - 1, g->snake.length - 2, &g->snake.tailDirX, &g->snake.tailDirY);
        g->snake.oldTailDirX = g->snake.tailDirX;
        g->snake.oldTailDirY = g->snake.tailDirY;
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
    PlaySound(g->sounds.crash);    // ★
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

// ★ 升级：多放几个障碍物，显示 "Level 2!"
void levelUp(Game *g)
{
    g->level++;
    PlaySound(g->sounds.levelUp);  // ★
    for (int i = 0; i < LEVEL_OBSTACLES; i++)
        placeItem(g, OBSTACLE);

    char text[32];
    snprintf(text, sizeof(text), "Level %d!", g->level);
    showMessage(g, text);
}

// ★ 依照关卡算出每一步要等多久（关卡越高越快）
float getMoveTime(Game *g)
{
    float moveTime = MOVE_TIME - (g->level - 1) * LEVEL_SPEEDUP;
    if (moveTime < MIN_MOVE_TIME)
        moveTime = MIN_MOVE_TIME;
    if (g->speedTimer > 0)      // 加速道具：再快一倍
        moveTime = moveTime / 2;
    return moveTime;
}

// ================= 游戏进行 =================

// 蛇走一格：先检查会不会撞到，再看有没有吃到东西
void stepGame(Game *g)
{
    Snake *s = &g->snake;
    takeQueuedDirection(s);              // ★ 这时候才真的转弯
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

    // ★ 先记住尾巴原本在哪（画的时候要从那里滑过去）
    //   一定要在“吃到食物、长度 +1”之前记，不然会读到还没用过的格子，尾巴会从角落飞过来
    int oldTailX = s->x[s->length - 1];
    int oldTailY = s->y[s->length - 1];
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
        PlaySound(g->sounds.eat);  // ★
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

    // ★ 尾巴：记住从哪一格滑到哪一格（吃到食物时尾巴不动）
    s->oldTailDirX = s->tailDirX;
    s->oldTailDirY = s->tailDirY;
    s->prevTailX = oldTailX;
    s->prevTailY = oldTailY;
    int last = s->length - 1;
    if (s->x[last] != oldTailX || s->y[last] != oldTailY)
        neighborDir(oldTailX, oldTailY, s->x[last], s->y[last], &s->tailDirX, &s->tailDirY);

    // 6. 够分数就赢，不够就再放一个新食物
    if (ate)
    {
        if (g->score >= WIN_SCORE)
        {
            g->state = WIN;
            PlaySound(g->sounds.win);   // ★
        }
        else
        {
            placeItem(g, FOOD);
            if (g->score % FOODS_PER_LEVEL == 0)   // ★ 每吃满 5 个就升级
                levelUp(g);
        }
    }

    // 7. 吃到道具
    if (gotBooster)
    {
        g->map[newY][newX] = EMPTY;
        PlaySound(g->sounds.booster);   // ★
        applyBooster(g);
        placeItem(g, BOOSTER);
    }
}

// 游戏进行中每一帧要做的事
void updatePlaying(Game *g)
{
    // ★ P：暂停 / 继续
    if (IsKeyPressed(KEY_P))
        g->paused = !g->paused;
    if (g->paused)
        return;          // 暂停中，后面都不做

    handleInput(g);

    float dt = GetFrameTime();
    if (g->speedTimer > 0)
        g->speedTimer -= dt;
    if (g->messageTimer > 0)
        g->messageTimer -= dt;

    g->timer += dt;
    float moveTime = getMoveTime(g);   // ★ 改用关卡算出来的速度
    if (g->timer >= moveTime)
    {
        // ★ 用“减掉”而不是“归零”：多出来的一点点时间留给下一步，节奏才会均匀
        g->timer -= moveTime;
        if (g->timer > moveTime)       // 卡顿太久（例如拖动窗口）就不要补走好几步
            g->timer = 0;
        stepGame(g);
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

// ================= 图片 =================

// ★ 读一张图片；文件不存在也没关系，画的时候会用色块代替
Texture2D loadImage(const char *path)
{
    Texture2D texture = { 0 };
    if (FileExists(path))
    {
        texture = LoadTexture(path);
        if (texture.width > 64)
        {
            // 大图（例如 512x512）要缩小很多：先做好几个缩小版本，缩小时才不会出现锯齿
            GenTextureMipmaps(&texture);
            SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        }
        else
        {
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);   // 小像素图放大时保持锐利
        }
        // ★ 图片边缘不要“重复”：不然画到左边时会取到右边的像素，出现一条细细的线
        SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
    }
    return texture;
}

// ★ 读进所有图片（都放在 assets 文件夹）
void loadTextures(Textures *t)
{
    t->head     = loadImage("assets/head.png");
    t->body     = loadImage("assets/body.png");
    t->corner   = loadImage("assets/corner.png");
    t->tail     = loadImage("assets/tail.png");
    t->apple    = loadImage("assets/apple.png");
    t->booster  = loadImage("assets/booster.png");
    t->obstacle = loadImage("assets/obstacle.png");
    t->floor    = loadImage("assets/floor.png");
    t->cover    = loadImage("assets/cover.png");
}

// ★ 释放所有图片（id 是 0 表示没读到，不用释放）
void unloadTextures(Textures *t)
{
    if (t->head.id > 0)     UnloadTexture(t->head);
    if (t->body.id > 0)     UnloadTexture(t->body);
    if (t->corner.id > 0)   UnloadTexture(t->corner);
    if (t->tail.id > 0)     UnloadTexture(t->tail);
    if (t->apple.id > 0)    UnloadTexture(t->apple);
    if (t->booster.id > 0)  UnloadTexture(t->booster);
    if (t->obstacle.id > 0) UnloadTexture(t->obstacle);
    if (t->floor.id > 0)    UnloadTexture(t->floor);
    if (t->cover.id > 0)    UnloadTexture(t->cover);
}

// ================= 画面 =================

// 在画面左右置中的位置写字
void drawCenterText(const char *text, int y, int size, Color color)
{
    int textWidth = MeasureText(text, size);
    DrawText(text, (SCREEN_W - textWidth) / 2, y, size, color);
}

// ★ 发光的字：先在旁边画几层半透明的字当作“光”，再画本体
void drawNeonText(const char *text, int y, int size, Color color)
{
    int textWidth = MeasureText(text, size);
    int x = (SCREEN_W - textWidth) / 2;
    DrawText(text, x - 2, y, size, Fade(color, 0.3f));
    DrawText(text, x + 2, y, size, Fade(color, 0.3f));
    DrawText(text, x, y - 2, size, Fade(color, 0.3f));
    DrawText(text, x, y + 2, size, Fade(color, 0.3f));
    DrawText(text, x, y, size, color);
}

// ★ 方向 → 旋转角度（图片都画成往右，其他方向就转过去）
//   往右 0 度、往下 90 度、往左 180 度、往上 270 度（画面的 y 是往下变大，所以顺时针）
float directionToAngle(int dx, int dy)
{
    if (dx == 1)  return 0;
    if (dy == 1)  return 90;
    if (dx == -1) return 180;
    return 270;
}

// 在第 (x, y) 格画一张图，可以旋转；flip = 1 时左右翻转。没有图片就画一个色块代替
//   ★ x、y 改成 float：可以画在两格中间（例如 3.5），蛇才能平滑地滑过去
//   ★ w、h：图片画成几格宽、几格高（例如 1.5 = 格子的 1.5 倍）
void drawSpriteSize(Game *g, Texture2D texture, float x, float y, float angle, int flip,
                    float w, float h, Color fallback)
{
    float cell = g->cell;
    float left = g->offsetX + x * cell;
    float top  = g->offsetY + y * cell;

    if (texture.id == 0)       // 还没有图片：画色块
    {
        DrawRectangleRec((Rectangle){ left + 2, top + 2, cell - 4, cell - 4 }, fallback);
        return;
    }

    Rectangle source = { 0, 0, texture.width, texture.height };      // 整张图
    if (flip)
        source.width = -source.width;                                 // ★ 宽度变负的 = 左右翻转
    Rectangle dest = { left + cell / 2, top + cell / 2, w * cell, h * cell }; // 画在格子中心
    Vector2 origin = { w * cell / 2, h * cell / 2 };                  // 绕着中心旋转
    DrawTexturePro(texture, source, dest, origin, angle, WHITE);
}

// 一般的画图：宽高一样，scale 是放大几倍
void drawSprite(Game *g, Texture2D texture, float x, float y, float angle, int flip, float scale, Color fallback)
{
    drawSpriteSize(g, texture, x, y, angle, flip, scale, scale, fallback);
}

// ★ 转角要转几度。corner.png 画的是“接右边和下边”，转 90 度就变成“接下边和左边”，以此类推
float cornerAngle(int ax, int ay, int bx, int by)
{
    int right = (ax == 1  || bx == 1);
    int left  = (ax == -1 || bx == -1);
    int down  = (ay == 1  || by == 1);
    int up    = (ay == -1 || by == -1);

    if (right && down) return 0;
    if (down && left)  return 90;
    if (left && up)    return 180;
    if (up && right)   return 270;
    return 0;
}

// ★ 把 0 ~ 1 的进度变成“慢 - 快 - 慢”，转头时看起来比较自然
float smoothStep(float p)
{
    if (p < 0) p = 0;
    if (p > 1) p = 1;
    return p * p * (3 - 2 * p);
}

// ★ 从角度 a 慢慢转到角度 b（p 从 0 到 1），走比较近的那一边
float turnAngle(float a, float b, float p)
{
    float diff = b - a;
    while (diff > 180)  diff -= 360;
    while (diff < -180) diff += 360;
    return a + diff * smoothStep(p);
}

// ★ 画一节身体：看它往头那边 (ax, ay) 和往尾巴那边 (bx, by) 的方向，决定用直的还是转角
void drawBodyPiece(Game *g, int x, int y, int ax, int ay, int bx, int by, Color color)
{
    Textures *t = &g->textures;
    if (ax == -bx && ay == -by)           // 两个方向相反 → 直的
    {
        float angle = 0;                  // 横的
        if (ax == 0)
            angle = 90;                   // 直的（上下）
        // ★ 长度刚好 1 格（一节接一节，花纹不会叠在一起），粗细还是放大 SNAKE_SCALE 倍
        drawSpriteSize(g, t->body, x, y, angle, 0, 1, SNAKE_SCALE, color);
    }
    else                                  // 否则 → 转角
    {
        Texture2D piece = t->corner;
        if (piece.id == 0)                // 还没画转角，先用直的代替
            piece = t->body;
        drawSprite(g, piece, x, y, cornerAngle(ax, ay, bx, by), 0, SNAKE_SCALE, color);
    }
}

// ★ 画会滑动的那一节（头或尾巴）：从格子 (fromX, fromY) 往 (dx, dy) 方向滑了 p 格
//   无边界模式穿过边缘时：两边各画一次，而且只画地图里面的部分，看起来就像从对面钻出来
void drawSliding(Game *g, Texture2D texture, int fromX, int fromY, int dx, int dy, float p,
                 float angle, float scale, Color color)
{
    float x = fromX + dx * p;
    float y = fromY + dy * p;
    int toX = fromX + dx;
    int toY = fromY + dy;

    if (toX >= 0 && toX < g->width && toY >= 0 && toY < g->height)   // 一般情况
    {
        drawSprite(g, texture, x, y, angle, 0, scale, color);
        return;
    }

    BeginScissorMode(g->offsetX, g->offsetY, g->width * g->cell, g->height * g->cell);
    drawSprite(g, texture, x, y, angle, 0, scale, color);                               // 正要出去的
    drawSprite(g, texture, x - dx * g->width, y - dy * g->height, angle, 0, scale, color); // 从对面进来的
    EndScissorMode();
}

// ★ 画蛇（平滑版）
//   格子里的身体不会动；只有“头”和“尾巴”会依照这一步走了多少，在两格之间滑过去。
//   这样蛇看起来是连续地爬，而不是一格一格跳。
void drawSnake(Game *g)
{
    Snake *s = &g->snake;
    Textures *t = &g->textures;

    // 加速中：没有图片时用黄色色块
    Color bodyColor = NEON_CYAN;
    if (g->speedTimer > 0)
        bodyColor = NEON_YELLOW;

    // 这一步走了多少：0 = 刚走完上一步，1 = 马上要走下一步
    float p = g->timer / getMoveTime(g);
    if (p > 1 || g->state != PLAYING)     // 游戏结束就停在格子正中间
        p = 1;

    // ★ 蛇的图片比格子大，会互相重叠，所以从尾巴画到头：越靠近头的越晚画、盖在上面

    // 1. 尾巴正要进去的那一格，先垫一节身体（不然尾巴和身体中间会有缝）
    int last = s->length - 1;
    int tailMoving = (s->prevTailX != s->x[last] || s->prevTailY != s->y[last]);
    if (tailMoving)
    {
        int ax, ay;
        segmentDir(g, last, last - 1, &ax, &ay);
        drawBodyPiece(g, s->x[last], s->y[last], ax, ay, -s->tailDirX, -s->tailDirY, bodyColor);
    }

    // 2. 尾巴：从上一格滑过来；转弯时慢慢转过去（前半步转完）
    float tailAngle = turnAngle(directionToAngle(s->oldTailDirX, s->oldTailDirY),
                                directionToAngle(s->tailDirX, s->tailDirY), p * 2);
    if (tailMoving)
        drawSliding(g, t->tail, s->prevTailX, s->prevTailY, s->tailDirX, s->tailDirY, p,
                    tailAngle, SNAKE_SCALE, NEON_PURPLE);
    else    // 刚吃到食物：尾巴这一步不动
        drawSprite(g, t->tail, s->x[last], s->y[last], tailAngle, 0, SNAKE_SCALE, NEON_PURPLE);

    // 3. 身体：从倒数第 2 节画到第 1 节（都在格子正中间，不会动）
    for (int i = last - 1; i >= 1; i--)
    {
        int ax, ay, bx, by;
        segmentDir(g, i, i - 1, &ax, &ay);   // 往头那边的方向
        segmentDir(g, i, i + 1, &bx, &by);   // 往尾巴那边的方向
        drawBodyPiece(g, s->x[i], s->y[i], ax, ay, bx, by, bodyColor);
    }

    // 4. 头：从第 1 节的位置滑向第 0 节；转弯时慢慢转过去。最后画才会盖在最上面
    //    方向看的是“真的走的方向”，不是刚按的键，所以按键的瞬间头不会先转过去
    int dx, dy, oldDx, oldDy;
    segmentDir(g, 1, 0, &dx, &dy);           // 这一步的方向
    segmentDir(g, 2, 1, &oldDx, &oldDy);     // 上一步的方向
    float angle = turnAngle(directionToAngle(oldDx, oldDy), directionToAngle(dx, dy), p * 2);
    drawSliding(g, t->head, s->x[1], s->y[1], dx, dy, p, angle, HEAD_SCALE, NEON_PINK);

    // 加速中：蛇头外面加一圈黄色的光
    if (g->speedTimer > 0)
    {
        Rectangle box = { g->offsetX + (s->x[1] + dx * p) * g->cell,
                          g->offsetY + (s->y[1] + dy * p) * g->cell, g->cell, g->cell };
        DrawRectangleLinesEx(box, 1, NEON_YELLOW);
    }
}

// ★ 用代码画一块金属钢板（一格）：底色 + 左上亮边 + 右下暗边 + 四角螺丝钉
void drawMetalPlate(Game *g, int x, int y)
{
    int c = g->cell;
    int left = g->offsetX + x * c;
    int top  = g->offsetY + y * c;

    DrawRectangle(left, top, c, c, PLATE_COLOR);
    DrawLine(left, top + 1, left + c, top + 1, PLATE_LIGHT);            // 上亮边
    DrawLine(left + 1, top, left + 1, top + c, PLATE_LIGHT);            // 左亮边
    DrawLine(left, top + c - 1, left + c, top + c - 1, PLATE_DARK);     // 下暗边
    DrawLine(left + c - 1, top, left + c - 1, top + c, PLATE_DARK);     // 右暗边

    if (c >= 24)              // 格子够大才画螺丝钉，太小会挤成一团
    {
        float r = c / 16.0f + 1;   // 螺丝钉的半径
        int d = c / 6;             // 螺丝钉离角落多远
        DrawCircle(left + d,     top + d,     r, RIVET_COLOR);
        DrawCircle(left + c - d, top + d,     r, RIVET_COLOR);
        DrawCircle(left + d,     top + c - d, r, RIVET_COLOR);
        DrawCircle(left + c - d, top + c - d, r, RIVET_COLOR);
    }
}

// ★ 发光的能量线路：格线的亮度会随时间慢慢变亮、变暗
void drawEnergyGrid(Game *g)
{
    // sinf 的结果在 -1 ~ 1 之间，所以透明度在 0.2 ~ 0.5 之间来回变化
    float glow = 0.35f + 0.15f * sinf(GetTime() * 2);
    Color line = Fade(NEON_PURPLE, glow);

    int right  = g->offsetX + g->width * g->cell;
    int bottom = g->offsetY + g->height * g->cell;

    for (int x = 0; x <= g->width; x++)        // 直线
    {
        int px = g->offsetX + x * g->cell;
        DrawLine(px, g->offsetY, px, bottom, line);
    }
    for (int y = 0; y <= g->height; y++)       // 横线
    {
        int py = g->offsetY + y * g->cell;
        DrawLine(g->offsetX, py, right, py, line);
    }
}

// ★ 开始画面上的三个选项（宽、高、模式）：选到的那一项发光，其他的暗暗的
void drawMenuRows(Game *g, int firstY, int gap, int size)
{
    const char *modeName = "Classic";
    if (g->mode == NO_WALLS)
        modeName = "No Walls";

    const char *rows[3];
    rows[0] = TextFormat("Width:  < %d >", g->width);
    rows[1] = TextFormat("Height: < %d >", g->height);
    rows[2] = TextFormat("Mode:   < %s >", modeName);

    for (int i = 0; i < 3; i++)
    {
        if (i == g->menuSelected)
            drawNeonText(rows[i], firstY + i * gap, size, NEON_CYAN);   // 选到的那一项发光
        else
            drawCenterText(rows[i], firstY + i * gap, size, DIM_TEXT);
    }
}

// ★ 模式说明
const char *modeHint(Game *g)
{
    if (g->mode == CLASSIC)
        return "Hit the wall = lose a life";
    return "Go through walls to the other side";
}

// ★ 画虚线框：每 12 像素画 8 像素、空 4 像素
void drawDashedRect(Rectangle r, float thick, Color color)
{
    float dash = 8, step = 12;
    for (float x = r.x; x < r.x + r.width; x += step)          // 上下两条
    {
        float w = fminf(dash, r.x + r.width - x);
        DrawRectangleRec((Rectangle){ x, r.y, w, thick }, color);
        DrawRectangleRec((Rectangle){ x, r.y + r.height - thick, w, thick }, color);
    }
    for (float y = r.y; y < r.y + r.height; y += step)         // 左右两条
    {
        float h = fminf(dash, r.y + r.height - y);
        DrawRectangleRec((Rectangle){ r.x, y, thick, h }, color);
        DrawRectangleRec((Rectangle){ r.x + r.width - thick, y, thick, h }, color);
    }
}

// 画开始画面
void drawMenu(Game *g)
{
    // ★ 有封面图：标题和下面的按键说明都已经画在图上了，只要把会变的文字写进中间的面板
    if (g->textures.cover.id > 0)
    {
        Texture2D cover = g->textures.cover;
        Rectangle source = { 0, 0, cover.width, cover.height };
        Rectangle dest = { 0, 0, SCREEN_W, SCREEN_H };            // 铺满整个画面
        DrawTexturePro(cover, source, dest, (Vector2){ 0, 0 }, 0, WHITE);

        drawMenuRows(g, 252, 38, 24);
        drawCenterText(modeHint(g), 398, 16, NEON_PURPLE);
        drawCenterText(g->menuError, 428, 16, NEON_RED);
        return;
    }

    // 没有封面图：用程序画
    drawNeonText("NEON SNAKE", 80, 60, NEON_PINK);
    drawMenuRows(g, 230, 60, 30);
    drawCenterText(modeHint(g), 420, 20, NEON_PURPLE);
    drawCenterText(g->menuError, 470, 20, NEON_RED);

    drawCenterText(TextFormat("Map size: %d - %d", MIN_SIZE, MAX_SIZE), 540, 20, DIM_TEXT);
    drawCenterText("UP/DOWN: select   LEFT/RIGHT or 0-9: change", 570, 20, DIM_TEXT);
    drawNeonText("ENTER: start", 600, 20, NEON_YELLOW);
}

// 画游戏画面（地图、蛇、分数、提示、结束画面）
void drawPlaying(Game *g)
{
    Textures *t = &g->textures;

    // ★ 1. 地板：有 floor.png 就用图片，没有就用代码画金属钢板
    for (int y = 0; y < g->height; y++)
    {
        for (int x = 0; x < g->width; x++)
        {
            if (t->floor.id > 0)
                drawSprite(g, t->floor, x, y, 0, 0, 1, PLATE_COLOR);
            else
                drawMetalPlate(g, x, y);
        }
    }

    // ★ 2. 发光的能量线路
    drawEnergyGrid(g);

    // 3. 食物、障碍物、道具
    for (int y = 0; y < g->height; y++)
    {
        for (int x = 0; x < g->width; x++)
        {
            if (g->map[y][x] == FOOD)
                drawSprite(g, t->apple, x, y, 0, 0, 1, NEON_RED);
            else if (g->map[y][x] == OBSTACLE)
                drawSprite(g, t->obstacle, x, y, 0, 0, 1, NEON_ORANGE);
            else if (g->map[y][x] == BOOSTER)
                drawSprite(g, t->booster, x, y, 0, 0, 1, NEON_PURPLE);
        }
    }

    // ★ 地图外框：青绿色（跟紫色背景对比）。经典模式是实线（墙），无边界模式是虚线（可以穿过）
    Rectangle frame = { g->offsetX - 3, g->offsetY - 3, g->width * g->cell + 6, g->height * g->cell + 6 };
    Rectangle glow  = { frame.x - 3, frame.y - 3, frame.width + 6, frame.height + 6 };
    if (g->mode == CLASSIC)
    {
        DrawRectangleLinesEx(glow, 3, Fade(NEON_GREEN, 0.3f));   // 外面一圈淡淡的光
        DrawRectangleLinesEx(frame, 2, NEON_GREEN);
    }
    else
    {
        drawDashedRect(glow, 3, Fade(NEON_GREEN, 0.25f));
        drawDashedRect(frame, 2, NEON_GREEN);
    }

    drawSnake(g);

    // 上方显示分数、生命、关卡，下面一条霓虹线
    DrawText(TextFormat("Score: %d / %d", g->score, WIN_SCORE), 10, 20, 20, NEON_CYAN);
    DrawText(TextFormat("Lives: %d", g->lives), 200, 20, 20, NEON_PINK);
    DrawText(TextFormat("Level: %d", g->level), 320, 20, 20, NEON_YELLOW);
    DrawText("P: Pause", SCREEN_W - 100, 20, 20, DIM_TEXT);
    DrawLine(0, HUD_H - 1, SCREEN_W, HUD_H - 1, Fade(NEON_PINK, 0.5f));

    // 提示文字
    if (g->messageTimer > 0)
        drawNeonText(g->message, HUD_H + 20, 30, NEON_YELLOW);

    // 暂停画面
    if (g->paused)
    {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.6f));
        drawNeonText("PAUSED", SCREEN_H / 2 - 40, 40, NEON_CYAN);
        drawCenterText("Press P to continue", SCREEN_H / 2 + 20, 20, DIM_TEXT);
    }

    // 结束画面
    if (g->state != PLAYING)
    {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.6f));
        int midY = SCREEN_H / 2;
        if (g->state == WIN)
            drawNeonText("YOU WIN!", midY - 40, 40, NEON_YELLOW);
        else
            drawNeonText("GAME OVER", midY - 40, 40, NEON_RED);
        drawCenterText("R: Restart   M: Menu   Q: Quit", midY + 20, 20, WHITE);
    }
}

// ★ 画出整个画面：先画在固定 600x660 的“画布”上，再按窗口大小缩放贴到窗口里
void drawGame(Game *g, RenderTexture2D canvas)
{
    // 1. 画在画布上（所有座标都照 600x660 来算，不用管窗口多大）
    BeginTextureMode(canvas);
    ClearBackground(BG_COLOR);      // ★ 深紫黑背景
    if (g->state == MENU)
        drawMenu(g);
    else
        drawPlaying(g);
    EndTextureMode();

    // 2. 算出缩放比例：宽、高各自能放大几倍，取小的那个，画面才不会变形或被切掉
    float scaleX = (float)GetScreenWidth() / SCREEN_W;
    float scaleY = (float)GetScreenHeight() / SCREEN_H;
    float scale = scaleX;
    if (scaleY < scale)
        scale = scaleY;

    // 3. 画布缩放后的大小和位置（放在窗口正中间）
    float drawW = SCREEN_W * scale;
    float drawH = SCREEN_H * scale;
    Rectangle source = { 0, 0, SCREEN_W, -SCREEN_H };   // 高度是负的：画布存起来是上下颠倒的，要翻回来
    Rectangle dest = { (GetScreenWidth() - drawW) / 2, (GetScreenHeight() - drawH) / 2, drawW, drawH };

    // 4. 把画布贴到窗口上
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(canvas.texture, source, dest, (Vector2){ 0, 0 }, 0, WHITE);
    EndDrawing();
}

// ★ 打开窗口：可以拉大缩小，而且一开始不会超出屏幕
void openWindow(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);   // 允许拉大缩小
    InitWindow(SCREEN_W, SCREEN_H, "Neon Snake");
    SetWindowMinSize(SCREEN_W / 2, SCREEN_H / 2);

    // 如果屏幕不够高，就把窗口缩小到放得下（留一点空间给 Dock 和标题栏）
    int maxH = GetMonitorHeight(GetCurrentMonitor()) - 150;
    if (SCREEN_H > maxH)
        SetWindowSize(SCREEN_W * maxH / SCREEN_H, maxH);

    SetTargetFPS(60);
}

int main(void)
{
    srand(time(NULL));

    Game game;
    initGame(&game);

    openWindow();
    RenderTexture2D canvas = LoadRenderTexture(SCREEN_W, SCREEN_H);  // 600x660 的画布
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);       // 缩放时比较平滑

    InitAudioDevice();                // ★ 打开声音
    loadSounds(&game.sounds);         // 做好音效
    loadTextures(&game.textures);     // ★ 读进图片

    int quit = 0;
    while (!WindowShouldClose() && !quit)
    {
        quit = updateGame(&game);
        drawGame(&game, canvas);
    }

    unloadTextures(&game.textures);   // ★ 释放图片
    unloadSounds(&game.sounds);       // 释放音效
    CloseAudioDevice();               // ★ 关闭声音
    UnloadRenderTexture(canvas);      // 释放画布
    CloseWindow();
    freeGame(&game);
    return 0;
}