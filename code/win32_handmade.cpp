/*
 * TODO: THIS IS NOT A FINAL PLATFORM LAYER
 * - Saved Game Locations
 * - Getting a handle to our own executable file
 * - Asset loading path
 * - Threading (launch a thread)
 * - Raw Input (support for multiple keyboards)
 * - Sleep/timeBeginPeriod
 * - ClipCursor() (for multimonitor support)
 * - Fullscreen support
 * - WM_SETCURSOR (control cursor visibility)
 * - QueryCancelAutoPlay
 * - WM_ACTIVEAPP( for when we are not the active application)
 * - Blit speed improvements(BitBlt)
 * - Hardware acceleration (OpenGL or Direct3D or BOTH??)
 * - GetKeyboardLayout (For French keyboards, international WASD support)
 *
 * Just a partial list of stuff
 */

// This must go first for typedefs below
// DO NOT MOVE
#include <stdint.h>
// Casey redefines static to use them for the specific
// use cases to make it more clear
// static(function variable) - persists between function calls
// static(top level in file) - makes the variable accessible only in that file
// static(function) - makes the function accessible only in that file
#define local_persist static
#define global_variable static
#define internal static

#define Pi32 3.14159265359f

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

// TODO Implement sine ourselves
// Handmade files need sign function so this goes first
#include <math.h>

// TODO For now these have to be here so the static
// defines are defined before the files. If we
// try to put these up top, the compiler complains
// This will be abstracted out better later
#include "handmade.h"
#include "handmade.cpp"

// Move windows headers down
// so we don't accidently call
// any of the functions in our
// platform independent code
#include <malloc.h>
#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

#include "win32_handmade.h"
// Macro used to generate our function signatures for the Xinput functions
// we want to dynamically pull in.
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)


// Typedef a function signature to a name
typedef X_INPUT_GET_STATE(x_input_get_state);
typedef X_INPUT_SET_STATE(x_input_set_state);

// Stub functions used if we can't
// dynamically load a very of X input
// so our app doesn't crash with a Null pointer.
X_INPUT_GET_STATE(XInputGetStateStub) {
        return ERROR_DEVICE_NOT_CONNECTED;
}

X_INPUT_SET_STATE(XInputSetStateStub) {
        return ERROR_DEVICE_NOT_CONNECTED;
}

// Then, assign them to variables as function pointers.
// Since XInput can have multiple versions across
// different versions of microsoft, we cannot statically
// link to them or the app will break on other machines
// So, we load them dynamically on startup.
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

// Create a mapping to our function pointers above.
// This is so people can't call the xinput functions
// in our code directly since we want to use
// our dynamically loaded functions.
#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_

// Same idea as Xinput
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);


//TODO This is a global for now
global_variable bool32 global_running;
global_variable Win32OffscreenBuffer global_back_buffer;
global_variable LPDIRECTSOUNDBUFFER global_secondary_sound_buffer;

void* platform_load_file(char *file_name) {
        // NOTE: Implements the Win32 file loading
        return 0;
}

// Goes through the sound buffer and sets everything to zero.
// Goes by bytes and not sample size(16 bit)
internal void
win32_clear_sound_buffer(Win32SoundOutput *sound_output)
{
        VOID *region1;
        DWORD region1_size;
        VOID *region2;
        DWORD region2_size;

        // Lock at the start of the sound buffer(first param),
        // lock the entire buffer(2nd param) and
        // write zero into the entire buffer
        if(SUCCEEDED(global_secondary_sound_buffer->Lock(
                        0,
                        sound_output->secondary_buffer_size,
                        &region1, &region1_size,
                        &region2, &region2_size, 0)))
        {
                uint8 *dest_sample = (uint8 *) region1;

                for (DWORD byte_index = 0; byte_index < region1_size; byte_index++) {
                        *dest_sample = 0;
                        dest_sample++;
                }

                dest_sample = (uint8 *) region2;
                for (DWORD byte_index = 0; byte_index < region2_size; byte_index++) {
                        *dest_sample = 0;
                        dest_sample++;
                }

                global_secondary_sound_buffer->Unlock(region1, region1_size, region2, region2_size);
        }
}

internal void
win32_fill_sound_buffer(Win32SoundOutput      *sound_output,
                        DWORD                  byte_to_lock,
                        DWORD                  bytes_to_write,
                        GameSoundOutputBuffer *sound_buffer)
{
        VOID *region1;
        DWORD region1_size;
        VOID *region2;
        DWORD region2_size;

        if(SUCCEEDED(global_secondary_sound_buffer->Lock(
                        byte_to_lock,
                        bytes_to_write,
                        &region1, &region1_size,
                        &region2, &region2_size, 0)))
        {

                // TODO: assert the region1/region2 size is valid
                int16 *dest_sample = (int16 *) region1;
                int16 *src_sample = sound_buffer->samples;

                // TODO: Collapse these two loops
                DWORD region1_sample_count = region1_size/sound_output->bytes_per_sample;
                for (DWORD sample_index = 0; sample_index < region1_sample_count; sample_index++) {
                        *dest_sample = *src_sample;
                        src_sample++;
                        dest_sample++;

                        *dest_sample = *src_sample;
                        src_sample++;
                        dest_sample++;
                        sound_output->running_sample_index++;
                }

                if (region2 != NULL) {
                        DWORD region2_sample_count = region2_size/sound_output->bytes_per_sample;
                        dest_sample = (int16 *) region2;

                        for (DWORD sample_index = 0; sample_index < region2_sample_count; sample_index++) {
                                *dest_sample = *src_sample;
                                src_sample++;
                                dest_sample++;

                                *dest_sample = *src_sample;
                                src_sample++;
                                dest_sample++;
                                sound_output->running_sample_index++;
                        }
                }

                global_secondary_sound_buffer->Unlock(region1, region1_size, region2, region2_size);
        }
}

internal void win32_load_x_input(void) {
        // Load the x input library
        // TODO Test this on Windows 8
        // Version 1.4 or xinput is the only version
        // avaliable on Windows 8, so we try and load that.
        HMODULE x_input_library = LoadLibraryA("xinput1_4.dll");

        // Try and get different versions of xinput
        // if 1.4 doesn't exist
        if (x_input_library == NULL) {
                x_input_library = LoadLibraryA("xinput9_1_0.dll");
        }

        if (x_input_library == NULL) {
                // 1_3 is very common on Windows 7
                x_input_library = LoadLibraryA("xinput1_3.dll");
        }

        if (x_input_library != NULL) {
                // If we can load the dll, but can't find the functions we
                // want, set the function pointers to the stub methods
                // so we don't get a null pointer.
                XInputGetState = (x_input_get_state *)GetProcAddress(x_input_library, "XInputGetState");

                if (XInputGetState == NULL) {
                        XInputGetState = XInputGetStateStub;
                }

                XInputSetState = (x_input_set_state *)GetProcAddress(x_input_library, "XInputSetState");
                if (XInputSetState == NULL) {
                        XInputSetState = XInputSetStateStub;
                }

                // TODO Diagnostic
        } else {
                // TODO: Diagnostic
        }
}

internal void win32_init_direct_sound(HWND window, int32 buffer_size, int32 samples_per_second) {
        // NOTE: Load the library
        HMODULE direct_sound_library = LoadLibraryA("dsound.dll");

        if (direct_sound_library != NULL) {

                direct_sound_create *DirectSoundCreate =
                        (direct_sound_create *)GetProcAddress(direct_sound_library, "DirectSoundCreate");

                // Object used when SoundCreate is successful
                // TODO: Double-check this works on XP - DirectSound 8 or 7???
                LPDIRECTSOUND direct_sound;

                // If we get a method back from the library and the
                // call was successful
                if (DirectSoundCreate != NULL && SUCCEEDED(DirectSoundCreate(NULL, &direct_sound, NULL))) {

                        WAVEFORMATEX wave_format = {};
                        wave_format.wFormatTag = WAVE_FORMAT_PCM;
                        wave_format.nChannels = 2;
                        wave_format.wBitsPerSample = 16;
                        wave_format.nBlockAlign = wave_format.nChannels * wave_format.wBitsPerSample / 8;
                        wave_format.nSamplesPerSec = samples_per_second;
                        wave_format.nAvgBytesPerSec = wave_format.nSamplesPerSec * wave_format.nBlockAlign;
                        wave_format.cbSize = 0;

                        // Allows us to set sound buffer format for primary buffer
                        if(SUCCEEDED(direct_sound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
                                DSBUFFERDESC buffer_desc = {};
                                buffer_desc.dwSize = sizeof(buffer_desc);
                                buffer_desc.dwFlags = DSBCAPS_PRIMARYBUFFER;
                                buffer_desc.dwBufferBytes = 0;

                                // NOTE: "Create" a primary buffer
                                // Holdover from original versions where
                                // you wrote directly into a buffer that
                                // represented the sound card.
                                // We only now need to set the format for the device.
                                LPDIRECTSOUNDBUFFER primary_buffer;

                                if(SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_desc, &primary_buffer, 0))) {

                                        if(SUCCEEDED(primary_buffer->SetFormat(&wave_format))) {

                                        } else {
                                                //TODO: Diagnostic
                                        }

                                } else {
                                       // TODO: Diagnostic
                                }

                        } else {
                                //TODO: Diagnostic
                        }

                        // NOTE: DBSCAPS_GETCURRENTPOSITION2
                        // "Create" a secondary buffer.
                        // This is the actual buffer we will write to.
                        DSBUFFERDESC buffer_desc = {};
                        buffer_desc.dwSize = sizeof(buffer_desc);
                        buffer_desc.dwBufferBytes = buffer_size;
                        buffer_desc.lpwfxFormat = &wave_format;

                        if(SUCCEEDED(direct_sound->CreateSoundBuffer(&buffer_desc, &global_secondary_sound_buffer, 0))) {

                        } else {
                               // TODO: Diagnostic
                        }

                } else {
                        // TODO: Diagnostic
                }

        } else {
                // TODO: Diagnostic
        }
}

internal Win32WindowDimension get_window_dimension(HWND window) {
        Win32WindowDimension result;

        // Get the drawable section
        // of our window;
        RECT client_rect;
        GetClientRect(window, &client_rect);
        result.width = client_rect.right - client_rect.left;
        result.height = client_rect.bottom - client_rect.top;

        return result;
}

// Everytime we resize the window,
// we need to reallocate our bitmap memory
// to match the new dimensions so it stays
// in sync as we write it to the window.

// DIB - Device Independent Bitmap
// Windows term for a bitmap that is not tied to
// a particular device
internal void win32_resize_DIB_section(Win32OffscreenBuffer *buffer,int width, int height) {

        if (buffer->memory != NULL) {
                VirtualFree(buffer->memory, 0, MEM_RELEASE);
        }

        int bytes_per_pixel = 4;

        buffer->width = width;
        buffer->height = height;
        buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
        buffer->info.bmiHeader.biWidth = buffer->width;

        // In BITMAPINFO - if biHeight is positive, it is a bottom-up
        // DIB. If it is negative, it is a top down DIB.
        // We want a top down DIB
        buffer->info.bmiHeader.biHeight = -buffer->height;
        buffer->info.bmiHeader.biPlanes = 1;

        // We only need 24 bits for our RGB
        // values, but on x86 architecture
        // 32 bit values are less costly
        // to read/write
        buffer->info.bmiHeader.biBitCount = 32;
        buffer->info.bmiHeader.biCompression = BI_RGB;
        buffer->info.bmiHeader.biSizeImage = 0;
        buffer->info.bmiHeader.biXPelsPerMeter = 0;
        buffer->info.bmiHeader.biYPelsPerMeter = 0;
        buffer->info.bmiHeader.biClrUsed = 0;
        buffer->info.bmiHeader.biClrImportant = 0;

        int bitmap_memory_size = buffer->width * buffer->height * bytes_per_pixel;
        buffer->pitch = buffer->width * bytes_per_pixel;

        buffer->memory = VirtualAlloc(NULL, bitmap_memory_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}


internal void win32_display_buffer_in_window(HDC device_context,
                int window_width, int window_height,
                Win32OffscreenBuffer *buffer)
{
        // Copies data from src(in this case, our bitmap memory,
        // to a destination, (in this case, its the window)

        // One thing we must consider for our bitmap is how it is represented
        // in memory. Since we are working with a 2D image and storing it
        // in a 1D array, we need to figure out how it is represented.
        // It can be top down(first row of pixels appears in memory first)
        // vs bottom up(last row of pixels appears in memory first)
        // In BITMAPINFO - if biHeight is positive, it is a bottom-up
        // DIB. If it is negative, it is a top down DIB.
        // We also need to figure out the stride/pitch.

        // Stride/Pitch - the number of bytes from one row of pixels in memory
        // to the next row of pixels in memory.
        //
        // This will also stretch the src dimensions(bitmap), to fit the destination
        // dimension(window)
        // TODO Aspect ratio correction
        // TODO Play with stretch modes
        StretchDIBits(device_context,
                        0, 0, window_width, window_height,
                        0, 0, buffer->width, buffer->height,
                        buffer->memory,
                        &buffer->info,
                        DIB_RGB_COLORS,
                        SRCCOPY);

}

internal void
win32_process_x_input_digital_button(GameButtonState *old_state,
                               GameButtonState       *new_state,
                               DWORD                  x_input_button_state,
                               DWORD                  button_bit)
{
        new_state->ended_down = ((x_input_button_state & button_bit) == button_bit);
        new_state->half_transition_count = (old_state->ended_down != new_state->ended_down) ? 1 : 0;
}

LRESULT CALLBACK win32_main_window_callback(HWND   window,
                                            UINT   message,
                                            WPARAM w_param,
                                            LPARAM l_param)
{
        // Return code for result of processing the message
        // Depends on the message being handled
        LRESULT result = 0;

        // Message can be one of the many fucking things
        // windows can pass to us to handle such as
        // changing window size and closing the window
        switch(message) {
                case WM_DESTROY:
                        {
                                //TODO Handle this as an error - recreate window?
                                global_running = false;
                        } break;
                case WM_CLOSE:
                        {
                                // TODO Handle this with a message to the user?
                                global_running = false;
                        } break;
                case WM_SYSKEYDOWN:
                case WM_SYSKEYUP:
                case WM_KEYDOWN:
                case WM_KEYUP:
                        {
                                uint32 vk_code = w_param;
                                bool32 was_down = ((l_param & (1 << 30)) != 0);
                                bool32 is_down = ((l_param & (1 << 31)) == 0);
                                if (was_down != is_down) {
                                        if (vk_code == 'W') {
                                        } else if (vk_code == 'A') {

                                        } else if (vk_code == 'S') {

                                        } else if (vk_code == 'D') {

                                        } else if (vk_code == 'Q') {

                                        } else if (vk_code == 'E') {

                                        } else if (vk_code == VK_UP) {

                                        } else if (vk_code == VK_LEFT) {

                                        } else if (vk_code == VK_DOWN) {

                                        } else if (vk_code == VK_RIGHT) {

                                        } else if (vk_code == VK_ESCAPE) {

                                        } else if (vk_code == VK_SPACE) {

                                        }
                                }

                                bool32 alt_key_was_down = ((l_param & (1 << 29)) != 0);
                                if ((vk_code == VK_F4) && alt_key_was_down) {
                                        global_running = false;
                                }

                        } break;
                case WM_ACTIVATEAPP:
                        {
                                OutputDebugStringA("WM_ACTIVATEAPP\n");
                        } break;
                case WM_PAINT:
                        {
                                PAINTSTRUCT paint;
                                HDC device_context = BeginPaint(window, &paint);

                                Win32WindowDimension window_dimension =
                                        get_window_dimension(window);

                                win32_display_buffer_in_window(device_context,
                                                window_dimension.width, window_dimension.height,
                                                &global_back_buffer);

                                EndPaint(window, &paint);
                        } break;
                default:
                        {
                                // We don't care about any other messages, so let windows
                                // default window callback handle them.
                                result = DefWindowProc(window, message, w_param, l_param);
                        } break;

        }

        return result;

}
// WinMain is the Windows specific entry point to a windows graphical application
int CALLBACK WinMain(
                HINSTANCE instance,
                HINSTANCE previous_instance,
                LPSTR command_line,
                int show_cmd
                )
{
        // The number of counts that happen per second
        // Doesn't change after startup
        LARGE_INTEGER perf_count_frequency_result;
        QueryPerformanceFrequency(&perf_count_frequency_result);
        int64 perf_count_frequency = perf_count_frequency_result.QuadPart;

        win32_load_x_input();

        WNDCLASSA window_class = {0};

        // Intialize our graphics buffer info once at startup
        win32_resize_DIB_section(&global_back_buffer,
                        1280, 720);

        // Redraw flags used to redraw the whole
        // screen when resizing instead of just
        // redrawing the part of the window that is new
        window_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        // The callback function Windows will call when
        // it wants us to take an action for it.
        window_class.lpfnWndProc = win32_main_window_callback;

        // Handle to the current instance of the application.
        // Used to identify the executable when it is loaded
        // into memory. Needed to do things like load icons or bitmaps
        window_class.hInstance = instance;
        //window_class.hIcon =;
        window_class.lpszClassName = "HandmadeHeroWindowClass";


        // Register the window class so it can be called in CreateWindow
        // by the className
        if(RegisterClassA(&window_class)) {
                HWND window =
                        CreateWindowExA(0, window_class.lpszClassName, "Handmade Hero",
                                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        0, 0, instance, 0);

                if(window != NULL) {

                        global_running = true;

                        // NOTE: Sound test
                        Win32SoundOutput sound_output = {};
                        sound_output.samples_per_second = 48000;
                        sound_output.bytes_per_sample = sizeof(int16)* 2;
                        sound_output.running_sample_index = 0;
                        sound_output.secondary_buffer_size = sound_output.samples_per_second * sound_output.bytes_per_sample;
                        sound_output.latency_sample_count = sound_output.samples_per_second / 15;

                        int16 *samples = (int16 *)VirtualAlloc(NULL, sound_output.secondary_buffer_size,MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                        // Load direct sound dlls and setup
                        // primary/secondary sound buffers
                        win32_init_direct_sound(window, sound_output.secondary_buffer_size, sound_output.samples_per_second);

                        win32_clear_sound_buffer(&sound_output);
                        global_secondary_sound_buffer->Play(0,0, DSBPLAY_LOOPING);

                        // NOTE: Since we specified CS_OWNDC, we can
                        // jsut get one device context and use it forever
                        // becuase we are not sharing it with anyone.
                        HDC device_context = GetDC(window);


                        // If we are compiling on our machine set our base address
                        // for our game memory to constant so its easier to debug.
                        // If a commerical version, set it to NULL so windows
                        // can set the the starting memory address to whatever
                        // it wants
#if HADMADE_INTERNAL
                        LPVOID base_address = (LPVOID)terabytes(2);
#else
                        LPVOID base_address = NULL;
#endif

                        GameMemory game_memory = {};
                        game_memory.permanent_storage_size = megabytes(64);
                        game_memory.transient_storage_size = gigabytes(4);

                        uint64 total_size = game_memory.permanent_storage_size + game_memory.transient_storage_size;

                        game_memory.permanent_storage = VirtualAlloc(base_address,
                                                                     total_size,
                                                                     MEM_RESERVE | MEM_COMMIT,
                                                                     PAGE_READWRITE);
                        game_memory.transient_storage = ((uint8 *) game_memory.permanent_storage
                                        + game_memory.permanent_storage_size);

                        if (samples != NULL
                                        && game_memory.permanent_storage != NULL
                                        && game_memory.transient_storage != NULL)
                        {
                                // Get start timestamp (microsecond percision)
                                LARGE_INTEGER last_counter;
                                QueryPerformanceCounter(&last_counter);

                                // The number of processor clock cycles done since the last reset
                                uint64 last_cycle_count = __rdtsc();

                                GameInput input[2] = {};

                                GameInput *new_input = &input[0];
                                GameInput *old_input = &input[1];

                                while(global_running) {
                                        // Windows does not send messages.
                                        // We have to extract them from the message queue
                                        // and send it to our windows procedure manually.
                                        MSG message;

                                        while(PeekMessage(&message, NULL, 0, 0, PM_REMOVE)) {
                                                // If windows decides to randomly kill our
                                                // process, quit
                                                if (message.message == WM_QUIT) {
                                                        global_running = false;
                                                }

                                                TranslateMessage(&message);
                                                DispatchMessage(&message);
                                        }

                                        // XUSER_MAX_COUNT - The number of xbox controllers
                                        // that can be used at once(Usually 4)
                                        // TODO Should we poll this more frequently?
                                        int max_controller_count = XUSER_MAX_COUNT;

                                        // If for some reason x input adds more controller input in
                                        // the future, set the number of controllers to what we have
                                        // in our game input struct so we aren't going out of bounds
                                        // in our array
                                        if (max_controller_count > array_count(new_input->controllers)) {
                                                max_controller_count = array_count(new_input->controllers);
                                        }

                                        for (DWORD controller_index = 0;
                                                        controller_index < max_controller_count;
                                                        controller_index++)
                                        {
                                                GameControllerInput *old_controller = &old_input->controllers[controller_index];
                                                GameControllerInput *new_controller = &new_input->controllers[controller_index];

                                                XINPUT_STATE controller_state;

                                                if (XInputGetState(controller_index, &controller_state)
                                                                == ERROR_SUCCESS)
                                                {
                                                        // If the controller is active get the state of it
                                                        XINPUT_GAMEPAD *gamepad = &controller_state.Gamepad;

                                                        //TODO: Dpad
                                                        bool32 up = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                                                        bool32 down = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                                                        bool32 left = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                                                        bool32 right = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;

                                                        new_controller->is_analog = true;
                                                        new_controller->start_x = old_controller->end_x;
                                                        new_controller->start_y = old_controller->end_y;

                                                        real32 x;
                                                        if (gamepad->sThumbLX < 0) {
                                                                x = (real32)gamepad->sThumbLX / 32768.0f;
                                                        } else {
                                                                x = (real32)gamepad->sThumbLX / 32767.0f;
                                                        }

                                                        new_controller->min_x = new_controller->max_x = new_controller->end_x = x;


                                                        real32 y;
                                                        if (gamepad->sThumbLY < 0) {
                                                                y = (real32)gamepad->sThumbLY / 32768.0f;
                                                        } else {
                                                                x = (real32)gamepad->sThumbLY / 32767.0f;
                                                        }

                                                        new_controller->min_y = new_controller->max_y = new_controller->end_y = y;

                                                        win32_process_x_input_digital_button(&old_controller->down,
                                                                                             &new_controller->down,
                                                                                             gamepad->wButtons,
                                                                                             XINPUT_GAMEPAD_A);

                                                        win32_process_x_input_digital_button(&old_controller->right,
                                                                                             &new_controller->right,
                                                                                             gamepad->wButtons,
                                                                                             XINPUT_GAMEPAD_B);

                                                        win32_process_x_input_digital_button(&old_controller->left,
                                                                                             &new_controller->left,
                                                                                             gamepad->wButtons,
                                                                                             XINPUT_GAMEPAD_X);

                                                        win32_process_x_input_digital_button(&old_controller->up,
                                                                                             &new_controller->up,
                                                                                             gamepad->wButtons,
                                                                                             XINPUT_GAMEPAD_Y);

                                                        win32_process_x_input_digital_button(&old_controller->left_shoulder,
                                                                                             &new_controller->left_shoulder,
                                                                                             gamepad->wButtons,
                                                                                             XINPUT_GAMEPAD_LEFT_SHOULDER);

                                                        win32_process_x_input_digital_button(&old_controller->right_shoulder,
                                                                                             &new_controller->right_shoulder,
                                                                                             gamepad->wButtons,
                                                                                             XINPUT_GAMEPAD_RIGHT_SHOULDER);
                                                       // bool32 start = gamepad->wButtons & XINPUT_GAMEPAD_START;
                                                       // bool32 back = gamepad->wButtons & XINPUT_GAMEPAD_BACK;
                                                } else {
                                                        // NOTE: the controller is not avaliable
                                                }
                                        }

                                        // Locks the sound buffer
                                        // and returns the regions of memory we can write
                                        // to. There are two regions just in case
                                        // the first region that is locked is not big
                                        // enough and hits the end of the sound memory.
                                        // If that is the case, we loop back to the
                                        // beginning of the sound buffer and that
                                        // region of memory is stored in the second
                                        // region

                                        // Our sound buffer is sterio, which
                                        // means dual channels. Each channel
                                        // takes a int16 for the sample
                                        // The buffer has
                                        // the format: LEFT RIGHT LEFT RIGHT LEFT RIGHT
                                        // where LEFT and RIGHT are a sample.
                                        // To make sense for sterio, we want to
                                        // handle one [LEFT RIGHT] group as a single
                                        // sample
                                        DWORD byte_to_lock = 0;
                                        DWORD target_cursor = 0;
                                        DWORD bytes_to_write = 0;
                                        DWORD write_cursor = 0;
                                        DWORD play_cursor = 0;
                                        bool32 sound_is_valid = false;
                                        if (SUCCEEDED(global_secondary_sound_buffer->GetCurrentPosition(&play_cursor, &write_cursor))) {
                                                byte_to_lock = (sound_output.running_sample_index * sound_output.bytes_per_sample) % sound_output.secondary_buffer_size;

                                                target_cursor = (play_cursor + (sound_output.latency_sample_count * sound_output.bytes_per_sample))
                                                        % sound_output.secondary_buffer_size;
                                                if (byte_to_lock > target_cursor) {
                                                        bytes_to_write = (sound_output.secondary_buffer_size - byte_to_lock);
                                                        bytes_to_write += target_cursor;
                                                } else {
                                                        bytes_to_write = (target_cursor - byte_to_lock);
                                                }

                                                sound_is_valid = true;

                                        }

                                        GameSoundOutputBuffer sound_buffer = {};
                                        sound_buffer.samples_per_second = sound_output.samples_per_second;

                                        sound_buffer.sample_count = bytes_to_write / sound_output.bytes_per_sample;
                                        sound_buffer.samples = samples;

                                        GameOffscreenBuffer buffer = {};
                                        buffer.memory = global_back_buffer.memory;
                                        buffer.width = global_back_buffer.width;
                                        buffer.height = global_back_buffer.height;
                                        buffer.pitch = global_back_buffer.pitch;

                                        game_update_and_render(&game_memory,
                                                               &buffer,
                                                               &sound_buffer,
                                                               new_input);

                                        // NOTE: DirectSound output test
                                        if (sound_is_valid) {
                                                win32_fill_sound_buffer(&sound_output, byte_to_lock, bytes_to_write, &sound_buffer);
                                        }

                                        Win32WindowDimension window_dimension =
                                                get_window_dimension(window);

                                        win32_display_buffer_in_window(device_context,
                                                        window_dimension.width, window_dimension.height,
                                                        &global_back_buffer);

                                        uint64 end_cycle_count = __rdtsc();

                                        // Log the end time and compute the total time elapsed,
                                        // FPS, and total CPU cycle count for the main loop
                                        LARGE_INTEGER end_counter;
                                        QueryPerformanceCounter(&end_counter);

                                        uint64 cycles_elapsed = end_cycle_count - last_cycle_count;
                                        int64 counter_elapsed = end_counter.QuadPart - last_counter.QuadPart;
                                        real64 milliseconds_per_frame = (real64)(((1000.0f * (real64)counter_elapsed) / (real64)perf_count_frequency));
                                        real64 frames_per_second = (real64)perf_count_frequency / (real64)counter_elapsed;
                                        // Our number of cycles be a lot per frame, so we divide to
                                        // get number of mega cycles
                                        real64 mega_cycles_per_frame = (((real64)cycles_elapsed) / ((real64)1000.0f * (real64)1000.0f));
#if 0
                                        char buffer[256];
                                        sprintf(buffer, "Milliseconds/frame: %fms/f, %f F/s, %f mc/f \n",
                                                        milliseconds_per_frame,
                                                        frames_per_second,
                                                        mega_cycles_per_frame);

                                        OutputDebugStringA(buffer);
#endif
                                        // Set last counter/last_cycle_count for the next frame comparison
                                        last_counter = end_counter;
                                        last_cycle_count = end_cycle_count;

                                        GameInput *temp = new_input;
                                        new_input = old_input;
                                        old_input = temp;
                                }
                        } else {
                                // TODO logging
                        }
                } else {
                        // TODO logging
                }

        } else {
                //TODO Logging
        }

        return(0);
}
