#include <windows.h>

LRESULT CALLBACK main_window_callback(HWND window,
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
                                OutputDebugStringA("WM_SIZE\n");
                        } break;
                case WM_DESTROY:
                        {
                                OutputDebugStringA("WM_DESTROY\n");
                        } break;
                case WM_CLOSE:
                        {
                                OutputDebugStringA("WM_CLOSE\n");
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

                                PatBlt(device_context, x, y, width, height, WHITENESS);
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
        window_class.lpfnWndProc = main_window_callback;

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
                        // Windows does not send messages.
                        // We have to extract them from the message queue
                        // and send it to our windows procedure manually.
                        MSG message;
                        for(;;) {
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
