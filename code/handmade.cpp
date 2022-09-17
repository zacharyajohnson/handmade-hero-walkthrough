#include "handmade.h"

internal void render_weird_gradient(GameOffscreenBuffer *buffer,
                int blue_offset, int green_offset) {

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

internal void game_update_and_render(GameOffscreenBuffer *buffer, int blue_offset, int green_offset) {
        render_weird_gradient(buffer, blue_offset, green_offset);
}
