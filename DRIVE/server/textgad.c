#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "curses.h"
#include "textgad.h"

#define TG_LEFT         -1
#define TG_CENTER       -2
#define TG_RIGHT        -3

#define CTRL(x) ((x) & 31)

typedef struct {
    void (*repaint)(TextGadget *glist, int which);
    int (*input)(TextGadget *glist, int which, int key,
                 void (*callback)(TextGadget *, int));
} TextFunTab;

static void
    paintLabel(TextGadget *glist, int which),
    paintCheckbox(TextGadget *glist, int which),
    paintMenu(TextGadget *glist, int which),
    paintButton(TextGadget *glist, int which),
    paintTextEdit(TextGadget *glist, int which),
    paintHSlide(TextGadget *glist, int which);
static int
    keyCheckbox(TextGadget *glist, int which, int key,
                void (*callback)(TextGadget *, int)),
    keyMenu(TextGadget *glist, int which, int key,
            void (*callback)(TextGadget *, int)),
    keyButton(TextGadget *glist, int which, int key,
              void (*callback)(TextGadget *, int)),
    keyTextEdit(TextGadget *glist, int which, int key,
                void (*callback)(TextGadget *, int)),
    keyHSlide(TextGadget *glist, int which, int key,
              void (*callback)(TextGadget *, int));

static TextFunTab funcTab[] = {
    { 0, 0 },                           /* NULL */
    { 0, 0 },                           /* CONTAINER */
    { paintLabel, NULL },               /* LABEL */
    { paintCheckbox, keyCheckbox },     /* CHECKBOX */
    { paintMenu, keyMenu },             /* MENU */
    { paintButton, keyButton },         /* BUTTON */
    { paintTextEdit, keyTextEdit },     /* TEXTEDIT */
    { paintHSlide, keyHSlide },         /* HSLIDE */
};

int tgInit(void)
{
    if (!initscr()) {
        return 0;
    }
    cbreak();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    noecho();
    return 1;
}

int tgCompile(TextGadget *glist)
{
    int i;
    int active = -1;

    if (glist[0].kind != TG_CONTAINER) {
        return 0;
    }
    for (i = 1; glist[i].kind != TG_NULL; i++) {
        /* Check for exactly 1 container, and find first active gadget */
        switch (glist[i].kind) {
        case TG_CONTAINER :
            return 0;
        case TG_LABEL :
            break;
        default :
            if (active < 0) {
                active = i;
            }
            break;
        }
    }
    glist[0].ival = active;
    glist[0].size = i;
    return 1;
}

void tgPaintAll(TextGadget *glist)
{
    int i;

    wclear(stdscr);
    for (i = 1; glist[i].kind != TG_NULL; i++) {
        tgUpdate(glist, i);
    }
    refresh();
}

static void paintRout(char *str, int width,
                        int c1, int c2, int pad,
                        int align, int active)
{
    int len, spacesBefore, spacesAfter;
    int i;

    len = strlen(str);
    if (width) {
        if (len > width) {
            len = width;
        }
        switch (align) {
        case TG_LEFT :
            spacesBefore = 0;
            break;
        case TG_CENTER :
            spacesBefore = (width - len) / 2;
            break;
        case TG_RIGHT :
            spacesBefore = width - len;
            break;
        default :
            spacesBefore = align;
            break;
        }
        spacesAfter = width - len - spacesBefore;
    } else {
        /* Default for un-sized gadgets is 1 space before and after */
        spacesBefore = spacesAfter = 1;
    }

    if (active) {
        /* Secondary visual indicator for active gadgets - replace
         * left and right margins with arrows
         */
        c1 = '>'; c2 = '<';
    }

    addch(c1);

    if (active) {
        /* Another visual indicator for active gadgets - reverse
         * video
         */
        standout();
    }

    for (i = 0; i < spacesBefore; i++) {
        addch(pad);
    }
    for (i = 0; i < len; i++) {
        addch(str[i]);
    }
    for (i = 0; i < spacesAfter; i++) {
        addch(pad);
    }

    if (active) {
        standend();
    }

    addch(c2);
}

void tgActivate(TextGadget *glist, int which)
{
    int active;

    active = glist[0].ival;

    glist[0].ival = which;
    tgUpdate(glist, active);
    tgUpdate(glist, which);
    refresh();
}

static void paintLabel(TextGadget *glist, int which)
{
    move(glist[which].row, glist[which].col);
    addstr((char *)glist[which].pval);
}

static void paintCheckbox(TextGadget *glist, int which)
{
    int active = (which == glist[0].ival);

    move(glist[which].row, glist[which].col);
    if (glist[which].ival) {
        paintRout("*", 1, '[', ']', ' ', 0, active);
    } else {
        paintRout(" ", 1, '[', ']', ' ', 0, active);
    }
    addch(' ');
    addstr((char *)glist[which].pval);
}

static void paintMenu(TextGadget *glist, int which)
{
    int active = (which == glist[0].ival);
    char **list, *str;
    int n;

    move(glist[which].row, glist[which].col);
    list = (char **) glist[which].pval;
    str = list[glist[which].ival];
    n = glist[which].size;
    paintRout(str, n, '[', ']', ' ', TG_CENTER, active);
}

static void paintButton(TextGadget *glist, int which)
{
    int active = (which == glist[0].ival);
    char *str;
    int n;

    move(glist[which].row, glist[which].col);
    str = (char *) glist[which].pval;
    n = glist[which].size;
    paintRout(str, n, '[', ']', ' ', TG_CENTER, active);
}

static void paintTextEdit(TextGadget *glist, int which)
{
    int active = (which == glist[0].ival);
    char *str;
    int n;

    str = (char *) glist[which].pval;
    n = glist[which].size;
    str += glist[which].start;

    move(glist[which].row, glist[which].col);
    paintRout("", n, '+', '+', '-', 0, 0);
    move(glist[which].row+1, glist[which].col);
    paintRout(str, n, '|', '|', ' ', TG_LEFT, active);
    move(glist[which].row+2, glist[which].col);
    paintRout("", n, '+', '+', '-', 0, 0);
}

static void paintHSlide(TextGadget *glist, int which)
{
    int active = (which == glist[0].ival);
    int i, n;

    move(glist[which].row, glist[which].col);
    n = glist[which].size;
    i = glist[which].ival;
    paintRout("#", n, '[', ']', '-', i, active);
}

void tgUpdate(TextGadget *glist, int which)
{
    int n = glist[which].kind;
    if ((n < 0) || (n > TG_HSLIDE)) {
        return;
    }

    funcTab[n].repaint(glist, which);
}

void tgFlush(TextGadget *glist)
{
    int active = glist[0].ival;

    switch (glist[active].kind) {
    case TG_TEXTEDIT :
        move(glist[active].row+1, glist[active].col+glist[active].cursor+1);
        break;
    case TG_HSLIDE :
        move(glist[active].row, glist[active].col+glist[active].ival+1);
        break;
    default :
        move(glist[active].row, glist[active].col+glist[active].cursor+1);
        break;
    }
    refresh();
}

static int keyCheckbox(TextGadget *glist, int which, int key,
                       void (*callback)(TextGadget *, int))
{
    switch (key) {
    case ' ' : case '\n' : case CTRL('F') :
    case '\b' : case CTRL('B') :
        /* Toggles the checkbox and calls back */
        glist[which].ival = !glist[which].ival;
        tgUpdate(glist, which);
        tgFlush(glist);
        (*callback)(glist, which);
        return 1;
    default :
        return 0;
    }
}

static int keyMenu(TextGadget *glist, int which, int key,
                   void (*callback)(TextGadget *, int))
{
    char **list = (char **) glist[which].pval;
    int i, n, newActive;

    switch (key) {
    case ' ' : case CTRL('F') :
        /* Increments the menu but does *not* call back */
        n = glist[which].ival + 1;
        if (list[n]) {
            glist[which].ival = n;
            tgUpdate(glist, which);
            tgFlush(glist);
        }
        return 1;

    case '\b' : case CTRL('B') :
        /* Decrements the menu but does *not* call back */
        n = glist[which].ival - 1;
        if (n >= 0) {
            glist[which].ival = n;
            tgUpdate(glist, which);
            tgFlush(glist);
        }
        return 1;

    case KEY_HOME : case CTRL('A') :
        glist[which].ival = 0;
        tgUpdate(glist, which);
        tgFlush(glist);
        return 1;

    case KEY_END : case CTRL('E') :
        for (i = 0; list[i]; i++) {
            /* NOTHING */
        }
        glist[which].ival = i-1;
        tgUpdate(glist, which);
        tgFlush(glist);
        return 1;

    case '\t' : /*case CTL_TAB : case ALT_TAB :*/ case KEY_BTAB :
        /* Call the callback */
        (*callback)(glist, which);
        /* However, we did *not* handle the key (this lets it
         * transition to the next gadget)
         */
        return 0;

    case '\r' : case '\n' :
        /* Call the callback */
        (*callback)(glist, which);
        return 1;

    default :
        if ((key < 32) || (key > 126)) {
            /* Only allow ASCII chars */
            return 0;
        }
        newActive = -1;
        for (i = glist[which].ival + 1; list[i]; i++) {
            if (tolower(list[i][0]) == tolower(key)) {
                newActive = i;
                break;
            }
        }
        if (newActive < 0) {
            for (i = 0; i < glist[which].ival; i++) {
                if (tolower(list[i][0]) == tolower(key)) {
                    newActive = i;
                    break;
                }
            }
        }
        if (newActive >= 0) {
            glist[which].ival = newActive;
            tgUpdate(glist, which);
            tgFlush(glist);
            return 1;
        }
        return 0;
    }
}

static int keyButton(TextGadget *glist, int which, int key,
                     void (*callback)(TextGadget *, int))
{
    switch (key) {
    case ' ' : case '\n' : case CTRL('F') :
    case '\b' : case CTRL('B') :
        /* Just calls back */
        (*callback)(glist, which);
        return 1;
    default :
        return 0;
    }
}

static int keyTextEdit(TextGadget *glist, int which, int key,
                       void (*callback)(TextGadget *, int))
{
    char *str = (char *) glist[which].pval;
    int i, n, len;

    switch (key) {
    case '\t' : /*case CTL_TAB : case ALT_TAB :*/ case KEY_BTAB :
        /* Call the callback */
        (*callback)(glist, which);
        /* However, we did *not* handle the key (this lets it
         * transition to the next gadget)
         */
        return 0;

    case '\r' : case '\n' :
        /* Call the callback */
        (*callback)(glist, which);
        return 1;

    case '\b' :
        n = glist[which].cursor + glist[which].start;
        if (n == 0) {
            return 1;
        }
        for (n--; str[n]; n++) {
            str[n] = str[n+1];
        }
        if (glist[which].cursor > 0) {
            glist[which].cursor--;
        } else {
            glist[which].start--;
        }
        tgUpdate(glist, which);
        tgFlush(glist);
        return 1;

    case KEY_HOME : case CTRL('A') :
        glist[which].cursor = glist[which].start = 0;
        tgUpdate(glist, which);
        tgFlush(glist);
        return 1;

    case KEY_END : case CTRL('E') :
        len = strlen(str);
        if (len < (glist[which].size-1)) {
            glist[which].cursor = len;
        } else {
            glist[which].cursor = glist[which].size - 1;
            glist[which].start = len - glist[which].size + 1;
        }
        tgUpdate(glist, which);
        tgFlush(glist);
        return 1;

    case KEY_LEFT : case CTRL('B') :
    doBackward :
        n = glist[which].cursor + glist[which].start;
        if (n <= 0) {
            return 1;
        }
        if (glist[which].cursor == 0) {
            if (glist[which].start > 0) {
                glist[which].start--;
            }
        } else {
            glist[which].cursor--;
        }
        tgUpdate(glist, which);
        tgFlush(glist);
        return 1;

    case KEY_RIGHT : case CTRL('F') :
    doForward :
        n = glist[which].cursor + glist[which].start;
        if (!str[n]) {
            return 1;
        }
        if (glist[which].cursor < (glist[which].size-1)) {
            glist[which].cursor++;
        } else {
            glist[which].start++;
        }
        tgUpdate(glist, which);
        tgFlush(glist);
        return 1;

    case CTRL('D') : case KEY_DC : case 127 : /* DEL */
        n = glist[which].cursor + glist[which].start;
        for ( ; str[n]; n++) {
            str[n] = str[n+1];
        }
        tgUpdate(glist, which);
        tgFlush(glist);
        return 1;

    default :
        if ((key < 32) || (key > 126)) {
            /* Only allow ASCII chars */
            return 0;
        }
        if (glist[which].valid && !strchr(glist[which].valid, key)) {
            /* Not a valid key */
            return 1;
        }
        len = strlen(str);
        if (len >= glist[which].ival) {
            break;
        }
        n = glist[which].cursor + glist[which].start;
        for (i = len; i > n; i--) {
            str[i] = str[i-1];
        }
        str[n] = key;
        goto doForward;
    }
    return 0;
}

static int keyHSlide(TextGadget *glist, int which, int key,
                     void (*callback)(TextGadget *, int))
{
    switch (key) {
    case ' ' : case '\n' : case CTRL('F') :
        /* Moves the slider and calls the callbac */
        if (glist[which].ival < (glist[which].size-1)) {
            glist[which].ival++;
            tgUpdate(glist, which);
            tgFlush(glist);
        }
        (*callback)(glist, which);
        return 1;
    case '\b' : case CTRL('B') :
        if (glist[which].ival > 0) {
            glist[which].ival--;
            tgUpdate(glist, which);
            tgFlush(glist);
        }
        (*callback)(glist, which);
        return 1;
    default :
        return 0;
    }
}

int tgCheckInput(TextGadget *glist, void (*callback)(TextGadget *, int))
{
    int i, n, key;
    int active, newActive;

    key = getch();
    if (key <= 0) {
        return 0;
    }

#if 0
{FILE *f = fopen("keys.txt", "a");
if (f) {
    fprintf(f, "Key: %d\n", key);
    fclose(f);
}}
#endif

    active = glist[0].ival;
    n = glist[active].kind;

    /* First, give each gadget a chance to handle the input */
    if ((n >= 0) && (n <= TG_HSLIDE) && funcTab[n].input) {
        if (funcTab[n].input(glist, active, key, callback)) {
            return key;
        }
    }

    switch (key) {
    /*case CTL_TAB : case ALT_TAB :*/ case KEY_BTAB :
        /* Go to previous gadget */
        newActive = -1;
        for (i = active - 1; i > 0; i--) {
            if (glist[i].kind != TG_LABEL) {
                newActive = i;
                break;
            }
        }
        if (newActive < 0) {
            for (i = glist[0].size-1; i > active; i--) {
                if (glist[i].kind != TG_LABEL) {
                    newActive = i;
                    break;
                }
            }
        }
        glist[0].ival = newActive;
        tgUpdate(glist, active);
        tgUpdate(glist, newActive);
        tgFlush(glist);
        break;

    case '\t' :
        /* Advance to next gadget */
        newActive = -1;
        for (i = active + 1; glist[i].kind != TG_NULL; i++) {
            if (glist[i].kind != TG_LABEL) {
                newActive = i;
                break;
            }
        }
        if (newActive < 0) {
            for (i = 1; i < active; i++) {
                if (glist[i].kind != TG_LABEL) {
                    newActive = i;
                    break;
                }
            }
        }
        glist[0].ival = newActive;
        tgUpdate(glist, active);
        tgUpdate(glist, newActive);
        tgFlush(glist);
        break;


    default :
        break;
    }

    return key;
}

void tgAddString(const char *str, TextGadget *glist, int start, int num)
{
    int i, n, idx;
    char *ptr;

    for (i = 1; i < num; i++) {
        strcpy(glist[start+i-1].pval, glist[start+i].pval);
        tgUpdate(glist, start+i-1);
    }

    idx = start+num-1;
    n = strlen(str);
    ptr = glist[idx].pval;

    if( n >= glist[idx].ival ) {
        n = glist[idx].ival - 1;
    }

    for (i = 0; i < n; i++) {
        *ptr++ = *str++;
    }
    for (; i < glist[idx].ival; i++) {
        *ptr++ = ' ';
    }
    *ptr = 0;

    tgUpdate(glist, idx);
    refresh();
}

void tgTerm(void)
{
    endwin();
}

/* EOF textgad.c */
