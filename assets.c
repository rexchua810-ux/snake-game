// assets.c — assets: sound effects made in code, loading and freeing images
// AI disclosure: parts of this file were written with the help of AI tools (Claude / Claude Code).
//   The author reviewed the code, tested it in the game, and can explain how it works.
//   See "Use of Generative AI" in README.md for details.

#include <stdlib.h>
#include <math.h>     // powf、fabsf
#include "game.h"

// ================= Sound effects =================

// "calculate" a sound effect in code: the frequency slides from startFreq to endFreq over duration seconds
//   frequency = vibrations per second; higher = higher pitch
static Sound makeSound(float startFreq, float endFreq, float duration)
{
    int sampleRate = 44100;                      // 44100 samples per second (CD quality)
    int count = (int)(sampleRate * duration);    // total number of samples
    short *samples = malloc(count * sizeof(short));

    float phase = 0;                             // position inside one vibration (0 ~ 1)
    for (int i = 0; i < count; i++)
    {
        float progress = (float)i / count;       // progress 0 → 1
        float freq = startFreq + (endFreq - startFreq) * progress;
        phase += freq / sampleRate;
        if (phase >= 1)
            phase -= 1;

        float volume = 0.3f * (1 - progress);    // quieter towards the end (fade out)

        // square wave: push up for the first half of each vibration, pull down for the second half, sounds like a retro console
        if (phase < 0.5f)
            samples[i] = (short)(volume * 32767);
        else
            samples[i] = (short)(-volume * 32767);
    }

    Wave wave = { count, sampleRate, 16, 1, samples };  // samples, sample rate, 16-bit, mono, data
    Sound sound = LoadSoundFromWave(wave);              // Raylib makes its own copy
    free(samples);                                      // so our copy can be freed
    return sound;
}

// frequency of a note. semitone = semitones away from A (440 Hz); 12 semitones higher (one octave) doubles the frequency
static float noteFreq(int semitone)
{
    return 440.0f * powf(2.0f, semitone / 12.0f);
}

// "compose" in code: 8 seconds of looping retro electronic music
//   chords: Am → F → C → G (one bar each), the lead is a fast arpeggio, with a bass line below
static Sound makeMusic(void)
{
    int sampleRate = 44100;
    float stepTime = 0.125f;                    // each step (16th note) is 0.125 s = 120 beats per minute
    int steps = 64;                             // 4 bars × 16 steps = 8 seconds
    int stepSamples = (int)(sampleRate * stepTime);
    int count = steps * stepSamples;
    short *samples = malloc(count * sizeof(short));

    // root of each chord (semitones from A) and its third (3 = minor, 4 = major)
    int chordRoot[4]  = { 0, -4, 3, -2 };       // A、F、C、G
    int chordThird[4] = { 3, 4, 4, 4 };         // Am is minor, the others are major
    int arp[4] = { 0, 1, 2, 1 };                // arpeggio order: root, third, fifth, third

    float leadPhase = 0, bassPhase = 0;
    for (int step = 0; step < steps; step++)
    {
        int bar = step / 16;                    // which bar
        int root = chordRoot[bar];
        int notes[3] = { root, root + chordThird[bar], root + 7 };   // the three notes of the chord

        float lead = noteFreq(notes[arp[step % 4]]);
        if (step % 8 >= 4)
            lead *= 2;                          // one octave higher on the second half of the beat, for more variety
        float bass = noteFreq(root - 24);       // bass: two octaves lower

        for (int i = 0; i < stepSamples; i++)
        {
            float t = (float)i / stepSamples;   // progress through this note (0 → 1)

            leadPhase += lead / sampleRate;
            if (leadPhase >= 1)
                leadPhase -= 1;
            bassPhase += bass / sampleRate;
            if (bassPhase >= 1)
                bassPhase -= 1;

            // lead: narrow square wave (up only 25% of the time), each note fades quickly, sounds like "beep beep beep"
            float leadWave = -1;
            if (leadPhase < 0.25f)
                leadWave = 1;
            float leadVolume = 0.12f * (1 - t);

            // bass: triangle wave, rounder and fuller
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

// create all sound effects at once
void loadSounds(Sounds *s)
{
    s->eat     = makeSound(600, 900, 0.08f);   // short rising "beep"
    s->booster = makeSound(400, 1200, 0.25f);  // slide up
    s->crash   = makeSound(300, 60, 0.4f);     // drop down
    s->levelUp = makeSound(500, 1000, 0.35f);
    s->win     = makeSound(400, 1600, 0.8f);
    s->golden  = makeSound(800, 1600, 0.2f);   // golden apple: higher and brighter
    s->music   = makeMusic();                  // background music
    SetSoundVolume(s->music, MUSIC_VOLUME);
}

// free all sound effects
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

// ================= Images =================

// load one image; a missing file is fine, a colored block is drawn instead
static Texture2D loadImage(const char *path)
{
    Texture2D texture = { 0 };
    if (FileExists(path))
    {
        texture = LoadTexture(path);
        if (texture.width > 64)
        {
            // large images (e.g. 512x512) are shrunk a lot: build smaller versions (mipmaps) first to avoid jagged edges
            GenTextureMipmaps(&texture);
            SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        }
        else
        {
            SetTextureFilter(texture, TEXTURE_FILTER_POINT);   // keep small pixel art sharp when enlarged
        }
        // don't "repeat" the image edges: otherwise the left edge picks up pixels from the right and a thin line appears
        SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
    }
    return texture;
}

// load all images (all in the assets folder)
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

// free all images (id 0 = not loaded, nothing to free)
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
