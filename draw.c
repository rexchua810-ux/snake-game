// draw.c — drawing functions and opening the window
// AI disclosure: parts of this file were written with the help of AI tools (Claude / Claude Code).
//   The author reviewed the code, tested it in the game, and can explain how it works.
//   See "Use of Generative AI" in README.md for details.

#include <stdlib.h>   // rand
#include <math.h>     // sinf、fminf
#include "game.h"

// ================= Drawing =================

// draw text centered horizontally
void drawCenterText(const char *text, int y, int size, Color color)
{
    int textWidth = MeasureText(text, size);
    DrawText(text, (SCREEN_W - textWidth) / 2, y, size, color);
}

// glowing text: draw a few semi-transparent copies around it as the "glow", then the text itself
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

// direction → rotation angle (images face right, other directions are rotated)
//   right 0°, down 90°, left 180°, up 270° (screen y grows downwards, so angles go clockwise)
float directionToAngle(int dx, int dy)
{
    if (dx == 1)  return 0;
    if (dy == 1)  return 90;
    if (dx == -1) return 180;
    return 270;
}

// draw an image on cell (x, y), with rotation; flip = 1 mirrors it horizontally. Without an image, draw a colored block
//   x and y are floats: the image can be drawn between two cells (e.g. 3.5), so the snake can slide smoothly
//   w, h: image width and height in cells (e.g. 1.5 = 1.5 times a cell)
void drawSpriteSize(Game *g, Texture2D texture, float x, float y, float angle, int flip,
                    float w, float h, Color fallback)
{
    float cell = g->cell;
    float left = g->offsetX + x * cell;
    float top  = g->offsetY + y * cell;

    if (texture.id == 0)       // no image yet: draw a colored block
    {
        DrawRectangleRec((Rectangle){ left + 2, top + 2, cell - 4, cell - 4 }, fallback);
        return;
    }

    Rectangle source = { 0, 0, texture.width, texture.height };      // the whole image
    if (flip)
        source.width = -source.width;                                 // negative width = mirrored horizontally
    Rectangle dest = { left + cell / 2, top + cell / 2, w * cell, h * cell }; // drawn at the cell center
    Vector2 origin = { w * cell / 2, h * cell / 2 };                  // rotate around the center
    DrawTexturePro(texture, source, dest, origin, angle, WHITE);
}

// normal drawing: same width and height, scale = how many times bigger
void drawSprite(Game *g, Texture2D texture, float x, float y, float angle, int flip, float scale, Color fallback)
{
    drawSpriteSize(g, texture, x, y, angle, flip, scale, scale, fallback);
}

// corner rotation. corner.png connects right and bottom; rotated 90° it connects bottom and left, and so on
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

// turn progress 0 ~ 1 into "slow - fast - slow", so turning the head looks natural
float smoothStep(float p)
{
    if (p < 0) p = 0;
    if (p > 1) p = 1;
    return p * p * (3 - 2 * p);
}

// turn smoothly from angle a to angle b (p goes from 0 to 1), the shorter way round
float turnAngle(float a, float b, float p)
{
    float diff = b - a;
    while (diff > 180)  diff -= 360;
    while (diff < -180) diff += 360;
    return a + diff * smoothStep(p);
}

// draw one body segment: use the directions towards the head (ax, ay) and towards the tail (bx, by) to choose straight or corner
void drawBodyPiece(Game *g, int x, int y, int ax, int ay, int bx, int by, Color color)
{
    Textures *t = &g->textures;
    if (ax == -bx && ay == -by)           // opposite directions → straight
    {
        float angle = 0;                  // horizontal
        if (ax == 0)
            angle = 90;                   // vertical
        // exactly 1 cell long (segments don't overlap their patterns), thickness scaled by SNAKE_SCALE
        drawSpriteSize(g, t->body, x, y, angle, 0, 1, SNAKE_SCALE, color);
    }
    else                                  // otherwise → corner
    {
        Texture2D piece = t->corner;
        if (piece.id == 0)                // no corner image yet: use the straight piece
            piece = t->body;
        drawSprite(g, piece, x, y, cornerAngle(ax, ay, bx, by), 0, SNAKE_SCALE, color);
    }
}

// draw a sliding segment (head or tail): from cell (fromX, fromY), p cells in direction (dx, dy)
//   No Walls mode, crossing an edge: draw it on both sides, clipped to the map, so it looks like it comes out of the opposite side
void drawSliding(Game *g, Texture2D texture, int fromX, int fromY, int dx, int dy, float p,
                 float angle, float scale, Color color)
{
    float x = fromX + dx * p;
    float y = fromY + dy * p;
    int toX = fromX + dx;
    int toY = fromY + dy;

    if (toX >= 0 && toX < g->width && toY >= 0 && toY < g->height)   // normal case
    {
        drawSprite(g, texture, x, y, angle, 0, scale, color);
        return;
    }

    BeginScissorMode(g->offsetX, g->offsetY, g->width * g->cell, g->height * g->cell);
    drawSprite(g, texture, x, y, angle, 0, scale, color);                               // the part leaving
    drawSprite(g, texture, x - dx * g->width, y - dy * g->height, angle, 0, scale, color); // the part coming in on the opposite side
    EndScissorMode();
}

// draw the snake (smooth version)
//   body segments stay in their cells; only the head and tail slide between two cells depending on how far this step has gone.
//   This makes the snake crawl continuously instead of jumping cell by cell.
void drawSnake(Game *g)
{
    Snake *s = &g->snake;
    Textures *t = &g->textures;

    // speed boost: yellow blocks when there is no image
    Color bodyColor = NEON_CYAN;
    if (g->speedTimer > 0)
        bodyColor = NEON_YELLOW;

    // progress of this step: 0 = previous step just finished, 1 = next step about to happen
    float p = g->timer / getMoveTime(g);
    if (p > 1 || g->state != PLAYING)     // game over: stop in the middle of the cell
        p = 1;

    // the snake images are bigger than a cell and overlap, so draw from tail to head: pieces nearer the head are drawn later, on top

    // 1. put a body piece under the cell the tail is entering (otherwise there is a gap between tail and body)
    int last = s->length - 1;
    int tailMoving = (s->prevTailX != s->x[last] || s->prevTailY != s->y[last]);
    if (tailMoving)
    {
        int ax, ay;
        segmentDir(g, last, last - 1, &ax, &ay);
        drawBodyPiece(g, s->x[last], s->y[last], ax, ay, -s->tailDirX, -s->tailDirY, bodyColor);
    }

    // 2. tail: slides in from the previous cell; turns smoothly at corners (within the first half of the step)
    float tailAngle = turnAngle(directionToAngle(s->oldTailDirX, s->oldTailDirY),
                                directionToAngle(s->tailDirX, s->tailDirY), p * 2);
    if (tailMoving)
        drawSliding(g, t->tail, s->prevTailX, s->prevTailY, s->tailDirX, s->tailDirY, p,
                    tailAngle, SNAKE_SCALE, NEON_PURPLE);
    else    // just ate: the tail does not move this step
        drawSprite(g, t->tail, s->x[last], s->y[last], tailAngle, 0, SNAKE_SCALE, NEON_PURPLE);

    // 3. body: from the second-to-last segment to segment 1 (all centered in their cells, not moving)
    for (int i = last - 1; i >= 1; i--)
    {
        int ax, ay, bx, by;
        segmentDir(g, i, i - 1, &ax, &ay);   // direction towards the head
        segmentDir(g, i, i + 1, &bx, &by);   // direction towards the tail
        drawBodyPiece(g, s->x[i], s->y[i], ax, ay, bx, by, bodyColor);
    }

    // 4. head: slides from segment 1 towards segment 0; turns smoothly. Drawn last so it is on top
    //    uses the direction actually moved, not the key just pressed, so the head doesn't turn early
    int dx, dy, oldDx, oldDy;
    segmentDir(g, 1, 0, &dx, &dy);           // direction of this step
    segmentDir(g, 2, 1, &oldDx, &oldDy);     // direction of the previous step
    float angle = turnAngle(directionToAngle(oldDx, oldDy), directionToAngle(dx, dy), p * 2);
    drawSliding(g, t->head, s->x[1], s->y[1], dx, dy, p, angle, HEAD_SCALE, NEON_PINK);

    // speed boost: yellow outline around the head
    if (g->speedTimer > 0)
    {
        Rectangle box = { g->offsetX + (s->x[1] + dx * p) * g->cell,
                          g->offsetY + (s->y[1] + dy * p) * g->cell, g->cell, g->cell };
        DrawRectangleLinesEx(box, 1, NEON_YELLOW);
    }
}

// draw one metal plate (one cell) in code: base color + light top-left edge + dark bottom-right edge + four rivets
void drawMetalPlate(Game *g, int x, int y)
{
    int c = g->cell;
    int left = g->offsetX + x * c;
    int top  = g->offsetY + y * c;

    DrawRectangle(left, top, c, c, PLATE_COLOR);
    DrawLine(left, top + 1, left + c, top + 1, PLATE_LIGHT);            // top highlight
    DrawLine(left + 1, top, left + 1, top + c, PLATE_LIGHT);            // left highlight
    DrawLine(left, top + c - 1, left + c, top + c - 1, PLATE_DARK);     // bottom shadow
    DrawLine(left + c - 1, top, left + c - 1, top + c, PLATE_DARK);     // right shadow

    if (c >= 24)              // only draw rivets when the cell is big enough, otherwise they get crowded
    {
        float r = c / 16.0f + 1;   // rivet radius
        int d = c / 6;             // rivet distance from the corner
        DrawCircle(left + d,     top + d,     r, RIVET_COLOR);
        DrawCircle(left + c - d, top + d,     r, RIVET_COLOR);
        DrawCircle(left + d,     top + c - d, r, RIVET_COLOR);
        DrawCircle(left + c - d, top + c - d, r, RIVET_COLOR);
    }
}

// glowing energy lines: the grid slowly gets brighter and darker over time
void drawEnergyGrid(Game *g)
{
    // sinf gives -1 ~ 1, so the transparency moves between 0.2 and 0.5
    float glow = 0.35f + 0.15f * sinf(GetTime() * 2);
    Color line = Fade(NEON_GREEN, glow);   // green energy lines to match the green floor

    int right  = g->offsetX + g->width * g->cell;
    int bottom = g->offsetY + g->height * g->cell;

    for (int x = 0; x <= g->width; x++)        // vertical lines
    {
        int px = g->offsetX + x * g->cell;
        DrawLine(px, g->offsetY, px, bottom, line);
    }
    for (int y = 0; y <= g->height; y++)       // horizontal lines
    {
        int py = g->offsetY + y * g->cell;
        DrawLine(g->offsetX, py, right, py, line);
    }
}

// the three options on the start menu (width, height, mode): the selected one glows, the others are dim
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
            drawNeonText(rows[i], firstY + i * gap, size, NEON_CYAN);   // the selected one glows
        else
            drawCenterText(rows[i], firstY + i * gap, size, DIM_TEXT);
    }
}

// mode description
const char *modeHint(Game *g)
{
    if (g->mode == CLASSIC)
        return "Hit the wall = lose a life";
    return "Go through walls to the other side";
}

// draw a dashed rectangle: every 12 pixels, 8 drawn and 4 empty
void drawDashedRect(Rectangle r, float thick, Color color)
{
    float dash = 8, step = 12;
    for (float x = r.x; x < r.x + r.width; x += step)          // top and bottom
    {
        float w = fminf(dash, r.x + r.width - x);
        DrawRectangleRec((Rectangle){ x, r.y, w, thick }, color);
        DrawRectangleRec((Rectangle){ x, r.y + r.height - thick, w, thick }, color);
    }
    for (float y = r.y; y < r.y + r.height; y += step)         // left and right
    {
        float h = fminf(dash, r.y + r.height - y);
        DrawRectangleRec((Rectangle){ r.x, y, thick, h }, color);
        DrawRectangleRec((Rectangle){ r.x + r.width - thick, y, thick, h }, color);
    }
}

// draw the start menu
void drawMenu(Game *g)
{
    // with a cover image: the title and key hints are already in the image, only the changing text is written into the middle panel
    if (g->textures.cover.id > 0)
    {
        Texture2D cover = g->textures.cover;
        Rectangle source = { 0, 0, cover.width, cover.height };
        Rectangle dest = { 0, 0, SCREEN_W, SCREEN_H };            // fill the whole screen
        DrawTexturePro(cover, source, dest, (Vector2){ 0, 0 }, 0, WHITE);

        drawMenuRows(g, 252, 38, 24);
        drawCenterText(modeHint(g), 398, 16, NEON_PURPLE);
        drawCenterText(g->menuError, 428, 16, NEON_RED);
        return;
    }

    // no cover image: draw it in code
    drawNeonText("NEON SNAKE", 80, 60, NEON_PINK);
    drawMenuRows(g, 230, 60, 30);
    drawCenterText(modeHint(g), 420, 20, NEON_PURPLE);
    drawCenterText(g->menuError, 470, 20, NEON_RED);

    drawCenterText(TextFormat("Map size: %d - %d", MIN_SIZE, MAX_SIZE), 540, 20, DIM_TEXT);
    drawCenterText("UP/DOWN: select   LEFT/RIGHT or 0-9: change", 570, 20, DIM_TEXT);
    drawNeonText("ENTER: start", 600, 20, NEON_YELLOW);
}

// draw the game screen (map, snake, score, messages, end screen)
void drawPlaying(Game *g)
{
    Textures *t = &g->textures;

    // 1. floor: use floor.png if it exists, otherwise draw metal plates in code
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

    // 2. glowing energy lines
    drawEnergyGrid(g);

    // 3. food, obstacles, boosters
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
            else if (g->map[y][x] == GOLDEN)
                drawGoldenApple(g, x, y);       // Golden apple
        }
    }

    // map border: aqua green (contrasts with purple). Solid line in Classic (wall), dashed in No Walls (can pass through)
    Rectangle frame = { g->offsetX - 3, g->offsetY - 3, g->width * g->cell + 6, g->height * g->cell + 6 };
    Rectangle glow  = { frame.x - 3, frame.y - 3, frame.width + 6, frame.height + 6 };
    if (g->mode == CLASSIC)
    {
        DrawRectangleLinesEx(glow, 3, Fade(NEON_GREEN, 0.3f));   // soft glow around it
        DrawRectangleLinesEx(frame, 2, NEON_GREEN);
    }
    else
    {
        drawDashedRect(glow, 3, Fade(NEON_GREEN, 0.25f));
        drawDashedRect(frame, 2, NEON_GREEN);
    }

    // invincible: the snake blinks (switches between drawn / hidden every 0.1 s)
    if (g->invincibleTimer <= 0 || (int)(g->invincibleTimer * 10) % 2 == 0)
        drawSnake(g);

    drawEffects(g);     // particles, floating text, red flash

    // score, lives and level at the top, with a neon line underneath
    DrawText(TextFormat("Score: %d / %d", g->score, WIN_SCORE), 10, 20, 20, NEON_CYAN);
    DrawText(TextFormat("Lives: %d", g->lives), 200, 20, 20, NEON_PINK);
    DrawText(TextFormat("Level: %d", g->level), 320, 20, 20, NEON_YELLOW);
    DrawText("P: Pause", SCREEN_W - 100, 20, 20, DIM_TEXT);
    DrawLine(0, HUD_H - 1, SCREEN_W, HUD_H - 1, Fade(NEON_PINK, 0.5f));

    // message text
    if (g->messageTimer > 0)
        drawNeonText(g->message, HUD_H + 20, 30, NEON_YELLOW);

    // pause screen
    if (g->paused)
    {
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.6f));
        drawNeonText("PAUSED", SCREEN_H / 2 - 40, 40, NEON_CYAN);
        drawCenterText("Press P to continue", SCREEN_H / 2 + 20, 20, DIM_TEXT);
        if (g->musicOn)
            drawCenterText("N: Music ON", SCREEN_H / 2 + 50, 20, DIM_TEXT);
        else
            drawCenterText("N: Music OFF", SCREEN_H / 2 + 50, 20, DIM_TEXT);
    }

    // end screen
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

// draw everything: first on a fixed 600x660 "canvas", then scale it to fit the window
void drawGame(Game *g, RenderTexture2D canvas)
{
    // 1. draw on the canvas (all coordinates assume 600x660, no matter the window size)
    BeginTextureMode(canvas);
    ClearBackground(BG_COLOR);      // dark purple-black background
    if (g->state == MENU)
        drawMenu(g);
    else
        drawPlaying(g);
    EndTextureMode();

    // 2. scale factor: how much width and height can each grow; use the smaller one so nothing is stretched or cut off
    float scaleX = (float)GetScreenWidth() / SCREEN_W;
    float scaleY = (float)GetScreenHeight() / SCREEN_H;
    float scale = scaleX;
    if (scaleY < scale)
        scale = scaleY;

    // 3. size and position of the scaled canvas (centered in the window)
    float drawW = SCREEN_W * scale;
    float drawH = SCREEN_H * scale;
    Rectangle source = { 0, 0, SCREEN_W, -SCREEN_H };   // negative height: the canvas is stored upside down and must be flipped back
    Rectangle dest = { (GetScreenWidth() - drawW) / 2, (GetScreenHeight() - drawH) / 2, drawW, drawH };

    // screen shake: move the whole picture a few pixels in a random direction, less and less towards the end
    if (g->shakeTimer > 0)
    {
        int power = (int)(SHAKE_POWER * g->shakeTimer / SHAKE_TIME) + 1;
        dest.x += (rand() % (2 * power + 1) - power) * scale;
        dest.y += (rand() % (2 * power + 1) - power) * scale;
    }

    // 4. draw the canvas onto the window
    BeginDrawing();
    ClearBackground(BLACK);
    DrawTexturePro(canvas.texture, source, dest, (Vector2){ 0, 0 }, 0, WHITE);
    EndDrawing();
}

// golden apple: a "breathing" golden glow behind the apple and small stars circling it; blinks when about to disappear
void drawGoldenApple(Game *g, int x, int y)
{
    if (g->goldenTimer < 2 && (int)(g->goldenTimer * 8) % 2 == 0)   // less than 2 seconds left: blink
        return;

    float cx = g->offsetX + (x + 0.5f) * g->cell;     // cell center (pixels)
    float cy = g->offsetY + (y + 0.5f) * g->cell;
    float pulse = 0.5f + 0.5f * sinf(GetTime() * 6);  // moves between 0 and 1

    DrawCircle(cx, cy, g->cell * (0.55f + 0.1f * pulse), Fade(GOLD_COLOR, 0.25f + 0.2f * pulse));
    drawSprite(g, g->textures.apple, x, y, 0, 0, 1.1f, GOLD_COLOR);   // apple drawn a bit bigger

    // two small stars (diamonds) circling the apple
    float angle = GetTime() * 3;
    for (int i = 0; i < 2; i++)
    {
        float a = angle + i * 3.14159f;
        Vector2 star = { cx + cosf(a) * g->cell * 0.55f, cy + sinf(a) * g->cell * 0.55f };
        DrawPoly(star, 4, g->cell * 0.1f, 45, GOLD_COLOR);
    }
}

// open the window: resizable, and never bigger than the screen at the start
void openWindow(void)
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);   // allow resizing
    InitWindow(SCREEN_W, SCREEN_H, "Neon Snake");
    SetWindowMinSize(SCREEN_W / 2, SCREEN_H / 2);

    // if the screen is not tall enough, shrink the window to fit (leave room for the Dock and title bar)
    int maxH = GetMonitorHeight(GetCurrentMonitor()) - 150;
    if (SCREEN_H > maxH)
        SetWindowSize(SCREEN_W * maxH / SCREEN_H, maxH);

    SetTargetFPS(60);
}
