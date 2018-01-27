typedef struct ClockInfoListStructTag {
    HWND hwnd;
    struct ClockInfoListStructTag *next;
} ClockInfoListStruct;

typedef struct ClockInfoTag {
    short gmtOffset;
    char *locationName;
} ClockInfoStruct;

#define CLOCK_NAME_SIZE 32
#define VERSION	"1.10 -- March 31, 2013"

#define TIMEZONE_NAME	101
#define GMT_OFFSET_SLIDER	102
#define GMT_OFFSET_TEXT	103

#define wfree(z)   LocalFree((LOCALHANDLE) z)
#define wmalloc(z) LocalAlloc(LPTR, z)

#define WC_ADD	    101
#define WC_MODIFY	102
#define WC_DELETE	103
#define WC_ONTOP    104
#define WC_SAVEDATA 105
#define WC_ABOUT    106
#define WC_EXIT	    107

#define POS_RIGHT    0x01
#define POS_BOTTOM   0x02
#define OR_VERT      0x04
#define ON_TOP       0x08

#define WC_POS_UL       200
#define WC_POS_UR       201
#define WC_POS_LL       202
#define WC_POS_LR       203

#define WC_OR_BASE      210
#define WC_OR_HORZ      210
#define WC_OR_VERT      211

extern HMENU popupMenu;

