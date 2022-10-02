#if !defined(WIN32_HANDMADE_H)

struct Win32SoundOutput {
        // NOTE: Sound test
        int samples_per_second;
        int bytes_per_sample;
        uint32 running_sample_index;
        int secondary_buffer_size;
        real32 t_sine;
        int latency_sample_count;
};

struct Win32OffscreenBuffer {
        // NOTE: Pixels are always 32-bits wide,
        // Memory Order BB GG RR xx
        BITMAPINFO info;
        void *memory;
        int width;
        int height;
        int pitch;
};

struct Win32WindowDimension {
        int width;
        int height;
};

#define WIN32_HANDMADE_H
#endif
