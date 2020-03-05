#ifndef __USER_SELECTOR_H__
#define __USER_SELECTOR_H__

// file type (*.bin is not supported, and can be opened only by File->Open)
enum SELECTOR_FILE
{
    SELECTOR_FILE_EXEC = 1,         // any GC executable (*.dol, *.elf)
    SELECTOR_FILE_DVD               // any DVD image (*.gcm, *.gmp)
};

// file info limits
#define MAX_TITLE       128         // 64 wasnt enough :(
#define MAX_COMMENT     128

// file entry
typedef struct UserFile
{
    int     type;                   // see above (one of SELECTOR_FILE_*)
    int     size;                   // file size
    char    id[8];                  // GameID = DiskID + banner checksum
    char    name[2*MAX_PATH+2];     // file path and name
    char    title[MAX_TITLE];       // alternate file name
    char    comment[MAX_COMMENT];   // some notes
    int     icon[2];                // banner/icon + same but highlighted
} UserFile;

// selector columns
#define SELECTOR_COLUMN_BANNER  "Icon"
#define SELECTOR_COLUMN_TITLE   "Title"
#define SELECTOR_COLUMN_SIZE    "Size"
#define SELECTOR_COLUMN_GAMEID  "Game ID"
#define SELECTOR_COLUMN_COMMENT "Comment"

// sort by ..
enum SELECTOR_SORT
{
    SELECTOR_SORT_DEFAULT = 1,      // first by icon, then by title
    SELECTOR_SORT_FILENAME,
    SELECTOR_SORT_TITLE,
    SELECTOR_SORT_SIZE,
    SELECTOR_SORT_ID,
    SELECTOR_SORT_COMMENT
};

// selector API
void    CreateSelector();
void    CloseSelector();
void    SetSelectorIconSize(bool smallIcon);
bool    AddSelectorPath(char *fullPath);            // FALSE, if path duplicated
void    ResizeSelector(int width, int height);
void    UpdateSelector();
int     SelectorGetSelected();
void    SelectorSetSelected(int item);
void    SelectorSetSelected(char *filename);
void    SortSelector(int sortBy);
void    DrawSelectorItem(LPDRAWITEMSTRUCT item);
void    NotifySelector(LPNMHDR pnmh);
void    ScrollSelector(char letter);

// all important data is placed here
typedef struct UserSelector
{
    bool        active;             // 1, if enabled (under control of UserWindow)
    bool        opened;             // 1, if visible
    bool        smallIcons;         // show small icons
    int         sortBy;             // sort rule (one of SELECTOR_SORT_*)
    int         width;              // selector width
    int         height;             // selector height

    HWND        hSelectorWindow;    // selector window handler
    HMENU       hFileMenu;          // popup file menu
    bool        compress;           // current file action (1:compress, 0:decompress)
    char        file1[256];         // first file for GCMCMPR
    char        file2[256];         // second file for GCMCMPR
    UserFile*   selected;           // first selected item (temporary for edit file dialog)

    // path list, where to search files.
    char**      paths;
    int         pathnum;

    // file filter
    uint32_t    filter;             // every 8-bits masking extension : [DOL][ELF][GCM][GMP]

    // we are using self-extended file list, so file count is
    // unlimited in theory (limited only by system resources).
    UserFile*   files;              // file list
    int         filenum;            // file count
} UserSelector;

extern  UserSelector usel;

#endif  // __USER_SELECTOR_H__
