#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hw.h"

#ifndef M_PI
#       define M_PI     3.141592653589
#endif

#define WIDTH   800
#define HEIGHT  480


int finished = 0;
int doSync = 1;

typedef struct {
    int cx, cy;
    int numDigits;
    int numDecimal;
    hwInt32 fontSize;
    hwInt32 fgColor, bgColor;
    hwInt32 guiList;
} DigiGaugeInfo;

typedef struct {
    float cx, cy, w, h;
    const char *selections;
    hwInt32 fontSize;
    hwInt32 fgColor, bgColor;
} SelectorGaugeInfo;

DigiGaugeInfo odoGauge = {
    400, 410,                   /* cx, cy */
    6, 0,                       /* dig, dec */
    18,                         /* FontSize */
    0xFFFFFFFF, 0xFF000000,     /* fgColor, bgColor */
};
DigiGaugeInfo tripGauge = {
    400, 360,                   /* cx, cy */
    4, 1,                       /* dig, dec */
    18,                         /* FontSize */
    0xFFFFFFFF, 0xFF000000,     /* fgColor, bgColor */
};
SelectorGaugeInfo gears = {
    105, 375, 48, 48,           /* cx, cy, w, h */
    "RN12345",                  /* Selections */
    48,                         /* FontSize */
    0xFFFFFFFF, 0xFF000000,     /* fgColor, bgColor */
};

void drawDigiGauge(hwDisplay disp, DigiGaugeInfo *gauge, float value);
void drawSelector(hwDisplay disp, SelectorGaugeInfo *gauge, int value);
hwObject speedo, tach, myclock;

int minutes = 3*60+27, speed = 37, rpm = 6400;
float tripMiles = 123.45, odoMiles = 1289.8;
int gear = 0;

int winW = WIDTH, winH = HEIGHT;

hwDisplay disp;
hwDrawable draw;

void display(void)
{
    if( speedo ) {
        HW_MODIFY_1F(speedo, hwStrNeedlePos, speed);
        speedo->draw(speedo);
    }
    if( tach ) {
        HW_MODIFY_1F(tach, hwStrNeedlePos, rpm);
        tach->draw(tach);
    }
    if( myclock ) {
        HW_MODIFY_2F(myclock, hwStrNeedlePos, minutes, (minutes%60)*12);
        myclock->draw(myclock);
    }

    drawDigiGauge(disp, &odoGauge, odoMiles);
    drawDigiGauge(disp, &tripGauge, tripMiles);
    drawSelector(disp, &gears, gear);

    disp->update(disp, HW_UPDATE_ALL);
}

void drawDigiGauge(hwDisplay disp, DigiGaugeInfo *gauge, float value)
{
    int i, n, width, maxAsc, maxDesc;
    int real, frac;
    char sReal[16], sFrac[8], str[2];
    int pow;
    float x, y, tx, ty, avgWidth, avgHeight;
    char zero[] = "0";
    TexFont *txf;

    pow = 1;
    for (i = 0; i < gauge->numDecimal; i++) {
        pow *= 10;
    }
    real = (int) value;
    frac = (int) (value * pow);
    frac -= real*pow;

    n = gauge->numDigits;
    for (i = 0; i < n; i++) {
        sReal[n-i-1] = '0' + (real % 10);
        real = real / 10;
    }
    sReal[i] = 0;

    n = gauge->numDecimal;
    for (i = 0; i < n; i++) {
        sFrac[n-i-1] = '0' + (frac % 10);
        frac = frac / 10;
    }
    sFrac[i] = 0;

    txf = txfLoadStaticFont(gauge->fontSize);

    txfGetStringMetrics(txf, zero, 1, &width, &maxAsc, &maxDesc);

    n = (gauge->numDigits + gauge->numDecimal + (gauge->numDecimal ? 1 : 0));

    avgWidth = (float)(width + 4);
    //avgHeight = (float)(maxAsc + 2); /* Numbers only ascend */
    avgHeight = gauge->fontSize;

    x = gauge->cx - n * avgWidth / 2;
    y = gauge->cy + maxAsc / 2;

    str[1] = 0;
    for (i = 0; i < gauge->numDigits; i++) {
        str[0] = sReal[i];
        txfGetStringMetrics(txf, sReal+i, 1, &width, &maxAsc, &maxDesc);
        tx = x + avgWidth/2 - width/2;
        ty = y;

        if (gauge->bgColor) {
            disp->guiRectangle(disp, 0, gauge->bgColor,
                           (int)x, (int)y - gauge->fontSize + 1,
                           avgWidth - 2,
                           gauge->fontSize + 2,
                           0);
        }
        disp->guiText(disp, txf, gauge->fgColor,
                      HW_TEXT_ALIGN_LEFT,
                      HW_TEXT_ALIGN_BOTTOM,
                      gauge->fontSize,
                      (int)tx, (int)ty,
                      str);

        x += avgWidth;
    }

    if (gauge->numDecimal) {
        txfGetStringMetrics(txf, ".", 1, &width, &maxAsc, &maxDesc);

        tx = x + avgWidth/2 - width/2;
        ty = y;

        disp->guiText(disp, txf, gauge->fgColor,
                      HW_TEXT_ALIGN_LEFT,
                      HW_TEXT_ALIGN_BOTTOM,
                      gauge->fontSize,
                      (int)tx, (int)ty,
                      ".");

        x += avgWidth;

        for (i = 0; i < gauge->numDecimal; i++) {
            str[0] = sFrac[i];
            txfGetStringMetrics(txf, zero, 1, &width, &maxAsc, &maxDesc);
            tx = x + avgWidth/2 - width/2;
            ty = y;

            if (gauge->bgColor) {
                disp->guiRectangle(disp, 0, gauge->bgColor,
                               (int)x, (int)y - gauge->fontSize + 1,
                               avgWidth - 2,
                               avgHeight + 2,
                               0);
            }
            disp->guiText(disp, txf, gauge->fgColor,
                          HW_TEXT_ALIGN_LEFT,
                          HW_TEXT_ALIGN_BOTTOM,
                          gauge->fontSize,
                          (int)tx, (int)ty,
                          str);

            x += avgWidth;
        }
    }
}

void drawSelector(hwDisplay disp, SelectorGaugeInfo *gauge, int value)
{
    int n, width, maxAsc, maxDesc;
    char str[2];
    float x, y;
    char zero[] = "0";
    TexFont *txf;
    hwInt32 box[10];

    if (gauge->bgColor) {
        disp->guiRectangle(disp, 0, gauge->bgColor,
                       (int)(gauge->cx - gauge->w / 2),
                       (int)(gauge->cy - gauge->h / 2),
                       gauge->w, gauge->h,
                       0);
    }
    box[0] = (int)(gauge->cx - gauge->w / 2);
    box[1] = (int)(gauge->cy - gauge->h / 2);
    box[2] = (int)(gauge->cx + gauge->w / 2);
    box[3] = (int)(gauge->cy - gauge->h / 2);
    box[4] = (int)(gauge->cx + gauge->w / 2);
    box[5] = (int)(gauge->cy + gauge->h / 2);
    box[6] = (int)(gauge->cx - gauge->w / 2);
    box[7] = (int)(gauge->cy + gauge->h / 2);
    box[8] = (int)(gauge->cx - gauge->w / 2);
    box[9] = (int)(gauge->cy - gauge->h / 2);
    disp->guiPolyline(disp, 0, gauge->fgColor, 5, box);

    txf = txfLoadStaticFont(gauge->fontSize);

    n = strlen(gauge->selections);
    if (value < 0) value = 0;
    if (value >= (n-1)) value = n-1;

    str[0] = gauge->selections[value];
    str[1] = 0;

    txfGetStringMetrics(txf, str, 1, &width, &maxAsc, &maxDesc);

    x = gauge->cx - width / 2;
    y = gauge->cy + maxAsc / 2;

    disp->guiText(disp, txf, gauge->fgColor,
                      HW_TEXT_ALIGN_LEFT,
                      HW_TEXT_ALIGN_BOTTOM,
                      gauge->fontSize,
                      (int)x, (int)y,
                      str);

}

void key(unsigned char key)
{
    int i;
    switch (key) {
    case 'g' : if (gear > 0) gear--; break;
    case 'G' : if (gear < 6) gear++; break;
    case 'h' : minutes = (minutes + 60) % (12*60); break;
    case 'H' : minutes = (minutes + 12*60 - 60) % (12*60); break;
    case 'm' : minutes = (minutes + 3) % (12*60); break;
    case 'M' : minutes = (minutes + 12*60 - 4) % (12*60); break;
    case 's' : speed += 3; break;
    case 'S' : speed -= 4; break;
    case 't' : rpm += 127; break;
    case 'T' : rpm -= 96; break;
    case 'o' : case 'O':
        tripMiles += 0.1;
        odoMiles += 0.1;
        break;
    case 'r' : case 'R' :
        tripMiles = 0.0;
        break;
    case '\033' :
        exit(1);
        break;
    }
}

void callback(hwDrawable draw, hwWinEvent *event)
{
    switch (event->type) {
    case HW_INPUT_KEYBOARD :
        key(event->keyboard.key);
        break;
    case HW_INPUT_CONFIG :
        winW = event->config.width;
        winH = event->config.height;
        disp->viewport(disp, 0, 0, winW, winH);
        break;
    }
}

void makeGauges(void)
{
    static char *gaugeSpecs[] = {
        "hwGauge speedo {",
            "PosX = 180",
            "PosY = 20",
            "Width = 440",
            "Height = 440",
            "Scale = 2.3",
            "LonRange = { 225., -45. }",
            "GaugeRange = { 0, 120, 10 }",
            "Divisions = { 12, 2, 10 }",
            "FontHeight = 11",
            "Label = \"MPH\"",
            "Color = 0xFFFFFFFF",
            "BackgroundColor = 0x80000000",
            "SpotColor = 0xFF808080",
            "NeedleColor = 0xFFFF0000",
            "NeedleLength = 75.",
            "OptFlags = HW_OPT_USE_DL",
        "}",
        "hwGauge tach {",
            "PosX = 605",
            "PosY = 15",
            "Width = 180",
            "Height = 180",
            "Scale = 3.0",
            "LonRange = { 225., -45. }",
            "GaugeRange = { 0, 12000, 2 }",
            "Divisions = { 6, 2, 4 }",
            "FontHeight = 20",
            // TBD: Get second label somehow "x1000 */
            "Label = \"RPM\"",
            "Color = 0xFFFFFFFF",
            "BackgroundColor = 0x80000000",
            "SpotColor = 0xFF808080",
            "NeedleColor = 0xFFFF0000",
            "NeedleLength = 75.",
            "OptFlags = HW_OPT_USE_DL",
        "}",
        "hwGauge myclock {",
            "PosX = 15",
            "PosY = 15",
            "Width = 180",
            "Height = 180 ",
            "Scale = 3.0",
            "LonRange = { 90., -270. }",
            "GaugeRange = { 0, (12*60), 0 }",
            "Divisions = { 12, 5, 0 }",
            "FontHeight = 20",
            "Color = 0xFFFFFFFF",
            "BackgroundColor = 0x80000000",
            "SpotColor = 0xFF808080",
            "NeedleColor = 0xFFFF0000",
            "NeedleLength = { 35., 75. }",
            "OptFlags = HW_OPT_USE_DL",
        "}",
        NULL
    };
    hwInt32
        nObjs;
    hwObject
        *objs;

    nObjs = hwParseArray( gaugeSpecs, &objs );
    if( nObjs != 3 ) {
        exit(1);
    }
    speedo = hwFindObject( "speedo" );
    tach = hwFindObject( "tach" );
    myclock = hwFindObject( "myclock" );
}

int main(int argc, char **argv)
{
    float grey[3] = {0.5, 0.5, 0.5};

    if (!hwInit(argc, argv)) exit(1);

    disp = hwDefaultDisplay->create(hwDefaultDisplay, NULL, NULL);
    if (!disp) exit(1);

    if (!disp->chooseVisual(disp, HW_VIS_DBUFF, 0)) exit(1);
    draw = disp->createWindow(disp, "Gauge", 100, 100, WIDTH, HEIGHT,
                              HW_WIN_INPUT);
    if (!draw) exit(1);
    
    disp->makeCurrent(disp, draw);

    disp->backgroundColor(disp, grey);
    disp->update(disp, HW_UPDATE_ALL);

    makeGauges();

    disp->inputHandler( disp, callback );

    while (1) {
        display();
    }
}
