#define TG_NULL         0       /* End-of-list */
#define TG_CONTAINER    1       /* Outer container - always must be 1st item */
#define TG_LABEL        2       /* A string label */
#define TG_CHECKBOX     3       /* A true/false checkbox */
#define TG_MENU         4       /* A menu */
#define TG_BUTTON       5       /* A command button */
#define TG_TEXTEDIT     6       /* Text editor */
#define TG_HSLIDE       7       /* A horizontal slider */

typedef struct TextGadgetStruct {
    int kind;           /* From TG_*, above */
    int row, col;       /* Position */
    int size;           /* Typically, width */
    void *pval;         /* String or array of strings (for menu) */
    int ival;           /* Integer value - menu, hslide, checkbox */
    char *valid;        /* Valid input chars, or NULL if anything goes */
    int start;          /* Starting offset of string */
    int cursor;         /* Cursor pos in string gadget */
} TextGadget;

int tgInit(void);
void tgPaintAll(TextGadget *glist);
void tgUpdate(TextGadget *glist, int which);
int tgCheckInput(TextGadget *glist, void (*callback)(TextGadget *, int));
void tgTerm(void);
void tgAddString(const char *str, TextGadget *glist, int start, int num);

/* EOF textgad.h */
