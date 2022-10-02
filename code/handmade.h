#if !defined(HANDMADE_H)

#if HANDMADE_SLOW
#define assert(expression) if(!(expression)) { *(int *)NULL = 0; }
#else
#define assert(expression)
#endif

// Helper macros if we need to allocate
// certain memory sizes
#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)
#define terabytes(value) (gigabytes(value)*1024LL)

#define array_count(array) (sizeof(array) / sizeof((array)[0]))

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

struct GameButtonState {
        int half_transition_count;
        bool32 ended_down;
};

struct GameControllerInput {
        bool32 is_analog;

        real32 start_x;
        real32 start_y;

        real32 min_x;
        real32 min_y;

        real32 max_x;
        real32 max_y;

        real32 end_x;
        real32 end_y;
        union {
                GameButtonState buttons[6];
                struct {
                        GameButtonState up;
                        GameButtonState down;
                        GameButtonState left;
                        GameButtonState right;
                        GameButtonState left_shoulder;
                        GameButtonState right_shoulder;
                };
        };
};

struct GameInput {
        GameControllerInput controllers[4];
};

struct GameMemory {
        bool32 is_initialized;

        uint64 permanent_storage_size;
        void *permanent_storage;

        uint64 transient_storage_size;
        void *transient_storage;
};

struct GameState {
        int tone_hertz;
        int green_offset;
        int blue_offset;
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
game_update_and_render(GameMemory            *memory,
                       GameOffscreenBuffer   *buffer,
                       GameSoundOutputBuffer *sound_buffer,
                       GameInput             *input);
#define HANDMADE_H
#endif
