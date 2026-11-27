// effects.c — 特效：吃到东西的粒子和飘字、撞到时的震动和闪光、背景音乐

#include <stdio.h>    // snprintf
#include <stdlib.h>   // rand
#include <math.h>     // sinf、cosf
#include "game.h"

// ================= 粒子 =================

// ★ 在第 (cellX, cellY) 格炸开一圈粒子
//   粒子放在固定大小的数组里：找 life == 0（没在用）的位置来放，放满了就不放
void spawnBurst(Game *g, int cellX, int cellY, Color color)
{
    float centerX = g->offsetX + (cellX + 0.5f) * g->cell;    // 格子中心（像素）
    float centerY = g->offsetY + (cellY + 0.5f) * g->cell;

    int made = 0;
    for (int i = 0; i < MAX_PARTICLES && made < BURST_COUNT; i++)
    {
        Particle *p = &g->particles[i];
        if (p->life > 0)                   // 这个位置正在用，跳过
            continue;

        float angle = (rand() % 360) * 3.14159f / 180;   // 随机方向（换算成弧度）
        float speed = 60 + rand() % 120;                  // 随机速度：每秒 60 ~ 180 像素
        p->x = centerX;
        p->y = centerY;
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;
        p->life = PARTICLE_LIFE;
        p->color = color;
        made++;
    }
}

// ★ 在第 (cellX, cellY) 格飘出一段字（例如 "+1"）
void addPopup(Game *g, int cellX, int cellY, const char *text, Color color)
{
    for (int i = 0; i < MAX_POPUPS; i++)
    {
        Popup *p = &g->popups[i];
        if (p->life > 0)
            continue;
        p->x = g->offsetX + (cellX + 0.5f) * g->cell;
        p->y = g->offsetY + cellY * g->cell;
        p->life = POPUP_LIFE;
        snprintf(p->text, sizeof(p->text), "%s", text);
        p->color = color;
        return;                            // 放好一个就结束
    }
}

// ★ 撞到东西：开始震动和闪光
void startCrashEffect(Game *g)
{
    g->shakeTimer = SHAKE_TIME;
    g->flashTimer = FLASH_TIME;
}

// ★ 每一帧更新特效：粒子移动、飘字往上飘、各种倒数
void updateEffects(Game *g, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &g->particles[i];
        if (p->life <= 0)
            continue;
        p->x += p->vx * dt;                // 位置 += 速度 × 时间
        p->y += p->vy * dt;
        p->vx *= 0.92f;                    // 慢慢减速（像有空气阻力）
        p->vy *= 0.92f;
        p->life -= dt;
    }

    for (int i = 0; i < MAX_POPUPS; i++)
    {
        Popup *p = &g->popups[i];
        if (p->life <= 0)
            continue;
        p->y -= 40 * dt;                   // 每秒往上飘 40 像素
        p->life -= dt;
    }

    if (g->shakeTimer > 0)
        g->shakeTimer -= dt;
    if (g->flashTimer > 0)
        g->flashTimer -= dt;
}

// ★ 画出粒子和飘字：剩下的时间越少越透明
void drawEffects(Game *g)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &g->particles[i];
        if (p->life <= 0)
            continue;
        float alpha = p->life / PARTICLE_LIFE;            // 1 → 0
        float size = 3 + 3 * alpha;                       // 越来越小
        DrawRectangle(p->x - size / 2, p->y - size / 2, size, size, Fade(p->color, alpha));
    }

    for (int i = 0; i < MAX_POPUPS; i++)
    {
        Popup *p = &g->popups[i];
        if (p->life <= 0)
            continue;
        float alpha = p->life / POPUP_LIFE;
        int width = MeasureText(p->text, 22);
        DrawText(p->text, p->x - width / 2, p->y, 22, Fade(p->color, alpha));
    }

    // 红色闪光：整个画面盖一层红色，慢慢变透明
    if (g->flashTimer > 0)
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(NEON_RED, 0.4f * g->flashTimer / FLASH_TIME));
}

// ================= 背景音乐 =================

// ★ 音乐是一段 8 秒的音效：播完了就从头再播一次，就变成一直循环
void updateMusic(Game *g)
{
    if (g->musicOn && !IsSoundPlaying(g->sounds.music))
        PlaySound(g->sounds.music);
}

// ★ 按 N：音乐开 / 关
void toggleMusic(Game *g)
{
    g->musicOn = !g->musicOn;
    if (!g->musicOn)
        StopSound(g->sounds.music);
}
