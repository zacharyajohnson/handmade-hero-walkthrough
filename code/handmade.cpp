#include "handmade.h"

internal void
game_output_sound(GameSoundOutputBuffer *sound_buffer,
                  int                    tone_hertz)
{
        local_persist real32 t_sine;
        int16 tone_volume = 3000;
        int wave_period = sound_buffer->samples_per_second/tone_hertz;

        int16 *sample_out = sound_buffer->samples;
        int sample_count = sound_buffer->sample_count;

        for (int sample_index = 0; sample_index < sample_count; sample_index++) {
                real32 sine_value = sinf(t_sine);
                int16 sample_value = (int16) (sine_value * tone_volume);

                *sample_out = sample_value;
                sample_out++;

                *sample_out = sample_value;
                sample_out++;
                t_sine += 2.0f * Pi32 * (real32)1.0 / (real32)wave_period;
        }
}

internal void
render_weird_gradient(GameOffscreenBuffer *buffer,
                      int                  blue_offset,
                      int                  green_offset)
{
        uint8 *row = (uint8 *)buffer->memory;

        for (int y = 0; y < buffer->height; y++) {

                uint32 *pixel = (uint32 *)row;
                for (int x = 0; x < buffer->width; x++) {
                        // LITTLE ENDIAN ARCHITECTURE
                        // Pixel in memory: BB GG RR xx
                        // The byte in  the lowest memory address becomes
                        // the lowest byte in the CPU.
                        // Pixel in CPU: xx RR GG BB
                        // Windows starts the blue channel
                        // first so when it gets swapped into the CPU,
                        // the RGB values are listed as expected since they
                        // started out on a little endian architecture.
                        uint8 blue = (x + blue_offset);
                        uint8 green = (y + green_offset);

                        *pixel = ((green << 8) | blue);
                        pixel++;

                }

                // We do this at the end even though we are
                // naturally iterating through the pixels
                // above because a bitmap can have padding
                // at the end. This effectivly means that
                // the pitch for a row can be greater than
                // the width of the image we are trying to draw.
                row += buffer->pitch;
        }
}

internal void
game_update_and_render(GameMemory            *memory,
                       GameOffscreenBuffer   *buffer,
                       GameSoundOutputBuffer *sound_buffer,
                       GameInput             *input)
{
        // Assert our game state can fit in our memory
        assert(sizeof(GameState) <= memory->permanent_storage_size);

        GameState *game_state = (GameState *) memory->permanent_storage;

        // If this is the first time through our render loop,
        // initalize to default values
        if(!memory->is_initialized) {
                game_state->blue_offset = 0;
                game_state->green_offset = 0;
                game_state->tone_hertz = 256;
                memory->is_initialized=true;
        }

        // Player 1
        GameControllerInput *input0 = &input->controllers[0];
        if(input0->is_analog) {
                // NOTE: Use analog movment tuning
                game_state->blue_offset += (int)4.0f * (input0->end_x);
                game_state->tone_hertz = 256 + (int)(128.0f * (input0->end_y));
        } else {
                // NOTE: Use digital movement tuning
        }

        if(input0->down.ended_down) {
                game_state->green_offset += 1;
        }


        //TODO: Allow sample offsets here for more robust platform options
        game_output_sound(sound_buffer,
                          game_state->tone_hertz);

        render_weird_gradient(buffer,
                              game_state->blue_offset,
                              game_state->green_offset);
}
