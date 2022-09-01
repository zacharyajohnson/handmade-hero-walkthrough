#include <windows.h>

// Casey redefines static to use them for the specific
// use cases to make it more clear
// static(function variable) - persists between function calls
// static(top level in file) - makes the variable accessible only in that file
// static(function) - makes the function accessible only in that file
#define local_persist static
#define global_variable static
#define internal static

//TODO This is a global for now
global_variable bool running;

global_variable BITMAPINFO bitmap_info;
global_variable void *bitmap_memory;
global_variable HBITMAP bitmap_handle;
global_variable HDC bitmap_device_context;

// Everytime we resize the window,
// we need to reallocate our bitmap memory
// to match the new dimensions so it stays
// in sync as we write it to the window.
internal void win32_resize_DIB_section(int width, int height) {
        //TODO Bulletproof this.
        // Maybe don't free first, free after, then free first if that fails
        if (bitmap_handle != NULL) {
                DeleteObject(bitmap_handle);
        }

        if (bitmap_device_context == NULL) {
                bitmap_device_context = CreateCompatibleDC(0);
        }

        bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
        bitmap_info.bmiHeader.biWidth = width;
        bitmap_info.bmiHeader.biHeight = height;
        bitmap_info.bmiHeader.biPlanes = 1;
        bitmap_info.bmiHeader.biBitCount = 32;
        bitmap_info.bmiHeader.biCompression = BI_RGB;
        bitmap_info.bmiHeader.biSize = 0;
        bitmap_info.bmiHeader.biXPelsPerMeter = 0;
        bitmap_info.bmiHeader.biYPelsPerMeter = 0;
        bitmap_info.bmiHeader.biClrUsed = 0;
        bitmap_info.bmiHeader.biClrImportant = 0;

        bitmap_handle = CreateDIBSection(
                        bitmap_device_context, &bitmap_info,
                        DIB_RGB_COLORS,
                        &bitmap_memory,
                        0, 0);



}

internal void win32_update_window(HDC device_context, int x, int y,
                int width, int height)
{
        // Copies data from src(in this case, our bitmap memory,
        // to a destination, (in this case, its the window)

        StretchDIBits(device_context,
                        x, y, width, height,
                        x, y, width, height,
                        bitmap_memory,
                        &bitmap_info,
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
                case WM_SIZE:
                        {
                                // Get the drawable section
                                // of our window;
                                RECT client_rect;
                                GetClientRect(window, &client_rect);
                                int width = client_rect.right - client_rect.left;
                                int height = client_rect.bottom - client_rect.top;
                                win32_resize_DIB_section(width, height);
                                OutputDebugStringA("WM_SIZE\n");
                        } break;
                case WM_DESTROY:
                        {
                                //TODO Handle this as an error - recreate window?
                                running = false;
                        } break;
                case WM_CLOSE:
                        {
                                // TODO Handle this with a message to the user?
                                running = false;
                        } break;
                case WM_ACTIVATEAPP:
                        {
                                OutputDebugStringA("WM_ACTIVATEAPP\n");
                        } break;
                case WM_PAINT:
                        {
                                PAINTSTRUCT paint;
                                HDC device_context = BeginPaint(window, &paint);


                                int x = paint.rcPaint.left;
                                int y = paint.rcPaint.top;


                                int height = paint.rcPaint.bottom - paint.rcPaint.top;
                                int width = paint.rcPaint.right - paint.rcPaint.left;

                                win32_update_window(device_context, x, y, width, height);
                                EndPaint(window, &paint);
                        } break;
                default:
                        {
                                OutputDebugStringA("Default\n");
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

        WNDCLASS window_class = {0};

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
                HWND window_handle =
                        CreateWindowExA(0, window_class.lpszClassName, "Handmade Hero",
                                        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        CW_USEDEFAULT, CW_USEDEFAULT,
                                        0, 0, instance, 0);

                if(window_handle != NULL) {
                        running = true;

                        // Windows does not send messages.
                        // We have to extract them from the message queue
                        // and send it to our windows procedure manually.
                        MSG message;
                        while(running) {
                               BOOL message_result =
                                       GetMessageA(&message, NULL, 0, 0);
                               if (message_result > 0) {
                                       TranslateMessage(&message);
                                       DispatchMessageA(&message);
                               } else {
                                       break;
                               }

                        }
                } else {
                        // TODO logging
                }

        } else {
                //TODO Logging
        }



        return(0);
}
