// assets.c — 素材：用代码做出音效、读取和释放图片

#include <stdlib.h>
#include <math.h>     // powf、fabsf
#include "game.h"

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
    s->golden  = makeSound(800, 1600, 0.2f);   // ★ 金苹果：比较高、比较亮
    s->music   = makeMusic();                  // ★ 背景音乐
    SetSoundVolume(s->music, MUSIC_VOLUME);
}

// ★ 释放所有音效
void unloadSounds(Sounds *s)
{
    UnloadSound(s->eat);
    UnloadSound(s->booster);
    UnloadSound(s->crash);
    UnloadSound(s->levelUp);
    UnloadSound(s->win);
    UnloadSound(s->golden);
    UnloadSound(s->music);
}

// ★ 音符的频率。semitone = 离 A（440Hz）几个半音；每高 12 个半音（一个八度）频率就乘 2
float noteFreq(int semitone)
{
    return 440.0f * powf(2.0f, semitone / 12.0f);
}

// ★ 用代码“作曲”：一段 8 秒、可以一直循环的复古电子音乐
//   和弦：Am → F → C → G（每个和弦一小节），主旋律是快速的琶音，下面配一条低音
Sound makeMusic(void)
{
    int sampleRate = 44100;
    float stepTime = 0.125f;                    // 每一格（16 分音符）0.125 秒 = 每分钟 120 拍
    int steps = 64;                             // 4 小节 × 16 格 = 8 秒
    int stepSamples = (int)(sampleRate * stepTime);
    int count = steps * stepSamples;
    short *samples = malloc(count * sizeof(short));

    // 四个和弦的根音（离 A 几个半音）和三度音（3 = 小调、4 = 大调）
    int chordRoot[4]  = { 0, -4, 3, -2 };       // A、F、C、G
    int chordThird[4] = { 3, 4, 4, 4 };         // Am 是小和弦，其他是大和弦
    int arp[4] = { 0, 1, 2, 1 };                // 琶音顺序：根音、三度、五度、三度

    float leadPhase = 0, bassPhase = 0;
    for (int step = 0; step < steps; step++)
    {
        int bar = step / 16;                    // 第几小节
        int root = chordRoot[bar];
        int notes[3] = { root, root + chordThird[bar], root + 7 };   // 和弦的三个音

        float lead = noteFreq(notes[arp[step % 4]]);
        if (step % 8 >= 4)
            lead *= 2;                          // 后半拍高一个八度，比较有变化
        float bass = noteFreq(root - 24);       // 低音：低两个八度

        for (int i = 0; i < stepSamples; i++)
        {
            float t = (float)i / stepSamples;   // 这个音符进行到哪里（0 → 1）

            leadPhase += lead / sampleRate;
            if (leadPhase >= 1)
                leadPhase -= 1;
            bassPhase += bass / sampleRate;
            if (bassPhase >= 1)
                bassPhase -= 1;

            // 主旋律：窄方波（只有 25% 的时间往上），每个音很快淡出，听起来像“哔哔哔”
            float leadWave = -1;
            if (leadPhase < 0.25f)
                leadWave = 1;
            float leadVolume = 0.12f * (1 - t);

            // 低音：三角波，声音比较圆、比较厚
            float bassWave = 4 * fabsf(bassPhase - 0.5f) - 1;
            float bassVolume = 0.18f;

            float value = leadWave * leadVolume + bassWave * bassVolume;
            samples[step * stepSamples + i] = (short)(value * 32767);
        }
    }

    Wave wave = { count, sampleRate, 16, 1, samples };
    Sound music = LoadSoundFromWave(wave);
    free(samples);
    return music;
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
