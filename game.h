// game.h — shared by all files: constants, colors, structs, and the functions each .c file provides
// AI disclosure: written with help from AI tools (Claude / Claude Code); see README.md.
//   every .c file starts with #include "game.h", so it knows which functions the other files offer

#ifndef GAME_H      // prevents the same .c file from including this header twice
#define GAME_H

#include "raylib.h"

// ===== Screen size =====
#define SCREEN_W 600        // screen of the window
#define HUD_H    60         // height of the score bar at the top
#define BOARD    600        // the limit domain of the map 600x600
#define SCREEN_H (HUD_H + BOARD)
#define BOARD_MARGIN 10     // pixels left around the map for the border

// ===== Game values (good ones to change live in the walkthrough video) =====
#define MOVE_TIME     0.15f // seconds per step (smaller = faster)
#define WIN_SCORE     20    // eat 20 food items to win
#define OBSTACLE_RATE 50    // about 1 obstacle per 50 cells (bigger = fewer)
#define BOOST_TIME    5.0f  // how many seconds the speed boost lasts
#define SHRINK_AMOUNT 3     // how many segments the size reducer removes
#define MIN_LENGTH    3     // shortest possible snake length
#define START_LIVES   1     // number of lives at the start
#define MESSAGE_TIME  1.5f  // how many seconds a message stays on screen
#define SNAKE_SCALE   1.5f  // snake body is drawn this many times the cell size (bigger = thicker)
// snake head is drawn this many times the cell size (kept smaller so it doesn't look like a ball)
#define HEAD_SCALE    1.0f

// Levels
#define FOODS_PER_LEVEL 5      // level up every 5 food items
#define LEVEL_SPEEDUP   0.015f // each level waits 0.015 s less per step (faster)
#define MIN_MOVE_TIME   0.06f  // fastest allowed speed
#define LEVEL_OBSTACLES 2      // 2 more obstacles every level

// Golden apple
// after each normal apple there is a 1 in 4 chance a golden apple appears
#define GOLDEN_CHANCE   4
#define GOLDEN_TIME     6.0f   // seconds before the golden apple disappears
#define GOLDEN_POINTS   3      // points for a golden apple

// Game-feel effects
#define INVINCIBLE_TIME 2.0f   // seconds of invincibility (against obstacles) after losing a life
#define SHAKE_TIME      0.3f   // seconds the screen shakes after a crash
#define SHAKE_POWER     8      // maximum shake in pixels
#define FLASH_TIME      0.3f   // seconds of the red flash
#define MAX_PARTICLES   64     // maximum particles on screen
#define BURST_COUNT     16     // particles per burst
#define PARTICLE_LIFE   0.6f   // seconds a particle lives
#define MAX_POPUPS      8      // maximum floating texts on screen (+1, +3)
#define POPUP_LIFE      0.8f   // seconds a floating text lives
#define MUSIC_VOLUME    0.35f  // background music volume (0 ~ 1)
// new obstacles must be at least this far from the head, so they never appear right in front of it
#define SAFE_DISTANCE   4

// ===== Map size range and defaults =====
#define MIN_SIZE     5
#define MAX_SIZE     30
#define DEFAULT_W    15
#define DEFAULT_H    12

// ===== Cyberpunk colors =====
#define BG_COLOR     (Color){ 12, 6, 28, 255 }     // dark purple-black background
#define NEON_PINK    (Color){ 255, 45, 200, 255 }
#define NEON_CYAN    (Color){ 0, 240, 255, 255 }
// aqua green: map border (contrasts with the purple background)
#define NEON_GREEN   (Color){ 0, 255, 170, 255 }
#define NEON_PURPLE  (Color){ 170, 80, 255, 255 }
#define NEON_YELLOW  (Color){ 255, 230, 0, 255 }
#define NEON_RED     (Color){ 255, 50, 80, 255 }
#define NEON_ORANGE  (Color){ 255, 140, 0, 255 }
#define DIM_TEXT     (Color){ 120, 100, 160, 255 }  // less important text

// ===== Metal floor colors =====
// plate base color (dark green, so the purple snake stands out)
#define PLATE_COLOR  (Color){ 16, 46, 34, 255 }
#define PLATE_LIGHT  (Color){ 30, 74, 56, 255 }    // plate highlight (top-left edge)
#define PLATE_DARK   (Color){ 6, 22, 15, 255 }     // plate shadow (bottom-right edge)
#define RIVET_COLOR  (Color){ 52, 104, 82, 255 }   // rivets
#define GOLD_COLOR   (Color){ 255, 200, 40, 255 }  // golden apple glow

// what a map cell can contain
#define EMPTY    0
#define FOOD     1
#define OBSTACLE 2
#define BOOSTER  3
#define GOLDEN   4          // Golden apple

// game states
#define PLAYING 0
#define LOSE    1
#define WIN     2
#define MENU    3           // start menu

// game modes
#define CLASSIC  0          // Classic: hitting the wall costs a life
#define NO_WALLS 1          // No Walls: come out on the opposite side

// booster types
#define SPEED_BOOST  0
#define SIZE_REDUCER 1
#define EXTRA_LIFE   2

// snake data
typedef struct {
    int *x;          // x of every segment, x[0] is the head
    int *y;          // y of every segment
    int length;      // current number of segments
    int dirX, dirY;  // moving direction (the direction of the last real step)

    // key queue: if two keys are pressed within one step (e.g. W then A quickly), both are used in
    //   order instead of being lost
    int queueX[2], queueY[2];
    int queueCount;

    // data for smooth movement
    // which cell the tail was in last step (it slides from there when drawn)
    int prevTailX, prevTailY;
    int tailDirX, tailDirY;       // direction the tail slides this step
} Snake;

// one particle (the small squares that burst out when something is eaten)
typedef struct {
    float x, y;          // position (pixels)
    float vx, vy;        // velocity (pixels per second)
    float life;          // seconds left; 0 = not in use
    Color color;
} Particle;

// floating text (e.g. the "+1" that rises when an apple is eaten)
typedef struct {
    float x, y;
    float life;          // seconds left; 0 = not in use
    char text[8];
    Color color;
} Popup;

// all sound effects
typedef struct {
    Sound eat;       // eat food
    Sound booster;   // eat a booster
    Sound crash;     // crash
    Sound levelUp;   // level up
    Sound win;       // win
    Sound golden;    // eat a golden apple
    Sound music;     // background music (8 seconds, looped)
} Sounds;

// all images (pixel art sprites)
typedef struct {
    Texture2D head;      // snake head (drawn facing right)
    Texture2D body;      // snake body (straight piece drawn horizontally: connects left and right)
    Texture2D corner;    // snake body corner (drawn connecting right and bottom)
    Texture2D tail;      // snake tail (drawn moving right: tip on the left)
    Texture2D apple;     // apple
    Texture2D booster;   // booster
    Texture2D obstacle;  // obstacle
    Texture2D floor;     // floor (optional; if missing, the code draws metal plates)
    // start menu cover (600x660, middle panel left empty, the code writes the text)
    Texture2D cover;
} Textures;

// all game data lives here, grouped by topic
typedef struct {
    // ----- game state -----
    int state;               // PLAYING / LOSE / WIN / MENU
    int mode;                // CLASSIC / NO_WALLS
    int score;
    int lives;
    int level;               // current level
    int paused;              // 1 = paused

    // ----- map -----
    int **map;               // 2D array from malloc, NULL before the game starts
    int width, height;       // map size (in cells)
    int cell;                // size of one cell (pixels)
    int offsetX, offsetY;    // where the map is drawn in the window (centered)

    // ----- snake -----
    Snake snake;

    // ----- timers (seconds) -----
    float timer;             // time collected toward the next step
    float speedTimer;        // speed boost left
    float goldenTimer;       // golden apple left (0 = no golden apple on the map)
    float invincibleTimer;   // invincibility left
    float messageTimer;      // message left on screen
    char message[32];        // message text (copied in)

    // ----- start menu -----
    int menuSelected;        // selected row (0 width, 1 height, 2 mode)
    const char *menuError;   // error text

    // ----- effects -----
    float shakeTimer;        // screen shake left
    float flashTimer;        // red flash left
    Particle particles[MAX_PARTICLES];   // fixed-size arrays inside Game (not global variables)
    Popup popups[MAX_POPUPS];

    // ----- assets -----
    Sounds sounds;
    Textures textures;
    int musicOn;             // 1 = background music on
} Game;

// ===== Functions shared between files =====
// Only functions used by another file are listed here.
// Helper functions used inside a single file are marked "static" in that file.

// map.c: the map (created with malloc) and placing items with rand()
int **createMap(int width, int height);
void freeMap(int **map, int height);
void placeItem(Game *g, int type);
void neighborDir(int ax, int ay, int bx, int by, int *dx, int *dy);
void segmentDir(Game *g, int i, int j, int *dx, int *dy);

// game.c: game rules (starting, every step, boosters, lives, levels)
void initGame(Game *g);
void startGame(Game *g);
void freeGame(Game *g);
float getMoveTime(Game *g);
int updateGame(Game *g);                // returns 1 when the player quits

// input.c: keyboard input
void updateMenu(Game *g);
void handleInput(Game *g);
void takeQueuedDirection(Snake *s);

// assets.c: sounds made in code, images from the assets folder
void loadSounds(Sounds *s);
void unloadSounds(Sounds *s);
void loadTextures(Textures *t);
void unloadTextures(Textures *t);

// draw.c: drawing and the window
void openWindow(void);
void drawGame(Game *g, RenderTexture2D canvas);

// effects.c: particles, floating text, shake, flash, background music
void spawnBurst(Game *g, int cellX, int cellY, Color color);
void addPopup(Game *g, int cellX, int cellY, const char *text, Color color);
void startCrashEffect(Game *g);
void updateEffects(Game *g, float dt);
void drawEffects(Game *g);
void updateMusic(Game *g);
void toggleMusic(Game *g);

#endif
