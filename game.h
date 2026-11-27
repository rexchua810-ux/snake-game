// game.h — 所有文件共用的东西：常数、配色、struct，以及每个 .c 文件提供的函数
//   每个 .c 文件开头都会 #include "game.h"，就知道彼此有哪些函数可以叫

#ifndef GAME_H      // 防止同一个 .c 文件不小心 include 两次
#define GAME_H

#include "raylib.h"

// ===== 画面大小 =====
#define SCREEN_W 600        // screen of the window
#define HUD_H    60         // 上方显示分数的区域高度
#define BOARD    600        // the limit domain of the map 600x600
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

// ★ 金苹果
#define GOLDEN_CHANCE   4      // 每吃一个普通苹果，有 1/4 的机会出现金苹果
#define GOLDEN_TIME     6.0f   // 金苹果几秒后消失
#define GOLDEN_POINTS   3      // 金苹果值几分

// ★ 手感特效
#define INVINCIBLE_TIME 2.0f   // 复活后无敌几秒
#define SHAKE_TIME      0.3f   // 撞到时画面震动几秒
#define SHAKE_POWER     8      // 震动最大几像素
#define FLASH_TIME      0.3f   // 红色闪光几秒
#define MAX_PARTICLES   64     // 画面上最多几颗粒子
#define BURST_COUNT     16     // 吃一次炸出几颗
#define PARTICLE_LIFE   0.6f   // 粒子活几秒
#define MAX_POPUPS      8      // 画面上最多几个飘字（+1、+3）
#define POPUP_LIFE      0.8f   // 飘字显示几秒
#define MUSIC_VOLUME    0.35f  // ★ 背景音乐音量（0 ~ 1）
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
#define PLATE_COLOR  (Color){ 16, 46, 34, 255 }    // 钢板底色（★ 改成深绿色，紫色的蛇比较明显）
#define PLATE_LIGHT  (Color){ 30, 74, 56, 255 }    // 钢板左上亮边
#define PLATE_DARK   (Color){ 6, 22, 15, 255 }     // 钢板右下暗边
#define RIVET_COLOR  (Color){ 52, 104, 82, 255 }   // 螺丝钉
#define GOLD_COLOR   (Color){ 255, 200, 40, 255 }  // ★ 金苹果的光

// 地图格子里的东西
#define EMPTY    0
#define FOOD     1
#define OBSTACLE 2
#define BOOSTER  3
#define GOLDEN   4          // ★ 金苹果

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

// ★ 一颗粒子（吃到东西时炸开的小方块）
typedef struct {
    float x, y;          // 位置（像素）
    float vx, vy;        // 速度（每秒移动几像素）
    float life;          // 还剩几秒；0 = 没在用
    Color color;
} Particle;

// ★ 飘字（例如吃到苹果时往上飘的 "+1"）
typedef struct {
    float x, y;
    float life;          // 还剩几秒；0 = 没在用
    char text[8];
    Color color;
} Popup;

// ★ 所有音效
typedef struct {
    Sound eat;       // 吃到食物
    Sound booster;   // 吃到道具
    Sound crash;     // 撞到东西
    Sound levelUp;   // 升级
    Sound win;       // 赢了
    Sound golden;    // ★ 吃到金苹果
    Sound music;     // ★ 背景音乐（8 秒，一直循环）
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

    float goldenTimer;       // ★ 金苹果还剩几秒（0 = 地图上没有金苹果）
    float invincibleTimer;   // ★ 无敌还剩几秒
    float shakeTimer;        // ★ 画面震动还剩几秒
    float flashTimer;        // ★ 红色闪光还剩几秒
    Particle particles[MAX_PARTICLES];   // ★ 固定大小的数组，装在 Game 里（不是全局变量）
    Popup popups[MAX_POPUPS];
    int musicOn;             // ★ 1 = 播放背景音乐
} Game;

// ================= map.c：地图：建立/释放地图、在随机空格放东西、方向计算 =================
int **createMap(int width, int height);
void freeMap(int **map, int height);
int isOnSnake(Snake *s, int x, int y);
void neighborDir(int ax, int ay, int bx, int by, int *dx, int *dy);
void segmentDir(Game *g, int i, int j, int *dx, int *dy);
int isValidSpot(Game *g, int x, int y, int type);
void placeItem(Game *g, int type);

// ================= game.c：游戏规则：开始与结束、道具与生命、关卡、蛇每走一步要检查的事 =================
void showMessage(Game *g, const char *text);
void resetSnake(Game *g);
void resetGame(Game *g);
void startGame(Game *g);
void freeGame(Game *g);
void initGame(Game *g);
void applyBooster(Game *g);
void loseLife(Game *g);
void levelUp(Game *g);
float getMoveTime(Game *g);
void stepGame(Game *g);
void removeGolden(Game *g);
void checkLevelUp(Game *g);
void updatePlaying(Game *g);
int updateGame(Game *g);

// ================= input.c：键盘输入：开始画面的选项、W/A/S/D 控制蛇 =================
int editNumber(int value);
void updateMenu(Game *g);
void setDirection(Game *g, int dx, int dy);
void takeQueuedDirection(Snake *s);
void handleInput(Game *g);

// ================= assets.c：素材：用代码做出音效、读取和释放图片 =================
Sound makeSound(float startFreq, float endFreq, float duration);
void loadSounds(Sounds *s);
void unloadSounds(Sounds *s);
float noteFreq(int semitone);
Sound makeMusic(void);
Texture2D loadImage(const char *path);
void loadTextures(Textures *t);
void unloadTextures(Textures *t);

// ================= draw.c：画面：所有画图的函数，以及打开窗口 =================
void drawCenterText(const char *text, int y, int size, Color color);
void drawNeonText(const char *text, int y, int size, Color color);
float directionToAngle(int dx, int dy);
void drawSpriteSize(Game *g, Texture2D texture, float x, float y, float angle, int flip,
                    float w, float h, Color fallback);
void drawSprite(Game *g, Texture2D texture, float x, float y, float angle, int flip, float scale, Color fallback);
float cornerAngle(int ax, int ay, int bx, int by);
float smoothStep(float p);
float turnAngle(float a, float b, float p);
void drawBodyPiece(Game *g, int x, int y, int ax, int ay, int bx, int by, Color color);
void drawSliding(Game *g, Texture2D texture, int fromX, int fromY, int dx, int dy, float p,
                 float angle, float scale, Color color);
void drawSnake(Game *g);
void drawMetalPlate(Game *g, int x, int y);
void drawEnergyGrid(Game *g);
void drawMenuRows(Game *g, int firstY, int gap, int size);
const char *modeHint(Game *g);
void drawDashedRect(Rectangle r, float thick, Color color);
void drawMenu(Game *g);
void drawPlaying(Game *g);
void drawGame(Game *g, RenderTexture2D canvas);
void openWindow(void);
void drawGoldenApple(Game *g, int x, int y);

// ================= effects.c：特效：粒子、飘字、震动、闪光、背景音乐 =================
void spawnBurst(Game *g, int cellX, int cellY, Color color);
void addPopup(Game *g, int cellX, int cellY, const char *text, Color color);
void startCrashEffect(Game *g);
void updateEffects(Game *g, float dt);
void drawEffects(Game *g);
void updateMusic(Game *g);
void toggleMusic(Game *g);

#endif
