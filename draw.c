// draw.c — 画面：所有画图的函数，以及打开窗口

#include <math.h>     // sinf、fminf
#include "game.h"

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
    Color line = Fade(NEON_GREEN, glow);   // ★ 绿色地板配绿色的能量线

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
