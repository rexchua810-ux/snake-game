// game.c — 游戏规则：开始与结束、道具与生命、关卡、蛇每走一步要检查的事

#include <stdio.h>    // snprintf
#include <stdlib.h>
#include "game.h"

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

    // ★ 清掉金苹果和所有特效
    g->goldenTimer = 0;
    g->invincibleTimer = 0;
    g->shakeTimer = 0;
    g->flashTimer = 0;
    for (int i = 0; i < MAX_PARTICLES; i++)
        g->particles[i].life = 0;
    for (int i = 0; i < MAX_POPUPS; i++)
        g->popups[i].life = 0;

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
    g->musicOn = 1;            // ★ 一开始就播放背景音乐
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
    spawnBurst(g, g->snake.x[0], g->snake.y[0], NEON_PINK);   // ★ 在蛇头的位置炸开
    startCrashEffect(g);           // ★ 画面震动 + 红色闪光
    g->lives--;
    if (g->lives <= 0)
    {
        g->state = LOSE;
    }
    else
    {
        resetSnake(g);
        g->speedTimer = 0;
        g->invincibleTimer = INVINCIBLE_TIME;   // ★ 复活后无敌一下子
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

// ★ 分数够了就升级。用 while：金苹果一次加 3 分，可能一口气跨过升级的门槛
void checkLevelUp(Game *g)
{
    while (g->level < g->score / FOODS_PER_LEVEL + 1)
        levelUp(g);
}

// ★ 把地图上的金苹果拿掉（时间到了）
void removeGolden(Game *g)
{
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            if (g->map[y][x] == GOLDEN)
                g->map[y][x] = EMPTY;
    g->goldenTimer = 0;
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

    int invincible = (g->invincibleTimer > 0);   // ★ 无敌中：撞到障碍物和自己都不算

    // 2. 撞障碍物
    if (g->map[newY][newX] == OBSTACLE && !invincible)
    {
        loseLife(g);
        return;
    }

    int ate = (g->map[newY][newX] == FOOD);
    int gotGolden = (g->map[newY][newX] == GOLDEN);   // ★ 金苹果
    int grow = (ate || gotGolden);                      // ★ 两种苹果都会变长

    // ★ 先记住尾巴原本在哪（画的时候要从那里滑过去）
    //   一定要在“吃到食物、长度 +1”之前记，不然会读到还没用过的格子，尾巴会从角落飞过来
    int oldTailX = s->x[s->length - 1];
    int oldTailY = s->y[s->length - 1];
    int gotBooster = (g->map[newY][newX] == BOOSTER);

    // 3. 撞到自己（没吃到食物时尾巴会移开，所以尾巴那一格不算）
    int checkLength = s->length;
    if (!grow)
        checkLength = s->length - 1;
    for (int i = 0; i < checkLength && !invincible; i++)
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
        spawnBurst(g, newX, newY, NEON_RED);             // ★ 炸开红色粒子
        addPopup(g, newX, newY, "+1", NEON_YELLOW);      // ★ 飘出 +1
    }

    // ★ 吃到金苹果：长度 +1、分数 +3
    if (gotGolden)
    {
        s->length++;
        g->score += GOLDEN_POINTS;
        PlaySound(g->sounds.golden);
        g->map[newY][newX] = EMPTY;
        g->goldenTimer = 0;
        spawnBurst(g, newX, newY, GOLD_COLOR);
        addPopup(g, newX, newY, "+3", GOLD_COLOR);
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

    // 6. 够分数就赢；不够就放新食物、看看要不要升级
    if (grow)
    {
        if (g->score >= WIN_SCORE)
        {
            g->state = WIN;
            PlaySound(g->sounds.win);   // ★
        }
        else
        {
            if (ate)
            {
                placeItem(g, FOOD);
                // ★ 地图上还没有金苹果时，有 1/GOLDEN_CHANCE 的机会出现一个
                if (g->goldenTimer <= 0 && rand() % GOLDEN_CHANCE == 0)
                {
                    placeItem(g, GOLDEN);
                    g->goldenTimer = GOLDEN_TIME;
                }
            }
            checkLevelUp(g);
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
    if (g->invincibleTimer > 0)          // ★ 无敌倒数
        g->invincibleTimer -= dt;
    if (g->goldenTimer > 0)              // ★ 金苹果倒数，时间到就消失
    {
        g->goldenTimer -= dt;
        if (g->goldenTimer <= 0)
            removeGolden(g);
    }
    updateEffects(g, dt);                // ★ 粒子、飘字、震动、闪光

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
    // ★ 背景音乐：N 开关；播完了就从头再播（任何画面都有效）
    if (IsKeyPressed(KEY_N))
        toggleMusic(g);
    updateMusic(g);

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
        updateEffects(g, GetFrameTime());   // ★ 结束画面也要让震动、粒子跑完
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
