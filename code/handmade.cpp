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
game_update_and_render(GameOffscreenBuffer   *buffer,
                       int                    blue_offset,
                       int                    green_offset,
                       GameSoundOutputBuffer *sound_buffer,
                       int                    tone_hertz)
{
        //TODO: Allow sample offsets here for more robust platform options
        game_output_sound(sound_buffer, tone_hertz);
        render_weird_gradient(buffer, blue_offset, green_offset);
}