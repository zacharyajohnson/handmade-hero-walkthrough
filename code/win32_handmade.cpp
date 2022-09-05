#include <windows.h>
#include <stdint.h>
#include <xinput.h>

// Casey redefines static to use them for the specific
// use cases to make it more clear
// static(function variable) - persists between function calls
// static(top level in file) - makes the variable accessible only in that file
// static(function) - makes the function accessible only in that file
#define local_persist static
#define global_variable static
#define internal static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

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
        return 0;
}

X_INPUT_SET_STATE(XInputSetStateStub) {
        return 0;
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

//TODO This is a global for now
global_variable bool global_running;
global_variable Win32OffscreenBuffer global_back_buffer;

internal void win32_load_x_input(void) {
        // Load the x input library
        // 1_3 is very common in machines
        HMODULE x_input_library = LoadLibraryA("xinput1_3.dll");

        if (x_input_library != NULL) {
                XInputGetState = (x_input_get_state *)GetProcAddress(x_input_library, "XInputGetState");
                XInputSetState = (x_input_set_state *)GetProcAddress(x_input_library, "XInputSetState");
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

internal void render_weird_gradient(Win32OffscreenBuffer *buffer,
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

        buffer->memory = VirtualAlloc(NULL, bitmap_memory_size, MEM_COMMIT, PAGE_READWRITE);
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

LRESULT CALLBACK win32_main_window_callback(HWND window,
                UINT message,
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
                                bool was_down = ((l_param & (1 << 30)) != 0);
                                bool is_down = ((l_param & (1 << 31)) == 0);
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
        win32_load_x_input();

        WNDCLASSA window_class = {0};

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
                        // NOTE: Since we specified CS_OWNDC, we can
                        // jsut get one device context and use it forever
                        // becuase we are not sharing it with anyone.
                        HDC device_context = GetDC(window);

                        global_running = true;

                        int blue_offset = 0;
                        int green_offset = 0;

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
                                for (DWORD controller_index = 0;
                                                controller_index < XUSER_MAX_COUNT;
                                                controller_index++)
                                {
                                        XINPUT_STATE controller_state;

                                        if (XInputGetState(controller_index, &controller_state)
                                                        == ERROR_SUCCESS)
                                        {
                                                // If the controller is active get the state of it
                                                XINPUT_GAMEPAD *gamepad = &controller_state.Gamepad;

                                                bool up = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
                                                bool down = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
                                                bool left = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
                                                bool right = gamepad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
                                                bool start = gamepad->wButtons & XINPUT_GAMEPAD_START;
                                                bool back = gamepad->wButtons & XINPUT_GAMEPAD_BACK;
                                                bool left_shoulder = gamepad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
                                                bool right_shoulder = gamepad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
                                                bool a_button = gamepad->wButtons & XINPUT_GAMEPAD_A;
                                                bool b_button = gamepad->wButtons & XINPUT_GAMEPAD_B;
                                                bool x_button = gamepad->wButtons & XINPUT_GAMEPAD_X;
                                                bool y_button = gamepad->wButtons & XINPUT_GAMEPAD_Y;

                                                int16 left_stick_x = gamepad->sThumbLX;
                                                int16 left_stick_y = gamepad->sThumbLY;

                                                blue_offset += left_stick_x >> 12;
                                                green_offset += left_stick_y >> 12;

                                        } else {
                                                // NOTE: the controller is not avaliable
                                        }
                                }

                                render_weird_gradient(&global_back_buffer,
                                                blue_offset, green_offset);

                                Win32WindowDimension window_dimension =
                                        get_window_dimension(window);

                                win32_display_buffer_in_window(device_context,
                                                window_dimension.width, window_dimension.height,
                                                &global_back_buffer);

                        }
                } else {
                        // TODO logging
                }

        } else {
                //TODO Logging
        }



        return(0);
}
