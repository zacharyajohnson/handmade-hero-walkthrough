#if !defined(HANDMADE_H)

struct GameOffscreenBuffer {
        void *memory;
        int width;
        int height;
        int pitch;
};

struct GameSoundOutputBuffer {
        int samples_per_second;
        int sample_count;
        int16 *samples;
};

/*
 * TODO: Services the platform layer provides to the game
 */

/*
 * NOTE: Services that the game provides to the platform layer
 */

// Takes: controller/keyboard input, bitmap buffer to use, sound buffer to use
// and timing
internal void
game_update_and_render(GameOffscreenBuffer   *buffer,
                       int                    blue_offset,
                       int                    green_offset,
                       GameSoundOutputBuffer *sound_buffer,
                       int                    tone_hertz);
#define HANDMADE_H
#endif
