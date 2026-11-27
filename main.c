// main.c — program entry: initialize → game loop → clean up
// AI disclosure: parts of this file were written with the help of AI tools (Claude / Claude Code).
//   The author reviewed the code, tested it in the game, and can explain how it works.
//   See "Use of Generative AI" in README.md for details.

#include <stdlib.h>
#include <time.h>
#include "game.h"

int main(void)
{
    srand(time(NULL));

    Game game = {0};
    initGame(&game);

    openWindow();
    RenderTexture2D canvas = LoadRenderTexture(SCREEN_W, SCREEN_H);  // 600x660 canvas
    SetTextureFilter(canvas.texture, TEXTURE_FILTER_BILINEAR);       // smoother when scaled

    InitAudioDevice();                // open the audio device
    loadSounds(&game.sounds);         // create the sound effects
    loadTextures(&game.textures);     // load the images

    int quit = 0;
    while (!WindowShouldClose() && !quit)
    {
        quit = updateGame(&game);
        drawGame(&game, canvas);
    }

    unloadTextures(&game.textures);   // free the images
    unloadSounds(&game.sounds);       // free the sound effects
    CloseAudioDevice();               // close the audio device
    UnloadRenderTexture(canvas);      // free the canvas
    CloseWindow();
    freeGame(&game);
    return 0;
}
