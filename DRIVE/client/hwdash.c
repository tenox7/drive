/*
 * hwdash.c - Dashboard for the GLFW build.
 *
 * The X11/Motif client drew the dash into a second window from libgui.
 * That code is all behind #if defined(MOTIF_GUI) and is not built here, so
 * this file reproduces it on top of the main window using the original
 * artwork (pixmaps/dboard.jpg and friends) and the layout the author
 * already wrote in pixmaps/drive_gui.hw.
 *
 * Everything is laid out on the original 800 x 300 dash canvas
 * (LARGEDASH_WIDTH x LARGEDASH_HEIGHT) and scaled to wherever the layout
 * actually landed in the window, so the original coordinates from
 * libgui/windows.h are used unchanged.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "global.h"
#include "gauge.h"
#include "drive.h"
#include "filenames.h"
#include "hw.h"
#include "TexFont.h"

extern hwDisplay disp;
extern gauge_type left_main_gauge, right_main_gauge, turbometer, blastometer;
extern gear_type gear;

/**** Original dash geometry, from libgui/windows.h ****/
#define LARGEDASH_WIDTH		800
#define LARGEDASH_HEIGHT	300
#define GAUGE_RADIUS		90
#define HUB_RADIUS		80
#define WHEEL_RADIUS		310
#define HUB_CENTER_X		295
#define HUB_CENTER_Y		350
#define LARGECOMPASS_X1		270
#define LARGECOMPASS_Y1		60
#define LARGECOMPASS_WWIDTH	60
#define LARGECOMPASS_WHEIGHT	30
#define GEAR_XC		(LARGECOMPASS_X1+LARGECOMPASS_WWIDTH/2)
#define GEAR_YC		((LARGECOMPASS_Y1+LARGECOMPASS_WHEIGHT+(HUB_CENTER_Y-HUB_RADIUS))/2)

/* Menu bar across the very bottom, below the dash */
#define MENU_BAR_H	30

/* Width of the accelerator/brake bar down the right edge */
#define ACCBRKBAR_WIDTH		25

/**** Steering wheel, from libgui/wheel.c ****/
#define SPOKES		3
#define SPOKE_HOLES	2
#define SPOKE_WIDTH1	60	/* At hub */
#define SPOKE_WIDTH2	30	/* At wheel */

#define WHITE		0xFFFFFFFF
#define BLACK		0xFF000000
#define RED		0xFFFF0000

static hwObject guiLayout;		/* driveGui from drive_gui.hw */
static hwObject speedo, tach;
static hwObject gearImg[7];		/* R N 1 2 3 4 5 */
static int guiLoaded;

static int helpVisible;

static hwObject menuBar;
static void loadMenus(void);
static void drawMenus(void);
static void markConfig(void);

/* Where the dash canvas ended up on screen, and its scale. */
static float dashScale = 1.0;
static int dashX, dashY;

static void drawHelpPanel(void);

/****************************************************************************
 * Gauge bookkeeping.  The values arrive through gaugeUpdate(); all of the
 * painting happens once per frame in drawDashboard().
 */
void initGaugeModule(void) {}
void addGauge(gauge_type *g) {}
void removeGauge(gauge_type *g) {}
void gaugeDraw(gauge_type *g) {}
void addGear(gear_type *g) {}
void removeGear(gear_type *g) {}
void gearDraw(gear_type *g) {}
void compassDraw(void) {}
void redrawDash(void) {}

/* Last thing the original bargraphUpdate() did, in libgui/gauge.c. */
void bargraphUpdate(float value, boolean_type force)
{
    cstate.accBrk_value = value;
}

void gaugeUpdate(gauge_type *g, float value, boolean_type force)
{
    if (!g) return;
    g->value = value;
}

void compassUpdate(compass_type *c, float travel_angle, float home_angle,
    boolean_type force)
{
    if (!c) return;
    c->travel_angle = travel_angle;
    c->home_angle = home_angle;
}

/****************************************************************************
 * Load pixmaps/drive_gui.hw.  Its image FileNames are relative, so they are
 * rewritten to absolute paths once the objects exist.
 */
static void setImageFile(hwObject img, const char *leaf)
{
    char path[MAXPATHLEN];

    if (!img) return;
    sprintf(path, "%s/pixmaps/%s", drivedir, leaf);
    img->modify(img, hwStrFileName, HW_TYPE_STRING, path);
}

static void loadGui(void)
{
    static char *leaf[7] = { "dashR.jpg", "dashN.jpg", "dash1.jpg",
			     "dash2.jpg", "dash3.jpg", "dash4.jpg",
			     "dash5.jpg" };
    static char *objName[7] = { "gearR", "gearN", "gear1", "gear2",
				"gear3", "gear4", "gear5" };
    char path[MAXPATHLEN];
    hwObject *objs;
    int i;

    guiLoaded = 1;

    sprintf(path, "%s/pixmaps/drive_gui.hw", drivedir);
    if (hwParseFile(path, &objs) <= 0) {
	fprintf(stderr, "drive: could not read %s; no dashboard\n", path);
	return;
    }

    guiLayout = hwFindObject("driveGui");
    speedo    = hwFindObject("speedo");
    tach      = hwFindObject("tach");

    setImageFile(hwFindObject("dashboard"), "dboard.jpg");
    for (i = 0; i < 7; i++) {
	gearImg[i] = hwFindObject(objName[i]);
	setImageFile(gearImg[i], leaf[i]);
    }

    loadMenus();
}

/* The whole window.  cstate.gWin* is the 3D viewport, which is smaller. */
static int winFullW = 640, winFullH = 480;
static int dashW, dashH;
static int viewW = 640, viewH = 480;

/*
 * The GUI overlay is flushed inside disp->update() and beginGui() builds its
 * ortho from the current viewport, so the viewport has to cover the whole
 * window while the overlay is queued and flushed.  client_update() puts the
 * 3D viewport back afterwards.
 */
void dashBeginOverlay(void)
{
    if (disp) disp->viewport(disp, 0, 0, winFullW, winFullH);
}

void dashEndOverlay(void)
{
    if (disp) disp->viewport(disp, 0, dashH + MENU_BAR_H, viewW, viewH);
}

/*
 * Carve the window up the way the original did: the dash across the bottom,
 * the accelerator/brake bar down the right hand edge, and the 3D view in
 * what is left.  Shrinking the viewport keeps the road clear of the dash.
 */
/* While the vehicle picker is up there is no dash or menu bar, so the
 * spinning vehicle gets the whole window.
 */
static int pickerMode;

void dashSetPicker(int on)
{
    pickerMode = on;
    dashResize(winFullW, winFullH);
}

void dashResize(int w, int h)
{
    winFullW = w;
    winFullH = h;

    if (pickerMode) {
	cstate.gWinWidth  = w;
	cstate.gWinHeight = h;
	if (disp) disp->viewport(disp, 0, 0, w, h);
	return;
    }

    dashW = w - ACCBRKBAR_WIDTH;
    if (dashW < 1) dashW = 1;
    dashH = (int)(LARGEDASH_HEIGHT * ((float)dashW / (float)LARGEDASH_WIDTH));

    /* Bottom to top: menu bar, dash, 3D view -- as the original stacked it. */
    viewW = dashW;
    viewH = h - dashH - MENU_BAR_H;
    if (viewH < 64) {		/* Window too short for a dash */
	dashH = 0;
	viewH = h - MENU_BAR_H;
    }

    dashScale = (float)dashW / (float)LARGEDASH_WIDTH;
    dashX = 0;
    dashY = h - MENU_BAR_H - dashH;

    cstate.gWinWidth  = viewW;
    cstate.gWinHeight = viewH;

    /* GL viewport origin is bottom left, so the view sits above the dash. */
    if (disp) disp->viewport(disp, 0, dashH + MENU_BAR_H, viewW, viewH);

    if (!guiLayout) return;
    HW_MODIFY_1I(guiLayout, hwStrAlign,  0);	/* absolute pixels */
    HW_MODIFY_1I(guiLayout, hwStrPosX,   dashX);
    HW_MODIFY_1I(guiLayout, hwStrPosY,   dashY);
    HW_MODIFY_1I(guiLayout, hwStrWidth,  dashW);
    HW_MODIFY_1I(guiLayout, hwStrHeight, dashH);
}

static void placeDash(void)
{
    static int placed;

    if (!placed && guiLayout) {
	placed = 1;
	dashResize(winFullW, winFullH);
    }
}

/* Dash canvas coordinates -> window coordinates */
#define DX(x)	(dashX + (int)((x) * dashScale))
#define DY(y)	(dashY + (int)((y) * dashScale))
#define DS(v)	((int)((v) * dashScale))

/****************************************************************************
 * Text
 */
static void dashText(int size, int color, int x, int y, const char *s)
{
    TexFont *txf = txfLoadStaticFont(size);

    disp->guiText(disp, txf, color, HW_TEXT_ALIGN_LEFT, HW_TEXT_ALIGN_BOTTOM,
	size, x, y, (unsigned char *)s);
}

static void centerText(int size, int color, int cx, int y, const char *s)
{
    TexFont *txf = txfLoadStaticFont(size);
    int width = 0, asc = 0, desc = 0;

    txfGetStringMetrics(txf, (unsigned char *)s, strlen(s), &width, &asc, &desc);
    dashText(size, color, cx - width/2, y + asc/2, s);
}

/****************************************************************************
 * Steering wheel -- libgui/wheel.c, drawn as white spokes over the image.
 */
#define XFORMPT(xin,yin,xo,yo) \
{   float _xt = (float)(xin), _yt = (float)(yin); \
    (xo) = DX(ca*_xt - sa*_yt + HUB_CENTER_X); \
    (yo) = DY(sa*_xt + ca*_yt + HUB_CENTER_Y); \
}

static void drawSpoke(float angle)
{
    float ca = cos(angle), sa = sin(angle);
    hwInt32 pts[4];
    int i, x, dx, diam, ddiam;
    int cxp, cyp;

    XFORMPT(HUB_RADIUS,   (SPOKE_WIDTH1/2), pts[0], pts[1]);
    XFORMPT(WHEEL_RADIUS, (SPOKE_WIDTH2/2), pts[2], pts[3]);
    disp->guiLines(disp, 0, WHITE, 2, pts);

    XFORMPT(HUB_RADIUS,   (-SPOKE_WIDTH1/2), pts[0], pts[1]);
    XFORMPT(WHEEL_RADIUS, (-SPOKE_WIDTH2/2), pts[2], pts[3]);
    disp->guiLines(disp, 0, WHITE, 2, pts);

    /* The two holes punched through each spoke */
    dx = ((WHEEL_RADIUS - HUB_RADIUS) / SPOKE_HOLES);
    x = HUB_RADIUS + dx/2;
    ddiam = ((SPOKE_WIDTH2 - SPOKE_WIDTH1) / SPOKE_HOLES * 3/4);
    diam = (SPOKE_WIDTH1 * 3/4) + ddiam/2;

    for (i = 0; i < SPOKE_HOLES; ++i, x += dx, diam += ddiam) {
	hwInt32 circle[2*17];
	int j, r = DS(diam/2);

	XFORMPT(x, 0, cxp, cyp);
	for (j = 0; j <= 16; j++) {
	    float a = j * (2.0 * M_PI / 16.0);
	    circle[j*2]   = cxp + (int)(r * cos(a));
	    circle[j*2+1] = cyp + (int)(r * sin(a));
	}
	disp->guiPolyline(disp, 0, WHITE, 17, circle);
    }
}

static void drawSteeringWheel(void)
{
    float angle, dangle;
    hwInt32 hub[2*25];
    int i, r;

    /* Hub */
    r = DS(HUB_RADIUS);
    for (i = 0; i < 24; i++) {
	float a = i * (2.0 * M_PI / 24.0);
	hub[i*2]   = DX(HUB_CENTER_X) + (int)(r * cos(a));
	hub[i*2+1] = DY(HUB_CENTER_Y) + (int)(r * sin(a));
    }
    disp->guiPolygon(disp, 0, BLACK, 24, hub);

    dangle = ((2.0 * M_PI) / SPOKES);
    angle  = (M_PI / 2.0) + cstate.xpointer_value * M_PI;
    for (i = 0; i < SPOKES; ++i, angle += dangle) {
	drawSpoke(angle);
    }
}

/****************************************************************************
 * Analog gauge needles
 */
static void updateNeedle(hwObject g, gauge_type *src)
{
    if (!g || !src) return;
    HW_MODIFY_1F(g, hwStrNeedlePos, src->value);
}

/****************************************************************************
 * Gear column -- libgui/gauge.c gearDraw().  Labels run bottom-up with the
 * selected one in red, over a black panel.
 */
static void drawGearColumn(void)
{
    int i, ch, w, h, x, y;

    ch = 18;				/* charheight on the 800x300 canvas */
    w  = 26;
    h  = ch * gear.num_gears;
    x  = GEAR_XC - w/2;
    y  = GEAR_YC - h/2;

    disp->guiRectangle(disp, 0, BLACK, DX(x), DY(y), DS(w), DS(h), 0);

    for (i = 0; i < gear.num_gears; i++) {
	int color = (i == gear.actual) ? RED : WHITE;
	int ty = y + (gear.num_gears - i) * ch - ch/2;

	centerText(12, color, DX(x + w/2), DY(ty), gear.labels[i]);
    }
}

/****************************************************************************
 * Digital readouts -- turbometer at (20,10), blastometer at (760,10)
 */
static void drawDigital(gauge_type *g, int x, int y, int rightAlign)
{
    char buf[64];
    int size = 18;

    if (g->value < 0) return;
    sprintf(buf, "%d", (int)(g->value + 0.5));
    dashText(size, WHITE, DX(x), DY(y) + size, buf);
    dashText(12, WHITE, DX(x), DY(y) + size + 14,
	g->label ? g->label : "");
}

/****************************************************************************
 * Accelerator / brake bar -- the vertical strip down the right hand edge
 * of the 3D view, as in libgui/windows.c.
 */
static void drawAccBrkBar(void)
{
    int w = ACCBRKBAR_WIDTH;
    int x = winFullW - w;
    int top = 0;
    int bot = winFullH;
    int h = bot - top;
    int mid = top + h/2;
    int len;
    float v = cstate.accBrk_value;

    if (h <= 0 || w <= 0) return;

    disp->guiRectangle(disp, 0, BLACK, x, top, w, h, 0);

    if (v > 1.0)  v = 1.0;
    if (v < -1.0) v = -1.0;

    len = (int)(v * (h/2));
    if (len > 0)      disp->guiRectangle(disp, 0, 0xFF00FF00, x, mid - len, w, len, 0);
    else if (len < 0) disp->guiRectangle(disp, 0, 0xFFFF0000, x, mid, w, -len, 0);

    dashText(12, WHITE, x + 1, top + 14, "ACC");
    dashText(12, WHITE, x + 1, bot - 4,  "BRK");
}

/****************************************************************************
 * Per-frame entry point, called from client_update() before the swap.
 */
void drawDashboard(void)
{
    int i, cur;

    if (!disp || winFullW <= 0 || winFullH <= 0) return;

    dashBeginOverlay();

    if (!guiLoaded) loadGui();
    if (!guiLayout) return;

    /* Only the gear image for the current gear is visible. */
    cur = gear.actual;
    if (cur < 0) cur = 0;
    if (cur > 6) cur = 6;
    for (i = 0; i < 7; i++) {
	if (!gearImg[i]) continue;
	HW_MODIFY_1B(gearImg[i], hwStrInvisible, (i != cur));
    }

    updateNeedle(speedo, &left_main_gauge);
    updateNeedle(tach,   &right_main_gauge);

    placeDash();
    guiLayout->draw(guiLayout);

    drawSteeringWheel();
    drawGearColumn();
    drawDigital(&turbometer,  20, 10, 0);
    drawDigital(&blastometer, 700, 10, 1);
    drawAccBrkBar();

    if (helpVisible) drawHelpPanel();

    drawMenus();
}

/****************************************************************************
 * Keyboard help, from drive_help's "Controls" section.
 */
static const char *helpText[] = {
    "DRIVING",
    "  mouse up/down      accelerate / brake",
    "  mouse left/right   steer",
    "  left/right button  shift down / up",
    "  c                  centre the pointer",
    "",
    "GEARS",
    "  . or >             shift up",
    "  , or <             shift down",
    "",
    "CAMERA",
    "  f or Up            look forward",
    "  s or Left          look left",
    "  d or Right         look right",
    "  a or Down          look back",
    "  i / o              zoom in / out",
    "",
    "VEHICLE",
    "  r                  restart at the start line",
    "  u                  upright the vehicle",
    "",
    "  ?                  close this help",
    "  Esc                quit",
    NULL
};

static void drawHelpPanel(void)
{
    int winW = winFullW;
    int i, y, lineH = 17, pad = 18;
    int w = 420, h = 0;
    int x0, y0;

    for (i = 0; helpText[i]; i++) h += lineH;
    h += pad * 2 + 8;

    x0 = (winW - w) / 2;
    y0 = (dashY - h) / 2;
    if (y0 < 8) y0 = 8;

    disp->guiRectangle(disp, 0, 0xE8101418, x0, y0, w, h, 6);

    y = y0 + pad + 4;
    for (i = 0; helpText[i]; i++) {
	int color = (helpText[i][0] && helpText[i][0] != ' ') ? 0xFFFFD060
							      : WHITE;
	dashText(12, color, x0 + pad, y, helpText[i]);
	y += lineH;
    }
}

void toggleHelpPanel(void)
{
    helpVisible = !helpVisible;
}

/****************************************************************************
 * Menu bar.
 *
 * pixmaps/drive_gui.hw already defines the original's six buttons, so the
 * bar itself is just made visible and fed events.  HoverWare has no menu
 * widget, so each pull-down is an hwRowCol of buttons built here and shown
 * under its title.
 */
#define MENU_ITEM_H	26

#define ID_VEHICLE	1000
#define ID_CONFIG	2000
#define ID_UPRIGHT	3000
#define ID_STARTOVER	4000
#define ID_QUIT		5000
#define ID_HELP		6000

/* Pull-down items; the ID is the title's ID plus the item index.  Fixed
 * depth cue levels, so the setting is absolute and not a nudge.
 */
static char *configItems[] = {
    "  No Fog", "  Little Fog", "  Some Fog", "  More Fog", "  Max Fog",
    "  Automatic Transmission", "  Manual Transmission", NULL
};
static float fogLevel[5] = { 0.0, 0.9, 1.8, 2.4, 3.0 };

static hwObject menuTitle[6];
static hwObject menuPanel[2];		/* Config only; Vehicle has none */
static int menuPanelW[2], menuPanelH[2];
static int openMenu = -1;		/* index into menuTitle, or -1 */

static int menuBarY(void) { return winFullH - MENU_BAR_H; }

static void doMenuCommand(int id)
{
    switch (id) {
    case ID_UPRIGHT :	client_upright();	break;
    case ID_STARTOVER :	client_restart();	break;
    case ID_QUIT :	client_quit(); exit(0);	break;
    case ID_HELP :	toggleHelpPanel();	break;

    case ID_VEHICLE :
	/* Go back to the spinning-vehicle picker.  It runs its own modal
	 * loop, so it has to start from the main loop rather than from
	 * inside this event callback.
	 */
	request_vehicle_select();
	break;

    case ID_CONFIG + 1 :
    case ID_CONFIG + 2 :
    case ID_CONFIG + 3 :
    case ID_CONFIG + 4 :
    case ID_CONFIG + 5 :
	driveFogAmount = fogLevel[id - ID_CONFIG - 1];
	break;
    case ID_CONFIG + 6 :	gear.type = GEAR_AUTOMATIC;	break;
    case ID_CONFIG + 7 :	gear.type = GEAR_STANDARD;	break;
    }
}

static void menuCallback(hwObject obj, hwInt32 reason)
{
    hwInt32 *idp;

    if (reason != HW_CB_ACTIVATE) return;
    if (obj->inquire(obj, hwStrID, (void **)&idp) != HW_TYPE_1I) return;

    /* A title opens or closes its pull-down; an item runs a command. */
    if (*idp == ID_CONFIG) {
	openMenu = (openMenu == 1) ? -1 : 1;
	return;
    }

    openMenu = -1;
    doMenuCommand(*idp);
}

static hwObject buildPanel(const char *name, char **items, int baseID,
    int *wOut, int *hOut)
{
    char header[128], rows[64], buf[160];
    char *spec[32];
    char idbuf[16][64], lbl[16][80];
    hwObject *objs, panel;
    int n = 0, i, count = 0, wide = 0;

    for (i = 0; items[i]; i++) count++;

    sprintf(header, "hwRowCol %s {", name);
    sprintf(rows,   "Rows = %d    Columns = 1", count);

    spec[n++] = header;
    spec[n++] = "Align = 0";
    spec[n++] = rows;
    spec[n++] = "Padding = {2,2}";
    spec[n++] = "BackgroundColor = 0xFF262C33";
    spec[n++] = "FontHeight = 18";
    spec[n++] = "Children = {";
    for (i = 0; i < count; i++) {
	int len = strlen(items[i]);
	if (len > wide) wide = len;
	sprintf(lbl[i], "hwButton { Label = \"%s\" ID = %d }%s",
	    items[i], baseID + i + 1, (i == count-1) ? "" : ",");
	spec[n++] = lbl[i];
    }
    spec[n++] = "}";
    spec[n++] = "}";
    spec[n] = NULL;

    (void)idbuf; (void)buf;

    if (hwParseArray(spec, &objs) <= 0) {
	fprintf(stderr, "drive: could not build %s menu\n", name);
	return NULL;
    }
    panel = hwFindObject((char *)name);
    if (!panel) {
	fprintf(stderr, "drive: %s menu missing after parse\n", name);
	return NULL;
    }

    /* Hook the items up here rather than in the spec, so the callback does
     * not have to be resolved by name at parse time.
     */
    {
	hwObject *kids;
	hwInt32 t = panel->inquire(panel, hwStrChildren, (void **)&kids);
	int nk = (HW_GET_BASE(t) == HW_TYPE_OBJECT) ? HW_GET_COUNT(t) : 0;

	for (i = 0; i < nk; i++) {
	    if (!kids[i]) continue;
	    kids[i]->modify(kids[i], hwStrCallback, HW_TYPE_CALLBACK,
		(void *)menuCallback);
	}
    }

    *wOut = wide * 11 + 30;
    if (*wOut < 190) *wOut = 190;
    *hOut = count * MENU_ITEM_H + 6;
    return panel;
}

static void loadMenus(void)
{
    static char *titles[6] = { "Vehicle", "Config", "Upright",
			       "Start Over", "Quit", "Help" };
    hwObject bar;
    int i;

    hwRegisterCallback("driveMenu", menuCallback);

    bar = hwFindObject("menuBar");
    if (!bar) return;

    /* The bar's buttons are anonymous in the layout file, so reach them
     * through the row/column's child list.
     */
    {
	hwObject *kids;
	hwInt32 t = bar->inquire(bar, hwStrChildren, (void **)&kids);
	int nkids = HW_GET_COUNT(t);

	if (HW_GET_BASE(t) != HW_TYPE_OBJECT) nkids = 0;
	if (nkids > 6) nkids = 6;
	for (i = 0; i < nkids; i++) {
	    menuTitle[i] = kids[i];
	    if (!kids[i]) continue;
	    kids[i]->modify(kids[i], hwStrCallback, HW_TYPE_CALLBACK,
		(void *)menuCallback);
	}
    }
    menuBar = bar;
    HW_MODIFY_1B(bar, hwStrInvisible, 0);
    (void)titles;

    menuPanel[1] = buildPanel("driveConfigMenu", configItems, ID_CONFIG,
	&menuPanelW[1], &menuPanelH[1]);
}

/* Put the bar along the very bottom, and the open pull-down above it. */
static void placeMenus(void)
{
    int i;

    if (menuBar) {
	HW_MODIFY_1I(menuBar, hwStrAlign,  0);
	HW_MODIFY_1I(menuBar, hwStrPosX,   0);
	HW_MODIFY_1I(menuBar, hwStrPosY,   menuBarY());
	HW_MODIFY_1I(menuBar, hwStrWidth,  winFullW);
	HW_MODIFY_1I(menuBar, hwStrHeight, MENU_BAR_H);
    }

    for (i = 0; i < 2; i++) {
	int x;
	if (!menuPanel[i]) continue;
	x = i * (winFullW / 6);
	if (x + menuPanelW[i] > winFullW) x = winFullW - menuPanelW[i];
	HW_MODIFY_1I(menuPanel[i], hwStrPosX,   x);
	HW_MODIFY_1I(menuPanel[i], hwStrPosY,   menuBarY() - menuPanelH[i]);
	HW_MODIFY_1I(menuPanel[i], hwStrWidth,  menuPanelW[i]);
	HW_MODIFY_1I(menuPanel[i], hwStrHeight, menuPanelH[i]);
    }
}

static void drawMenus(void)
{
    char buf[64];

    if (!menuBar) return;

    placeMenus();
    menuBar->draw(menuBar);

    if (openMenu >= 0 && openMenu < 2 && menuPanel[openMenu]) {
	menuPanel[openMenu]->draw(menuPanel[openMenu]);
    }

    /* Tick the level that is in effect. */
    if (openMenu == 1) {
	int x = winFullW/6;

	markConfig();
	disp->guiRectangle(disp, 0, 0xFF1A1F24, x,
	    menuBarY() - menuPanelH[1] - 24, menuPanelW[1], 24, 0);
	dashText(12, 0xFFFFD060, x + 8,
	    menuBarY() - menuPanelH[1] - 7, "DEPTH CUE");
    }
}

/*
 * Feed a window event to the menus.  Returns 1 if the menu swallowed it,
 * so the click does not also change gear.
 */
int dashMenuEvent(void *ev)
{
    hwWinEvent *event = (hwWinEvent *)ev;
    int y = event->pointer.y;
    int inBar, inPanel = 0;

    if (!menuBar) return 0;

    inBar = (y >= menuBarY());
    if (openMenu >= 0 && openMenu < 2 && menuPanel[openMenu]) {
	inPanel = (y >= menuBarY() - menuPanelH[openMenu]) && !inBar;
    }

    if (!inBar && !inPanel) {
	/* Click elsewhere closes an open menu, and is then passed on. */
	if (openMenu >= 0 && event->type == HW_INPUT_BUTTON_PRESS) {
	    openMenu = -1;
	}
	return 0;
    }

    if (inPanel) {
	menuPanel[openMenu]->modify(menuPanel[openMenu], hwStrEvent,
	    HW_TYPE_EVENT, event);
    }
    else {
	menuBar->modify(menuBar, hwStrEvent, HW_TYPE_EVENT, event);
    }
    return 1;
}

/****************************************************************************
 * Vehicle picker overlay.
 *
 * The X11 client had a Motif list widget for this; the GLFW build only had
 * the spinning model and undocumented keys.  This draws the list of
 * vehicles over the spinning preview and lets them be clicked.
 */
#define PICK_ROW_H	26
#define PICK_W		230
#define PICK_START_H	34

int dashPickerActive(void)
{
    return pickerMode;
}

static void pickerGeom(int *x, int *y, int *w, int *h)
{
    int rows = num_cars > 0 ? num_cars : 1;

    *w = PICK_W;
    *h = rows * PICK_ROW_H + PICK_START_H + 68;
    *x = 24;
    *y = (winFullH - *h) / 2;
    if (*y < 16) *y = 16;
}

void drawVehiclePicker(void)
{
    int x, y, w, h, i, rowY;
    int cur = -1;

    if (!disp || num_cars <= 0) return;

    pickerGeom(&x, &y, &w, &h);

    for (i = 0; i < num_cars; i++) {
	if (cstate.car_name && strcmp(driveables[i].name, cstate.car_name) == 0) {
	    cur = i;
	    break;
	}
    }

    disp->guiRectangle(disp, 0, 0xE8161B20, x, y, w, h, 6);
    dashText(12, 0xFFFFD060, x + 12, y + 20, "CHOOSE A VEHICLE");

    rowY = y + 30;
    for (i = 0; i < num_cars; i++) {
	if (i == cur) {
	    disp->guiRectangle(disp, 0, 0xFF3A4653,
		x + 8, rowY, w - 16, PICK_ROW_H - 2, 3);
	}
	dashText(18, (i == cur) ? 0xFFFFFFFF : 0xFFAAB4BE,
	    x + 18, rowY + PICK_ROW_H - 8, driveables[i].name);
	rowY += PICK_ROW_H;
    }

    rowY += 6;
    disp->guiRectangle(disp, 0, 0xFF2E7D32, x + 8, rowY, w - 16, PICK_START_H - 8, 4);
    dashText(18, 0xFFFFFFFF, x + 78, rowY + 20, "START");

    dashText(12, 0xFFB8C4CE, x + 12, y + h - 30,
	"click a name, or space / backspace");
    dashText(12, 0xFFB8C4CE, x + 12, y + h - 12,
	"r g b / R G B colour, Return starts");
}

/*
 * Hit test for the picker.  Returns a vehicle index, -2 for START, or -1.
 */
int dashPickerHit(int mx, int my)
{
    int x, y, w, h, i, rowY;

    if (num_cars <= 0) return -1;
    pickerGeom(&x, &y, &w, &h);
    if (mx < x || mx > x + w) return -1;

    rowY = y + 30;
    for (i = 0; i < num_cars; i++) {
	if (my >= rowY && my < rowY + PICK_ROW_H) return i;
	rowY += PICK_ROW_H;
    }
    rowY += 6;
    if (my >= rowY && my < rowY + PICK_START_H - 8) return -2;
    return -1;
}

/* Put a tick next to whichever fog level is currently in effect. */
static void markConfig(void)
{
    static int lastFog = -2, lastGear = -2;
    hwObject *kids;
    hwInt32 t;
    int nk, i, fog = 0, trans;

    if (!menuPanel[1]) return;

    for (i = 0; i < 5; i++) {
	if (driveFogAmount >= fogLevel[i] - 0.01) fog = i;
    }
    trans = (gear.type == GEAR_STANDARD) ? 6 : 5;
    if (fog == lastFog && trans == lastGear) return;
    lastFog = fog;
    lastGear = trans;

    t = menuPanel[1]->inquire(menuPanel[1], hwStrChildren, (void **)&kids);
    if (HW_GET_BASE(t) != HW_TYPE_OBJECT) return;
    nk = HW_GET_COUNT(t);
    if (nk > 7) nk = 7;

    for (i = 0; i < nk; i++) {
	char lbl[64];

	if (!kids[i]) continue;
	sprintf(lbl, "%c %s", (i == fog || i == trans) ? '*' : ' ',
	    configItems[i] + 2);
	kids[i]->modify(kids[i], hwStrLabel, HW_TYPE_STRING, lbl);
    }
}
