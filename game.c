// game.c — game rules: start/end, boosters and lives, levels, what happens on every step
// AI disclosure: parts of this file were written with the help of AI tools (Claude / Claude Code).
//   The author reviewed the code, tested it in the game, and can explain how it works.
//   See "Use of Generative AI" in README.md for details.

#include <stdio.h>    // snprintf
#include <stdlib.h>
#include "game.h"

// ================= Starting and ending a game =================

// show a message (e.g. "Speed Up!")
void showMessage(Game *g, const char *text)
{
    snprintf(g->message, sizeof(g->message), "%s", text);  // copy the text into message
    g->messageTimer = MESSAGE_TIME;
}

// put the snake back at the start (length 3, moving right)
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
    g->snake.prevTailX = g->snake.x[2];   // tail is not moving
    g->snake.prevTailY = g->snake.y[2];
    g->snake.tailDirX = 1;
    g->snake.tailDirY = 0;
    g->snake.oldTailDirX = 1;
    g->snake.oldTailDirY = 0;
    g->timer = 0;
}

// start a new round (same map size)
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
    g->message[0] = '\0';     // clear the message
    g->messageTimer = 0;
    g->level = 1;
    g->paused = 0;

    // remove the golden apple and all effects
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

// Enter on the start menu: allocate memory for the chosen size, then start
void startGame(Game *g)
{
    // work out the cell size so the map fits in the 600x600 area (with BOARD_MARGIN around it for the border)
    int space = BOARD - 2 * BOARD_MARGIN;
    if (g->width > g->height)
        g->cell = space / g->width;
    else
        g->cell = space / g->height;

    // center the map in the area
    g->offsetX = (SCREEN_W - g->width * g->cell) / 2;
    g->offsetY = HUD_H + (BOARD - g->height * g->cell) / 2;

    g->map = createMap(g->width, g->height);
    g->snake.x = malloc(g->width * g->height * sizeof(int));  // the snake can never be longer than the whole map
    g->snake.y = malloc(g->width * g->height * sizeof(int));

    resetGame(g);
}

// free all memory from malloc
void freeGame(Game *g)
{
    if (g->map == NULL)      // game not started yet: nothing to free
        return;
    freeMap(g->map, g->height);
    free(g->snake.x);
    free(g->snake.y);
    g->map = NULL;
}

// program start: set default values and show the start menu
void initGame(Game *g)
{
    g->map = NULL;
    g->width = DEFAULT_W;
    g->height = DEFAULT_H;
    g->mode = CLASSIC;
    g->state = MENU;
    g->menuSelected = 0;
    g->menuError = "";
    g->musicOn = 1;            // background music is on from the start
}

// ================= Boosters and lives =================

// booster eaten: rand() decides which type, then apply its effect
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
        // the tail jumps straight to its new position, no sliding this step
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

// crash: lose a life. With lives left, restart from the start position; with none left, the game is lost
void loseLife(Game *g)
{
    PlaySound(g->sounds.crash);
    spawnBurst(g, g->snake.x[0], g->snake.y[0], NEON_PINK);   // burst at the head's position
    startCrashEffect(g);           // screen shake + red flash
    g->lives--;
    if (g->lives <= 0)
    {
        g->state = LOSE;
    }
    else
    {
        resetSnake(g);
        g->speedTimer = 0;
        g->invincibleTimer = INVINCIBLE_TIME;   // short invincibility after respawning
        showMessage(g, "Lost a life!");
    }
}

// level up: add obstacles and show "Level 2!"
void levelUp(Game *g)
{
    g->level++;
    PlaySound(g->sounds.levelUp);
    for (int i = 0; i < LEVEL_OBSTACLES; i++)
        placeItem(g, OBSTACLE);

    char text[32];
    snprintf(text, sizeof(text), "Level %d!", g->level);
    showMessage(g, text);
}

// level up when the score is high enough. Uses while: a golden apple adds 3 points and may cross a level boundary
void checkLevelUp(Game *g)
{
    while (g->level < g->score / FOODS_PER_LEVEL + 1)
        levelUp(g);
}

// remove the golden apple from the map (time is up)
void removeGolden(Game *g)
{
    for (int y = 0; y < g->height; y++)
        for (int x = 0; x < g->width; x++)
            if (g->map[y][x] == GOLDEN)
                g->map[y][x] = EMPTY;
    g->goldenTimer = 0;
}

// time per step for the current level (higher level = faster)
float getMoveTime(Game *g)
{
    float moveTime = MOVE_TIME - (g->level - 1) * LEVEL_SPEEDUP;
    if (moveTime < MIN_MOVE_TIME)
        moveTime = MIN_MOVE_TIME;
    if (g->speedTimer > 0)      // speed booster: twice as fast
        moveTime = moveTime / 2;
    return moveTime;
}

// ================= Playing =================

// move the snake one step: check crashes first, then whether something was eaten
void stepGame(Game *g)
{
    Snake *s = &g->snake;
    takeQueuedDirection(s);              // the turn actually happens here
    int newX = s->x[0] + s->dirX;
    int newY = s->y[0] + s->dirY;

    // 1. leaving the map
    if (newX < 0 || newX >= g->width || newY < 0 || newY >= g->height)
    {
        if (g->mode == CLASSIC)
        {
            loseLife(g);
            return;
        }
        // No Walls mode: come out on the opposite side
        newX = (newX + g->width) % g->width;
        newY = (newY + g->height) % g->height;
    }

    int invincible = (g->invincibleTimer > 0);   // invincible: hitting an obstacle does not count (hitting itself or a wall still does)

    // 2. hit an obstacle
    if (g->map[newY][newX] == OBSTACLE && !invincible)
    {
        loseLife(g);
        return;
    }

    int ate = (g->map[newY][newX] == FOOD);
    int gotGolden = (g->map[newY][newX] == GOLDEN);   // Golden apple
    int grow = (ate || gotGolden);                      // both kinds of apple make the snake longer

    // remember where the tail was (it is drawn sliding from there)
    //   must be saved before "length + 1", otherwise an unused cell is read and the tail flies in from a corner
    int oldTailX = s->x[s->length - 1];
    int oldTailY = s->y[s->length - 1];
    int gotBooster = (g->map[newY][newX] == BOOSTER);

    // 3. hit itself (when nothing is eaten the tail moves away, so the tail cell does not count)
    int checkLength = s->length;
    if (!grow)
        checkLength = s->length - 1;
    for (int i = 0; i < checkLength; i++)
    {
        if (s->x[i] == newX && s->y[i] == newY)
        {
            loseLife(g);
            return;
        }
    }

    // 4. food eaten: length + 1, score + 1
    if (ate)
    {
        s->length++;
        g->score++;
        PlaySound(g->sounds.eat);
        g->map[newY][newX] = EMPTY;
        spawnBurst(g, newX, newY, NEON_RED);             // red particle burst
        addPopup(g, newX, newY, "+1", NEON_YELLOW);      // "+1" floats up
    }

    // golden apple eaten: length + 1, score + 3
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

    // 5. move
    for (int i = s->length - 1; i > 0; i--)
    {
        s->x[i] = s->x[i - 1];
        s->y[i] = s->y[i - 1];
    }
    s->x[0] = newX;
    s->y[0] = newY;

    // tail: remember which cell it slides from and to (the tail stays still after eating)
    s->oldTailDirX = s->tailDirX;
    s->oldTailDirY = s->tailDirY;
    s->prevTailX = oldTailX;
    s->prevTailY = oldTailY;
    int last = s->length - 1;
    if (s->x[last] != oldTailX || s->y[last] != oldTailY)
        neighborDir(oldTailX, oldTailY, s->x[last], s->y[last], &s->tailDirX, &s->tailDirY);

    // 6. enough points wins; otherwise place new food and check for a level up
    if (grow)
    {
        if (g->score >= WIN_SCORE)
        {
            g->state = WIN;
            PlaySound(g->sounds.win);
        }
        else
        {
            if (ate)
            {
                placeItem(g, FOOD);
                // if there is no golden apple yet, 1 in GOLDEN_CHANCE chance to spawn one
                if (g->goldenTimer <= 0 && rand() % GOLDEN_CHANCE == 0)
                {
                    placeItem(g, GOLDEN);
                    g->goldenTimer = GOLDEN_TIME;
                }
            }
            checkLevelUp(g);
        }
    }

    // 7. booster eaten
    if (gotBooster)
    {
        g->map[newY][newX] = EMPTY;
        PlaySound(g->sounds.booster);
        applyBooster(g);
        placeItem(g, BOOSTER);
    }
}

// everything done each frame while playing
void updatePlaying(Game *g)
{
    // P: pause / resume
    if (IsKeyPressed(KEY_P))
        g->paused = !g->paused;
    if (g->paused)
        return;          // paused: do nothing else

    handleInput(g);

    float dt = GetFrameTime();
    if (g->speedTimer > 0)
        g->speedTimer -= dt;
    if (g->messageTimer > 0)
        g->messageTimer -= dt;
    if (g->invincibleTimer > 0)          // invincibility countdown
        g->invincibleTimer -= dt;
    if (g->goldenTimer > 0)              // golden apple countdown, it disappears when time is up
    {
        g->goldenTimer -= dt;
        if (g->goldenTimer <= 0)
            removeGolden(g);
    }
    updateEffects(g, dt);                // particles, floating text, shake, flash

    g->timer += dt;
    float moveTime = getMoveTime(g);   // speed depends on the level
    if (g->timer >= moveTime)
    {
        // subtract instead of resetting to 0: leftover time goes to the next step, so the rhythm stays even
        g->timer -= moveTime;
        if (g->timer > moveTime)       // after a long freeze (e.g. dragging the window) don't take several steps at once
            g->timer = 0;
        stepGame(g);
    }
}

// per-frame update: do the work for the current screen. Returns 1 to quit
int updateGame(Game *g)
{
    // background music: N toggles it; restart it when it ends (works on every screen)
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
    else  // LOSE or WIN
    {
        updateEffects(g, GetFrameTime());   // let shake and particles finish on the end screen too
        if (IsKeyPressed(KEY_R))
            resetGame(g);
        if (IsKeyPressed(KEY_M))     // back to the start menu (the size will be chosen again, so free the memory first)
        {
            freeGame(g);
            g->state = MENU;
        }
        if (IsKeyPressed(KEY_Q))
            return 1;
    }
    return 0;
}
