/******************************************************************************/
/* WorldClock -- A Multiple-Timezone Digital Clock                            */
/* wrldclck.c -- multiple clock manager and mainline control                  */
/* Jeff Otterson  27 April 1995      version 1.00  original version           */
/* Jeff Otterson  18 October 1995    version 1.01  remove bwcc                */
/* Jeff Otterson  15 December 1995   version 1.02  fix add clock cancel bug   */
/* Jeff Otterson  31 July 2016       version 1.12  seconds                    */
/* Jeff Otterson  15 December 2023   version 1.13  always clear on invalidate */
/******************************************************************************/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <time.h>
#include <stdio.h>
#include "worldclock.h"
#include "wclock.h"

#define TIMER_ID 101
#define TIMER_ID 101
#define INI_FILE_NAME "./WorldClock.ini"

ClockInfoListStruct *clockInfoList = NULL;
static HINSTANCE hInstance;
static int numClocks = 0;
HMENU popupMenu;
HMENU positionsMenu;
HWND AddClock(HWND parentWindow, int layout, char *data, int gmtOffset);
void AdjustWindow(HWND hwnd, int layout);
int  ModifyClock(HWND clockWindow);
void DeleteClock(HWND parentWindow, int layout, HWND clockWindow);
void DeleteClockListEntry(HWND parentWindow, int layout, ClockInfoListStruct *deleteEntry);

LRESULT WINAPI ModifyDialogProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI AboutBoxDialogProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT WndProc (HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int nCmdShow)
{
    static char szAppName [] = "WorldClock" ;
    HWND        hwndTop ;
    MSG         msg;
    WNDCLASS    wndclass;

    hInstance = hInst;
    _tzset();
    if (!hPrevInstance)
    {
        wndclass.style          = CS_HREDRAW | CS_VREDRAW;
        wndclass.lpfnWndProc    = (WNDPROC) WndProc ;
        wndclass.cbClsExtra     = 0;
        wndclass.cbWndExtra     = 0;
        wndclass.hInstance      = hInstance ;
        wndclass.hIcon          = LoadIcon (hInstance, szAppName) ;
        wndclass.hCursor        = LoadCursor (NULL, IDC_ARROW) ;
        wndclass.hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
        wndclass.lpszMenuName   = NULL ;
        wndclass.lpszClassName  = szAppName ;
        RegisterClass (&wndclass);
    }
    RegisterClockClass(hInstance);

    positionsMenu = CreatePopupMenu();
    AppendMenu(positionsMenu, MF_ENABLED | MF_STRING, WC_POS_UL,   "Upper Left");
    AppendMenu(positionsMenu, MF_ENABLED | MF_STRING, WC_POS_UR,   "Upper Right");
    AppendMenu(positionsMenu, MF_ENABLED | MF_STRING, WC_POS_LL,   "Lower Left");
    AppendMenu(positionsMenu, MF_ENABLED | MF_STRING, WC_POS_LR,   "Lower Right");
    AppendMenu(positionsMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(positionsMenu, MF_ENABLED | MF_STRING, WC_OR_HORZ,  "Horizontal");
    AppendMenu(positionsMenu, MF_ENABLED | MF_STRING, WC_OR_VERT,  "Vertical");

    popupMenu = CreatePopupMenu();
    AppendMenu(popupMenu, MF_ENABLED | MF_STRING, WC_ADD,        "Add a New Clock");
    AppendMenu(popupMenu, MF_ENABLED | MF_STRING, WC_MODIFY,     "Modify this Clock");
    AppendMenu(popupMenu, MF_ENABLED | MF_STRING, WC_DELETE,     "Delete this Clock");
    AppendMenu(popupMenu, MF_ENABLED | MF_STRING | MF_UNCHECKED, WC_ONTOP,  "Clocks Stay on Top");
    AppendMenu(popupMenu, MF_ENABLED | MF_POPUP, (UINT_PTR) positionsMenu, "Relocate Clocks");
    AppendMenu(popupMenu, MF_ENABLED | MF_STRING, WC_SAVEDATA,   "Save Setup");
    AppendMenu(popupMenu, MF_ENABLED | MF_STRING, WC_ABOUT,      "About World Clock");
    AppendMenu(popupMenu, MF_ENABLED | MF_STRING, WC_EXIT,       "Exit World Clock");

    hwndTop = CreateWindowEx (WS_EX_TOOLWINDOW,
							  szAppName,
							  "World Clock",
                              WS_POPUP,
                              0,
                              0,
                              CLOCK_DISPLAY_WIDTH + 4,
                              100,
                              NULL, NULL, hInstance, NULL);

    ShowWindow (hwndTop, nCmdShow);
    UpdateWindow (hwndTop);

    while (GetMessage (&msg, NULL, 0, 0))
    {
        TranslateMessage (&msg) ;
        DispatchMessage (&msg) ;
    }
    return (int) msg.wParam ;
} /* WinMain() */

LRESULT WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i, gmtOffset;
    char data[CLOCK_NAME_SIZE], name[CLOCK_NAME_SIZE];
    ClockInfoListStruct *clockInfoListPtr, *clockInfoListDeletePtr;
    ClockInfoStruct *clockInfo;
    HWND clockWindow;
    static unsigned char layout;
    DLGPROC aboutBoxDialogProc;

    switch (message)
    {
        case WM_CREATE:
            layout = (unsigned char) GetPrivateProfileInt("WindowData", "Layout", 1, INI_FILE_NAME);
            numClocks = GetPrivateProfileInt("ClockData", "NumClocks", 0, INI_FILE_NAME);

            if (numClocks == 0)
            {
                AddClock(hwnd, layout, "GMT", 0);
                numClocks = 1;
            } /* if numClocks == 0 */
            else
            {
                for (i = 1; i <= numClocks; i++)
                {
                    sprintf_s(name, CLOCK_NAME_SIZE, "Clock%dName", i);
                    GetPrivateProfileString("ClockData", name, "", data, CLOCK_NAME_SIZE, INI_FILE_NAME);
                    if (strlen(data) == 0)
                        break;
                    sprintf_s(name, CLOCK_NAME_SIZE, "Clock%dOffset", i);
                    gmtOffset = GetPrivateProfileInt("ClockData", name, 24, INI_FILE_NAME);
                    AddClock(hwnd, layout, data, gmtOffset);
                } /* for i */
            } /* if numClocks == 0 */

            AdjustWindow(hwnd, layout);

#ifdef SHOW_SECONDS
            if (SetTimer(hwnd, TIMER_ID, 1000, NULL) == 0)
#else
            if (SetTimer(hwnd, TIMER_ID, 30000, NULL) == 0)
#endif
            {
                MessageBox(hwnd, "Could not allocate timer!", "Startup Failure", MB_OK | MB_ICONSTOP);
                PostMessage(hwnd,WM_CLOSE, 0, 0L);
            }

            break;

        case WM_TIMER:
            clockInfoListPtr = clockInfoList;
            while (clockInfoListPtr != NULL)
            {
                InvalidateRect(clockInfoListPtr->hwnd, NULL, TRUE);
                clockInfoListPtr = clockInfoListPtr->next;
            } /* while clockInfoListPtr != NULL */
            break;

        case WM_COMMAND:
            switch (wParam)
            {
                case WC_ADD:
                    numClocks++;
                    clockWindow = AddClock(hwnd, layout, "GMT-Zero", 0);
                    if (!ModifyClock(clockWindow))
                        DeleteClock(hwnd, layout, clockWindow);
                    else
                        AdjustWindow(hwnd, layout);
                    break;

                case WC_MODIFY:
                    clockWindow = (HWND) lParam;
                    ModifyClock(clockWindow);
                    break;

                case WC_DELETE:
                    clockWindow = (HWND) lParam;
                    DeleteClock(hwnd, layout, clockWindow);
                    break;

                case WC_POS_UL:
                    layout &= ~POS_RIGHT;
                    layout &= ~POS_BOTTOM;
                    AdjustWindow(hwnd, layout);
                    break;

                case WC_POS_UR:
                    layout |= POS_RIGHT;
                    layout &= ~POS_BOTTOM;
                    AdjustWindow(hwnd, layout);
                    break;

                case WC_POS_LR:
                    layout |= POS_RIGHT;
                    layout |= POS_BOTTOM;
                    AdjustWindow(hwnd, layout);
                    break;

                case WC_POS_LL:
                    layout &= ~POS_RIGHT;
                    layout |= POS_BOTTOM;
                    AdjustWindow(hwnd, layout);
                    break;

                case WC_OR_HORZ:
                    layout &= ~OR_VERT;
                    AdjustWindow(hwnd, layout);
                    break;

                case WC_OR_VERT:
                    layout |= OR_VERT;
                    AdjustWindow(hwnd, layout);
                    break;

                case WC_ONTOP:
                    if (layout & ON_TOP)
                        layout &= ~ON_TOP;
                    else
                        layout |= ON_TOP;
                    AdjustWindow(hwnd, layout);
                    break;

                case WC_SAVEDATA:
                    sprintf_s(data, CLOCK_NAME_SIZE, "%d",layout);
                    WritePrivateProfileString("WindowData", "Layout",  data, INI_FILE_NAME);

                    sprintf_s(data, CLOCK_NAME_SIZE,"%d", numClocks);
                    WritePrivateProfileString("ClockData", "NumClocks",  data, INI_FILE_NAME);

                    i = 1;
                    clockInfoListPtr = clockInfoList;
                    while (clockInfoListPtr != NULL)
                    {
                        clockInfo = (ClockInfoStruct *) (LONG_PTR) GetWindowLongPtr(clockInfoListPtr->hwnd, GWLP_USERDATA);
                        sprintf_s(name, CLOCK_NAME_SIZE, "Clock%dName",i);
                        WritePrivateProfileString("ClockData", name,  clockInfo->locationName, INI_FILE_NAME);
                        sprintf_s(name, CLOCK_NAME_SIZE, "Clock%dOffset",i);
                        sprintf_s(data, CLOCK_NAME_SIZE, "%d",clockInfo->gmtOffset);
                        WritePrivateProfileString("ClockData", name,  data, INI_FILE_NAME);
                        clockInfoListPtr = clockInfoListPtr->next;
                        i++;
                    } /* while clockInfoListPtr != NULL */
                    break;

                case WC_ABOUT:
                    aboutBoxDialogProc = (DLGPROC) MakeProcInstance((FARPROC) AboutBoxDialogProc, hInstance);
                    DialogBox(hInstance, "AboutBox", hwnd, aboutBoxDialogProc);
                    FreeProcInstance(aboutBoxDialogProc);
                    break;

                case WC_EXIT:
                    PostMessage (hwnd, WM_CLOSE, 0, 0L);
                    break;

            } /* switch wParam */
            return(0);

        case WM_CLOSE:
            KillTimer(hwnd, TIMER_ID);

            clockInfoListPtr = clockInfoList;
            while (clockInfoListPtr != NULL)
            {
                clockInfoListDeletePtr = clockInfoListPtr;
                clockInfoListPtr = clockInfoListPtr->next;
                wfree(clockInfoListDeletePtr);
            } /* while clockInfoListPtr != NULL */

            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0 ;
    }
    return DefWindowProc(hwnd, message, wParam, lParam) ;
} /* WndProc() */

HWND AddClock(HWND parentWindow, int layout, char *name, int gmtOffset)
{
    ClockInfoListStruct *clockInfoListPtr = clockInfoList;
    ClockInfoListStruct *newClockInfoListNode = wmalloc(sizeof(ClockInfoListStruct));

    if (clockInfoList == NULL)
    {
        clockInfoListPtr = newClockInfoListNode;
        clockInfoList = clockInfoListPtr;
    } /* if clockInfoList == NULL */
    else
    {
        while (clockInfoListPtr->next != NULL)
            clockInfoListPtr = clockInfoListPtr->next;
        clockInfoListPtr->next = newClockInfoListNode;
        clockInfoListPtr = clockInfoListPtr->next;
    } /* if clockInfoList == NULL */
    /* list entry is created, populate it */
    if (layout)
    {
        clockInfoListPtr->hwnd = CreateWindow(CLOCK_CLASS_NAME,
                                              name,
                                              WS_CHILD | WS_VISIBLE | WS_BORDER,
                                              0,
                                              (numClocks-1) * CLOCK_DISPLAY_HEIGHT,
                                              CLOCK_DISPLAY_WIDTH,
                                              CLOCK_DISPLAY_HEIGHT,
                                              parentWindow,
                                              NULL,
                                              hInstance,
                                              NULL);
    }
    else
    {
        clockInfoListPtr->hwnd = CreateWindow(CLOCK_CLASS_NAME,
                                              name,
                                              WS_CHILD | WS_VISIBLE | WS_BORDER,
                                              (numClocks-1) * CLOCK_DISPLAY_WIDTH,
                                              0,
                                              CLOCK_DISPLAY_WIDTH,
                                              CLOCK_DISPLAY_HEIGHT,
                                              parentWindow,
                                              NULL,
                                              hInstance,
                                              NULL);
    }
    SendMessage(clockInfoListPtr->hwnd, CLOCK_PARAMS_MSG,
                (WPARAM) gmtOffset,
                (LPARAM) name);
    return(clockInfoListPtr->hwnd);
} /* AddClock */

int ModifyClock(HWND clockWindow)
{
    ClockInfoStruct *clockInfo;
    DLGPROC modifyDialogProc;
    INT_PTR dbx;

    clockInfo = (ClockInfoStruct *) (LONG_PTR) GetWindowLongPtr(clockWindow, GWLP_USERDATA);

    modifyDialogProc = (DLGPROC) MakeProcInstance((FARPROC) ModifyDialogProc, hInstance);
    dbx = DialogBoxParamA(hInstance, "SetupDialog", clockWindow, modifyDialogProc, (LPARAM) clockInfo);
    FreeProcInstance((FARPROC) modifyDialogProc);
    InvalidateRect(clockWindow, NULL, TRUE);
    return (int) dbx;
} /* ModifyClock() */

LRESULT WINAPI ModifyDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static ClockInfoStruct *clockInfo;
    static short gmtOffset;
	int nScrollCode;
    HWND tempControl;
    char tempText[CLOCK_NAME_SIZE + 1];
    LPSTR tempTextPtr;
    int pos, min, max;

    switch (message)
    {
        case WM_INITDIALOG:
            clockInfo = (ClockInfoStruct *) lParam;
            SetWindowText(GetDlgItem(hDlg, TIMEZONE_NAME), clockInfo->locationName);
            gmtOffset = clockInfo->gmtOffset;
            tempControl = GetDlgItem(hDlg, GMT_OFFSET_SLIDER);
            SetScrollRange(tempControl, SB_CTL, -23, 23, FALSE);
            SetScrollPos(tempControl, SB_CTL, gmtOffset, TRUE);
            sprintf_s(tempText, CLOCK_NAME_SIZE, "%d", gmtOffset);
            SetWindowText(GetDlgItem(hDlg, GMT_OFFSET_TEXT), tempText);
            return(TRUE);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    tempTextPtr = tempText;
                    tempControl = GetDlgItem(hDlg, TIMEZONE_NAME);
                    GetWindowText(tempControl, tempTextPtr, CLOCK_NAME_SIZE);
                    wfree(clockInfo->locationName);
                    clockInfo->locationName = (char *) wmalloc(sizeof(tempText)+1);
                    strcpy_s(clockInfo->locationName, CLOCK_NAME_SIZE, tempText);
                    clockInfo->gmtOffset = gmtOffset;
                    EndDialog(hDlg, TRUE);
                    return(TRUE);

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return(TRUE);
            } /* switch wParam */
			return(TRUE);

        case WM_HSCROLL:
            tempControl = (HWND) lParam;
            GetScrollRange(tempControl, SB_CTL, &min, &max);
            pos = GetScrollPos (tempControl, SB_CTL);
			nScrollCode = (int) LOWORD(wParam);  // scroll bar value 
            switch (nScrollCode)
            {
                case SB_PAGEDOWN:
                    pos = pos + (max - min) / 10;
                    break;

                case SB_LINEDOWN:
                    pos ++;
                    break;

                case SB_PAGEUP:
                    pos = pos - (max - min) / 10;
                    break;

                case SB_LINEUP:
                    pos --;
                    break;

                case SB_TOP:
                    pos = min;
                    break;

                case SB_BOTTOM:
                    pos = max;
                    break;

                case SB_THUMBTRACK:
                case SB_THUMBPOSITION:
                    pos = LOWORD(lParam);
                    break;

                default:
                    break;
            } /* switch wParam */

            if (pos > max)
                pos = max;

            if (pos < min)
                pos = min;

            SetScrollPos(tempControl, SB_CTL, pos, TRUE);

            if (tempControl == GetDlgItem(hDlg, GMT_OFFSET_SLIDER))
            { /* offset slider */
                gmtOffset = (short) pos;
                sprintf_s(tempText, CLOCK_NAME_SIZE, "%d", gmtOffset);
                SetWindowText(GetDlgItem(hDlg, GMT_OFFSET_TEXT), tempText);
            } /* if control == .. CUSTOMIZE_TEXT_BUFFER_SLIDER */
            return(TRUE);

    } /* switch message */
    return(FALSE);
} /* ModifyDialogProc() */

LRESULT WINAPI AboutBoxDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_COMMAND:
            switch (wParam)
            {
                case IDOK:
                    EndDialog(hDlg, FALSE);
                    return(TRUE);
                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return(TRUE);
            } /* switch wParam */
    } /* switch message */
    return(FALSE);
} /* AboutBoxDialogProc() */

void DeleteClock(HWND parentWindow, int layout, HWND clockWindow)
{
    ClockInfoListStruct *clockInfoListPtr = clockInfoList;
    ClockInfoListStruct *lastNode = NULL;

    while (clockInfoListPtr != NULL)
    {
        if (clockInfoListPtr->hwnd == clockWindow)
        { /* found entry to delete */
            if (clockInfoListPtr == clockInfoList)
            { /* special case for first entry */
                if (clockInfoListPtr->next == NULL)
                {
                    MessageBox(parentWindow,
                               "Cannot delete last clock!",
                               "World Clock Error Message",
                               MB_ICONINFORMATION | MB_OK);
                } /* if clockInfoListPtr->next == NULL */
                else
                {
                    clockInfoList = clockInfoListPtr->next;
                    DeleteClockListEntry(parentWindow, layout, clockInfoListPtr);
                    break;
                } /* if clockInfoListPtr->next == NULL */
            } /* if clockInfoListPtr == clockInfoList */
            else
            { /* was not the first entry */
                lastNode->next = clockInfoListPtr->next; /* unlink this node */
                DeleteClockListEntry(parentWindow, layout, clockInfoListPtr);
                break;
            } /* if clockInfoListPtr == clockInfoList */
        } /* if clockInfoListPtr->hwnd == clockWindow */
        lastNode = clockInfoListPtr;
        clockInfoListPtr = clockInfoListPtr->next;
    } /* while clockInfoListPtr != NULL */
} /* DeleteClock() */

void DeleteClockListEntry(HWND parentWindow, int layout, ClockInfoListStruct *deleteEntry)
{
    SendMessage(deleteEntry->hwnd, WM_CLOSE, 0, 0L);
    wfree(deleteEntry);
    numClocks--;
    AdjustWindow(parentWindow, layout);
} /* DeleteClockListEntry() */

void AdjustWindow(HWND hwnd, int layout)
{
    int x, y, width, height;
    int deltaX, deltaY, newX, newY;
    ClockInfoListStruct *clockInfoListPtr;

    if (layout & OR_VERT)
    { /* vertical layout */
        width = CLOCK_DISPLAY_WIDTH;
        height = CLOCK_DISPLAY_HEIGHT * numClocks;
        deltaX = 0;
        deltaY = CLOCK_DISPLAY_HEIGHT;
    }
    else
    { /* horizontal layout */
        width  = CLOCK_DISPLAY_WIDTH * numClocks;
        height = CLOCK_DISPLAY_HEIGHT;
        deltaX = CLOCK_DISPLAY_WIDTH;
        deltaY = 0;
    }

    if (layout & POS_RIGHT)
        x = GetSystemMetrics(SM_CXSCREEN) - width;
    else
        x = 0;

    if (layout & POS_BOTTOM)
        y = GetSystemMetrics(SM_CYSCREEN) - height;
    else
        y = 0;

    clockInfoListPtr = clockInfoList;
    newX = 0;
    newY = 0;
    while (clockInfoListPtr != NULL)
    {
        MoveWindow(clockInfoListPtr->hwnd, newX, newY, CLOCK_DISPLAY_WIDTH, CLOCK_DISPLAY_HEIGHT, TRUE);
        newX += deltaX;
        newY += deltaY;
        clockInfoListPtr = clockInfoListPtr->next;
    } /* while clockInfoListPtr != NULL */
    SetWindowPos(hwnd, (layout & ON_TOP) ? HWND_TOPMOST : HWND_NOTOPMOST,  x, y, width, height, SWP_SHOWWINDOW);
    CheckMenuItem(popupMenu, WC_ONTOP, ((layout & ON_TOP) ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);
} /* AdjustWindow */


