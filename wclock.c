/******************************************************************************/
/* WorldClock -- A Multiple-Timezone Digital Clock                            */
/*   wclock.c -- clock window management                                      */
/* Jeff Otterson  27 April 1995      version 1.00  original version           */
/* Jeff Otterson  18 October 1995    version 1.01  remove bwcc                */
/* Jeff Otterson  15 December 1995   version 1.02  speedup display of clocks  */
/******************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>
#include <time.h>
#include "worldclock.h"
#include "wclock.h"

typedef struct SegmentVectorsStructTag {
    UINT startX;
    UINT startY;
    UINT endX;
    UINT endY;
} SegmentVectorsStruct;

LRESULT WINAPI ClockWndProc (HWND, UINT, WPARAM, LPARAM);
void DrawDigit(HDC hdc, UINT x, UINT y, UINT digit, HPEN litPen, HPEN darkPen);
void DrawColon(HDC hdc, UINT x, UINT y, UINT onOff, HPEN litPen, HPEN darkPen);

void RegisterClockClass(HINSTANCE hInstance)
{
    WNDCLASS clockClass;
    if (!GetClassInfo(hInstance, CLOCK_CLASS_NAME, &clockClass))
    {
        clockClass.style = CS_HREDRAW | CS_VREDRAW  | CS_DBLCLKS;
        clockClass.lpfnWndProc = ClockWndProc;
        clockClass.cbClsExtra = 0;
        clockClass.cbWndExtra = sizeof(LPARAM);
        clockClass.hInstance = hInstance;
        clockClass.hIcon = NULL;
        clockClass.hCursor = LoadCursor (NULL, IDC_ARROW);
        clockClass.hbrBackground = GetStockObject (WHITE_BRUSH);
        clockClass.lpszMenuName = NULL;
        clockClass.lpszClassName = CLOCK_CLASS_NAME;
        RegisterClass (&clockClass);
    }
} /* RegisterClockClass */

/******************************************************************************/
/* ClockWndProc -- message handling for the clock window.                     */
/******************************************************************************/
LRESULT WINAPI ClockWndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    HPEN litPen, darkPen;
    PAINTSTRUCT ps;
    int oldMapMode;
    ClockInfoStruct *clockInfo;
    int hours;
    UINT tens, ones, xOffset;
    char *newName;
    short int newOffset;
    time_t now;
    struct tm gmtTime;
    int textWidth;
    POINT point;
    SIZE textSize;

    switch (message)
    {
        case WM_CREATE:
            clockInfo = (ClockInfoStruct *) wmalloc(sizeof(ClockInfoStruct));
            clockInfo->gmtOffset = 0;
            clockInfo->locationName = NULL;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) clockInfo);
            return(0);

        case CLOCK_PARAMS_MSG:
            clockInfo = (ClockInfoStruct *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            newName = (char *) lParam;
            newOffset = (short int) wParam;
            if (clockInfo->locationName != NULL)
                wfree(clockInfo->locationName);
            clockInfo->locationName = (char *) wmalloc(strlen(newName) +1);
            strcpy_s(clockInfo->locationName, CLOCK_NAME_SIZE, newName);
            clockInfo->gmtOffset = newOffset;
            return(0);

        case WM_PAINT:
            clockInfo = (ClockInfoStruct *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            now = time(NULL);
			gmtime_s(&gmtTime, &now);
            hdc = BeginPaint (hwnd, &ps);
            hours = gmtTime.tm_hour + clockInfo->gmtOffset;
            if (hours < 0)
                hours += 24;
            else
                if (hours > 23)
                    hours -= 24;

            oldMapMode = SetMapMode(hdc, MM_TEXT);
            xOffset = 3;
            tens = hours / 10;
            ones = hours - tens * 10;

            litPen  = CreatePen(PS_SOLID, 2, RGB(255,  0,  0));
            darkPen = CreatePen(PS_SOLID, 2, RGB(255,255,255));
            
            DrawDigit(hdc, xOffset, CLOCK_Y_OFFSET, tens, litPen, darkPen);
            xOffset += DIGIT_WIDTH;
            DrawDigit(hdc, xOffset, CLOCK_Y_OFFSET, ones, litPen, darkPen);
            xOffset += DIGIT_WIDTH;
            DrawColon(hdc, xOffset, CLOCK_Y_OFFSET, TRUE, litPen, darkPen);
            xOffset += COLON_WIDTH;
            tens = gmtTime.tm_min / 10;
            ones = gmtTime.tm_min - tens * 10;
            DrawDigit(hdc, xOffset, CLOCK_Y_OFFSET, tens, litPen, darkPen);
            xOffset += DIGIT_WIDTH;
            DrawDigit(hdc, xOffset, CLOCK_Y_OFFSET, ones, litPen, darkPen);
#ifdef SHOW_SECONDS
            xOffset += DIGIT_WIDTH;
            DrawColon(hdc, xOffset, CLOCK_Y_OFFSET, TRUE, litPen, darkPen);
            xOffset += COLON_WIDTH;
            tens = gmtTime.tm_sec / 10;
            ones = gmtTime.tm_sec - tens * 10;
            DrawDigit(hdc, xOffset, CLOCK_Y_OFFSET, tens, litPen, darkPen);
            xOffset += DIGIT_WIDTH;
            DrawDigit(hdc, xOffset, CLOCK_Y_OFFSET, ones, litPen, darkPen);
#endif
            DeleteObject(litPen);
            DeleteObject(darkPen);
            
            if (clockInfo->locationName != NULL)
            {
                GetTextExtentPoint32(hdc, clockInfo->locationName, (int) strlen(clockInfo->locationName), &textSize);
                textWidth = textSize.cx;
                TextOutA(hdc,
                        (int)(CLOCK_DISPLAY_WIDTH - textWidth) / 2,
                        DIGIT_HEIGHT - 2,
                        clockInfo->locationName, 
						(int) strlen(clockInfo->locationName));
            }
            SetMapMode(hdc, oldMapMode);
            EndPaint (hwnd, &ps);
            return(0);

        case WM_RBUTTONDOWN:
            point.x = LOWORD(lParam);
            point.y = HIWORD(lParam);
            ClientToScreen(hwnd, &point);
            TrackPopupMenu(popupMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                           point.x, point.y, 0, hwnd, NULL);
            return(0);

        case WM_COMMAND:
            return(SendMessage(GetParent(hwnd), WM_COMMAND, wParam, (LPARAM) hwnd));

        case WM_DESTROY: /* clean up data and close the window */
            clockInfo = (ClockInfoStruct *) (LONG_PTR) GetWindowLongPtr(hwnd, GWLP_USERDATA);
            wfree(clockInfo->locationName);
            wfree(clockInfo);
            return(0);

    } /* switch */
    return DefWindowProc (hwnd, message, wParam, lParam);
} /* ClockWndProc() */

void DrawDigit(HDC hdc, UINT x, UINT y, UINT digit, HPEN litPen, HPEN darkPen)
{
    const unsigned char digitBitmap[10] = { 0x3f,
                                            0x06,
                                            0x5b,
                                            0x4f,
                                            0x66,
                                            0x6d,
                                            0x7d,
                                            0x07,
                                            0x7f,
                                            0x6f };

    const SegmentVectorsStruct segmentVectors[7] = {{ 2, 0,10, 0},
                                                    {13, 2,13,12},
                                                    {13,16,13,26},
                                                    { 2,28,10,28},
                                                    { 0,16, 0,26},
                                                    { 0, 2, 0,12},
                                                    { 2,14,10,14}};
    int i;
    HPEN oldPen;

    if (digit > 9)
        return;
    oldPen = SelectObject(hdc, litPen);
    for (i = 0; i < 7; i++)
    {
        if (digitBitmap[digit] & (1 << i))
            SelectObject(hdc, litPen);
        else
            SelectObject(hdc, darkPen);
        MoveToEx(hdc, segmentVectors[i].startX + x, segmentVectors[i].startY + y, NULL);
        LineTo(hdc, segmentVectors[i].endX   + x, segmentVectors[i].endY   + y);
    } /* for i */
    SelectObject(hdc, oldPen);
} /* DrawDigit */

void DrawColon(HDC hdc, UINT x, UINT y, UINT onOff, HPEN litPen, HPEN darkPen)
{
    HPEN oldPen;
    oldPen = SelectObject(hdc, onOff ? litPen : darkPen);
    Rectangle(hdc, x, y + DIGIT_HEIGHT * 3 / 10, x + 2, y + DIGIT_HEIGHT * 3 / 10 + 2);
    Rectangle(hdc, x, y + DIGIT_HEIGHT * 6 / 10, x + 2, y + DIGIT_HEIGHT * 6 / 10 + 2);
    SelectObject(hdc, oldPen);
} /* DrawColon */
