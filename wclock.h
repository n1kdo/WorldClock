void RegisterClockClass(HINSTANCE hInstance);

#define SHOW_SECONDS 

#define CLOCK_CLASS_NAME "ClockClass"
#define DIGIT_WIDTH  18
#define DIGIT_HEIGHT 34
#define COLON_WIDTH  5
#ifdef SHOW_SECONDS
#define CLOCK_DISPLAY_WIDTH  (DIGIT_WIDTH * 6 + COLON_WIDTH * 2 + 6)
#else
#define CLOCK_DISPLAY_WIDTH  (DIGIT_WIDTH * 4 + COLON_WIDTH + 6)
#endif
#define CLOCK_DISPLAY_HEIGHT (DIGIT_HEIGHT + 15)
#define CLOCK_Y_OFFSET 2

#define CLOCK_PARAMS_MSG (WM_USER + 1)

