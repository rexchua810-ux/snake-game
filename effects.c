// effects.c — effects: particles and floating text when eating, shake and flash on crashes,
//   background music
// AI disclosure: written with help from AI tools (Claude / Claude Code); see README.md.

#include <stdio.h>    // snprintf
#include <stdlib.h>   // rand
#include <math.h>     // sinf、cosf
#include "game.h"

// ================= Particles =================

// burst a ring of particles at cell (cellX, cellY)
//   particles live in a fixed-size array: use a slot with life == 0 (unused); if all are used, skip
void spawnBurst(Game *g, int cellX, int cellY, Color color)
{
    float centerX = g->offsetX + (cellX + 0.5f) * g->cell;    // cell center (pixels)
    float centerY = g->offsetY + (cellY + 0.5f) * g->cell;

    int made = 0;
    for (int i = 0; i < MAX_PARTICLES && made < BURST_COUNT; i++)
    {
        Particle *p = &g->particles[i];
        if (p->life > 0)                   // slot in use, skip it
            continue;

        float angle = (rand() % 360) * 3.14159f / 180;   // random direction (converted to radians)
        // random speed: 60 ~ 180 pixels per second
        float speed = 60 + rand() % 120;
        p->x = centerX;
        p->y = centerY;
        p->vx = cosf(angle) * speed;
        p->vy = sinf(angle) * speed;
        p->life = PARTICLE_LIFE;
        p->color = color;
        made++;
    }
}

// float a short text up from cell (cellX, cellY) (e.g. "+1")
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
        return;                            // one is enough, stop here
    }
}

// crash: start the shake and the flash
void startCrashEffect(Game *g)
{
    g->shakeTimer = SHAKE_TIME;
    g->flashTimer = FLASH_TIME;
}

// update the effects every frame: particles move, floating text rises, timers count down
void updateEffects(Game *g, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &g->particles[i];
        if (p->life <= 0)
            continue;
        p->x += p->vx * dt;                // position += velocity × time
        p->y += p->vy * dt;
        p->vx *= 0.92f;                    // slow down gradually (like air resistance)
        p->vy *= 0.92f;
        p->life -= dt;
    }

    for (int i = 0; i < MAX_POPUPS; i++)
    {
        Popup *p = &g->popups[i];
        if (p->life <= 0)
            continue;
        p->y -= 40 * dt;                   // rises 40 pixels per second
        p->life -= dt;
    }

    if (g->shakeTimer > 0)
        g->shakeTimer -= dt;
    if (g->flashTimer > 0)
        g->flashTimer -= dt;
}

// draw particles and floating text: the less time left, the more transparent
void drawEffects(Game *g)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        Particle *p = &g->particles[i];
        if (p->life <= 0)
            continue;
        float alpha = p->life / PARTICLE_LIFE;            // 1 → 0
        float size = 3 + 3 * alpha;                       // smaller and smaller
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

    // red flash: cover the whole screen with red that slowly fades
    if (g->flashTimer > 0)
        DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(NEON_RED, 0.4f * g->flashTimer / FLASH_TIME));
}

// ================= Background music =================

// the music is an 8-second sound: when it ends, play it again, so it loops forever
void updateMusic(Game *g)
{
    if (g->musicOn && !IsSoundPlaying(g->sounds.music))
        PlaySound(g->sounds.music);
}

// N: music on / off
void toggleMusic(Game *g)
{
    g->musicOn = !g->musicOn;
    if (!g->musicOn)
        StopSound(g->sounds.music);
}
