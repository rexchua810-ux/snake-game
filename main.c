// main.c — 程序的起点：初始化 → 游戏循环 → 释放资源

#include <stdlib.h>
#include <time.h>
#include "game.h"

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
