/* (c) Copyright Hewlett-Packard Company 2001
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hw.h"

#ifdef WIN32
#include <windows.h>
#endif

#ifdef MAC
#include <OpenGL/OpenGl.h>
#endif

#ifdef WIN_GL
#include <GL/gl.h>
#endif

#ifdef GLFW
#include "GLFW/glfw3.h"
#endif

#if 0 && defined(ANDROID)
#  include <android/log.h>
#  define ANDROID_LOG(a) __android_log_print a
#else
#  define ANDROID_LOG(a)
#endif

#if !defined(ANDROID)

#include "../../JPEG/jpeglib.h"
#include "../../LIBPNG/writepng.h"

#endif

#define MAX_FAST        32.0
#define MIN_SLOW        (1.0 / 16.0)
#define RESET_TIME      60.0

typedef struct {
    // Configuration
    int showFPS;        // Show FPS
    int doDump;         // Dump image sequence
    int doExit;         // Exit after given time
    char *dumpFmt;      // printf format string for image names
    int fixedStep;      // Vs. dynamic step
    int winWidth;       // Current window
    int winHeight;      //   size
    hwFloat *chapters;  // CHAPTERS contents from input file
    hwInt32 numChapters; // Number of chapters
    hwObject *objects;  // Objects read from input file
    int numObjects;     // Number of objects
    int dumpJpeg;       // Otherwise, dump PNG

    // Time / frame variables
    int frameNum;       // Current frame number, for image dumping
    int numFrames;      // Frame number, for FPS tracking
    double prevTime;    // (RT) Previous time stamp
    double saveTime;    // (RT) Base time for FPS tracking
    double currTime;    // (RT) Current time stamp
    double keyPressTime; // (RT) When was the last keypress?
    double lastFrameTime; // (RT) How much time elapsed?
    double stepTime;    // (VT) Step time, if fixedStep
    double elapsedTime; // (VT) Current elapsed time
    double timeMag;     // (VT) From +/- MIN_SLOW to MAX_FAST, inclusive
    double totalTime;   // (VT) If CHAPTERS is present
    double exitTime;    // (VT) Time to exit, if doExit is true

    // Command / feedback variables
    int keyPressed;     // Key press countdown happening?
    int resized;        // Were we resized?
    int reset;          // Was a reset requested?
    double timeSkip;    // Skip some amount of time

    // Joystick state
    float hatX, hatY;   // Position of the D-pad "hat"
} WelcomeState;

static WelcomeState
    wss, *ws = &wss;
hwDisplay
    welcomeDisp;
hwDrawable
    welcomeDraw;

void welcomeEvent( hwDrawable draw, hwWinEvent *event );
static void recurseDraw( hwObject obj );
static void recurseProp( hwObject obj, const char *prop, hwInt32 val );

#if !defined(ANDROID)
static void dumpScreen( char *dumpFmt );
#endif

void welcomeRender(void);
void usage(char *progName);

#if defined(ANDROID)
int welcomeMain( int argc, char **argv )
#else
int main( int argc, char **argv )
#endif
{
    hwObject
        chapterObj;
    float
        timeOffset = 0.0;
    int
        aaMode = 0,
        wireframe = -1,
        sampVert = -1,
        renderMode = 0,
        optFlags = 0,
        optFlagsSet = 0,
        fullScreen = 0,
        joystick = 0,
        preWarm = 0,
        winX = 0,
        winY = 0,
        i;
    hwInt32
        typ,
        hideCursor = 0,
        visFlags;
    char
        *filename = NULL,
        buff[1024];

    /* Initialize state */
    ws->winWidth = 640;
    ws->winHeight = 512;
    ws->reset = 0;
    ws->chapters = NULL;
    ws->numChapters = 0;
    ws->showFPS = 0;
    ws->numFrames = 0;
    ws->fixedStep = 0;
    ws->doDump = 0;
    ws->dumpJpeg = 0;
    ws->dumpFmt = NULL;
    ws->doExit = 0;
    ws->timeMag = 1.0;
    ws->hatX = ws->hatY = 0.0;
    ws->keyPressed = 0;

    /* Parse command-line argument list */
    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-fps" ) == 0 ) {
            ws->showFPS = 1;
        }
        else if( strcmp( argv[i], "-opt" ) == 0 ) {
            if( i >= (argc-1) ) usage(argv[0]);
            optFlags = atoi( argv[i+1] );
            optFlagsSet = 1;
            i += 1;
        }
        else if( strcmp( argv[i], "-wire" ) == 0 ) {
            wireframe = HW_TRUE;
        }
        else if( strcmp( argv[i], "-rgb" ) == 0 ) {
            sampVert = HW_TRUE;
        }
        else if( strcmp( argv[i], "-win" ) == 0 ) {
            if( i >= (argc-4) ) usage(argv[0]);
            winX = atoi(argv[i+1]);
            winY = atoi(argv[i+2]);
            ws->winWidth = atoi(argv[i+3]);
            ws->winHeight = atoi(argv[i+4]);
            i += 4;
        }
        else if( strcmp( argv[i], "-prewarm" ) == 0 ) {
            preWarm = 1;
        }
        else if( strcmp( argv[i], "-aa" ) == 0 ) {
            aaMode = 1;
        }
        else if( strcmp( argv[i], "-aa4" ) == 0 ) {
            aaMode = 2;
        }
        else if( strcmp( argv[i], "-aa8" ) == 0 ) {
            aaMode = 3;
        }
        else if( strcmp( argv[i], "-frame" ) == 0 ) {
            if( i >= (argc-1) ) usage(argv[0]);
            ws->frameNum = atoi(argv[i+1]);
            i += 1;
        }
        else if( strcmp( argv[i], "-step" ) == 0 ) {
            if( i >= (argc-1) ) usage(argv[0]);
            ws->fixedStep = 1;
            ws->stepTime = atof(argv[i+1]);
            i += 1;
        }
        else if( strcmp( argv[i], "-exit" ) == 0 ) {
            if( i >= (argc-1) ) usage(argv[0]);
            ws->doExit = 1;
            ws->exitTime = atof(argv[i+1]);
            i += 1;
        }
        else if( strcmp( argv[i], "-dump" ) == 0 ) {
            if( i >= (argc-1) ) usage(argv[0]);
            ws->doDump = 1;
            ws->dumpFmt = argv[i+1];
            i += 1;
        }
        else if( strcmp( argv[i], "-jpeg") == 0 ) {
            if( i >= (argc-1) ) usage(argv[0]);
            ws->dumpJpeg = atoi(argv[i+1]);
            i += 1;
        }
        else if( strcmp( argv[i], "-full" ) == 0 ) {
            ws->winWidth = 1280;
            ws->winHeight = 1024;
            winX = (1280 - ws->winWidth) / 2;
            winY = (1024 - ws->winHeight) / 2;
        }
        else if( strcmp( argv[i], "-timeoffset" ) == 0 ) {
            if( i >= (argc-1) ) usage(argv[0]);
            timeOffset = atof( argv[i+1] );
            i += 1;
        }
        else if( strcmp( argv[i], "-fullscreen" ) == 0 ) {
            fullScreen = HW_WIN_FULLSCREEN;
        }
        else if( strcmp( argv[i], "-joystick" ) == 0 ) {
            joystick = HW_WIN_JOYSTICK;
        }
        else if( strcmp( argv[i], "-hidecursor" ) == 0 ) {
            hideCursor = HW_WIN_HIDE_CURSOR;
        }
        else if( *argv[i] != '-' ) {
            if( filename ) usage(argv[0]);
            filename = argv[i];
        }
        else {
            usage(argv[0]);
        }
    }
    if( !filename )     usage(argv[0]);

    /* Initialize HoverWare, and choose the visual */
    if (!hwInit(argc, argv)) exit(1);
    welcomeDisp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );
    if( !welcomeDisp ) exit( 1 );

    visFlags = HW_VIS_DBUFF | HW_VIS_DEPTH;
    switch( aaMode ) {
    case 1 : visFlags |= HW_VIS_AA;     break;
    case 2 : visFlags |= HW_VIS_AA_MED; break;
    case 3 : visFlags |= HW_VIS_AA_HI;  break;
    }

    if( !welcomeDisp->chooseVisual( welcomeDisp, visFlags, 0 ) ) {
        exit( 1 );
    }

    /* Create a simple window of specified size, and get the
     * real X window ID from the HoverWare drawable
     */
    if( winX < 0 ) winX = 0;
    if( winY < 0 ) winY = 0;

    welcomeDraw = welcomeDisp->createWindow( welcomeDisp, "Welcome",
                        winX, winY,
                        ws->winWidth, ws->winHeight,
                        HW_WIN_INPUT | fullScreen | hideCursor | joystick );

    if( !welcomeDraw ) exit( 1 );

    /* Make the created drawable current */
    /* This needs to happen before most HW calls */
    welcomeDisp->makeCurrent( welcomeDisp, welcomeDraw );

    /* Parse the given file */
    ws->numObjects = hwParseFile( filename, &ws->objects );
    if( !ws->numObjects )   exit( 1 );

    /* Set up attributes as we like them: WireFrame, SampVert, and OptLevel */
    for( i = 0; i < ws->numObjects; i++ ) {
        if( sampVert >= 0 ) {
            recurseProp(ws->objects[i], hwStrSampVert, sampVert);
        }
        if( optFlagsSet ) {
            recurseProp(ws->objects[i], hwStrOptFlags, optFlags);
        }
    }

    welcomeDisp->inputHandler( welcomeDisp, welcomeEvent );

    if( wireframe >= 0 ) {
        renderMode = HW_RENDER_DEFAULT | HW_RENDER_EDGE_MODE;
        renderMode &= ~HW_RENDER_CULL_FACE;
    }
    else {
        renderMode = HW_RENDER_DEFAULT;
    }
    welcomeDisp->renderMode( welcomeDisp, renderMode );

    if( preWarm ) {
        for( i = 0; i < ws->numObjects; i++ ) {
            recurseDraw( ws->objects[i] );
        }
        welcomeDisp->update( welcomeDisp, HW_UPDATE_ALL & ~HW_UPDATE_TIME );
    }

    /* See if there are chapter divisions */
    chapterObj = hwFindObject("CHAPTERS");
    if (chapterObj) {
        typ = chapterObj->inquire(chapterObj, hwStrData,
                                  (void **)&ws->chapters);
        if (HW_GET_BASE(typ) == HW_TYPE_FLOAT) {
            ws->numChapters = HW_GET_COUNT(typ);
            ws->totalTime = ws->chapters[ws->numChapters-1];
        }
        else {
            ws->chapters = NULL;
        }
    }

    // Establish the start time
    ws->currTime = hwGetSysTime();
    ws->saveTime = ws->prevTime = ws->currTime;

    ws->elapsedTime = timeOffset;
    welcomeDisp->setStartTime(welcomeDisp, 0.0);
    welcomeDisp->setCurrTime(welcomeDisp, ws->elapsedTime);
    welcomeDisp->setElapsedTime(welcomeDisp, ws->elapsedTime);

#if !defined(ANDROID)
    /* Main loop - just draw all of the time */
    while( 1 ) {
        welcomeRender();
    }
#endif
}

static const char *usageMsg[] = {
    "Usage: %s file.hw [opt-args] # where opt-args can include:\n",
    "    -fps           # Track the FPS on stdout\n",
    "    -opt optlevel  # Set the default optimization level\n",
    "    -wire          # Force wireframe\n",
    "    -rgb           # Force RGB-per-vertex rather than textures\n",
    "    -win x y w h   # Define window position and size\n",
    "    -prewarm       # Preload all files and textures before beginning\n",
    "    -aa            # Draw with 2x multisample\n",
    "    -aa4           # Draw with 4x multisample\n",
    "    -aa8           # Draw with 8x multisample\n",
    "    -frame frameno # Start rendering at a given frame number\n",
    "    -step t        # Step \"t\" seconds per frame instead of real-time\n",
    "    -exit frameno  # Stop rendering and exit after a given frame number\n",
    "    -dump fmtstr   # Dump each frame as a PNG - the fmtstr is a printf-\n",
    "                   # style string intended to format the frame number\n",
    "                   # as part of the filename - e.g. FRAME%03d.png\n",
    "    -jpeg quality  # Dump JPEG instead of PNG, with the given quality\n",
    "                   # level from 1 to 100\n",
    "    -timeoffset t  # Start at time \"t\" seconds rather than 0\n",
    "    -fullscreen    # Run in a full-screen borderless window\n",
    "    -joystick      # Run with joystick support\n",
    "    -hidecursor    # Hide the mouse cursor\n",
    "\n",
    "While running, the following keyboard commands are available:\n",
    "    <space>        Pause the animation\n",
    "    R              Reset the animation\n",
    "    > or .         Go to the next chapter (defined in an hwData\n",
    "                   objecte named CHAPTERS - each element in its\n",
    "                   Data array is a chapter begin time in seconds).\n",
    "                   If no chapters are defined, skips forward 60\n",
    "                   seconds.\n",
    "    < or ,         Go to the previous chapter.  If no chapters\n",
    "                   are defined, skips backwards 60 seconds.  If\n",
    "                   the current chapter is 5 or more seconds in\n",
    "                   progress, skip back to the beginning of\n",
    "                   the current chapter.\n",
    "    + or =         Doubles the speed of playback\n",
    "    - or _         Halves the speed of playback\n",
    "    / or ?         Reverses the direction of time\n",
    "    Q              Exits the program\n",
    "\n",
    "If joystick support is enabled, the following joystick commands\n",
    "are available:\n",
    "    D-pad left     Halves the speed of playback\n",
    "    D-pad right    Doubles the speed of playback\n",
    "    D-pad up       Go to the next chapter\n",
    "    D-pad down     Go to the previous chapter\n",
    "    A button       Pause the animation\n",
    "    B button       Reverse the direction of time\n",
    "    X button       Exit the program\n",
    "    Y button       Reset the animation\n",
    NULL
};

void usage(char *progName)
{
    int i;
    fprintf(stderr, usageMsg[0], progName);
    for (i = 1; usageMsg[i]; i++) {
        fprintf(stderr, "%s", usageMsg[i]);
    }
    exit(1);
}

void welcomeRender(void)
{
    int
        tempReset,
        i;

#if defined(ANDROID)
    welcomeDisp->makeCurrent(welcomeDisp, welcomeDraw);
    welcomeDisp->update( welcomeDisp, HW_UPDATE_MATRIX | HW_UPDATE_CLEAR );
#endif

    tempReset = 0;
    if(ws->keyPressed && ((ws->currTime - ws->keyPressTime) > RESET_TIME)) {
        ws->keyPressed = 0;
        tempReset = 1;
    }

    // Reset time
    if(ws->reset || tempReset) {
        if (ws->reset) {
            ws->elapsedTime = 0.0;
            welcomeDisp->setCurrTime(welcomeDisp, 0.0);
            welcomeDisp->setElapsedTime(welcomeDisp, 0.0);
        }
        ws->timeMag = 1.0;
        ws->timeSkip = 0.0;
        ws->reset = 0;
    }

    /* Draw all of the objects we read */
    for( i = 0; i < ws->numObjects; i++ ) {
        ws->objects[i]->draw( ws->objects[i] );
    }

    {
        int tics;

        tics = fmod( welcomeDisp->getElapsedTime(welcomeDisp), 2.0)  * 16;
        welcomeDisp->setVisibility( welcomeDisp, 1 << tics );
    }

    ws->currTime = hwGetSysTime();

    // Perform frames-per-second report
    if( ws->showFPS ) {
        ws->numFrames++;
        if( ws->numFrames == 10 ) {
            (void)printf( "FPS: %f\r",
                          ws->numFrames / (ws->currTime - ws->saveTime) );
            (void)fflush( stdout );
            ws->saveTime = ws->currTime;
            ws->numFrames = 0;
        }
    }

    // Dump images
    if (ws->doDump ) {
#if !defined(ANDROID)
        welcomeDisp->update( welcomeDisp,
                             HW_UPDATE_ALL
                                & ~(HW_UPDATE_SWAP | HW_UPDATE_CLEAR
                                    | HW_UPDATE_TIME) );
        dumpScreen(ws->dumpFmt);
        welcomeDisp->update( welcomeDisp, HW_UPDATE_SWAP|HW_UPDATE_CLEAR );
#endif
    }
    else {
        /* Do the flush and double-buffer switch */
#if defined(ANDROID)
        welcomeDisp->update( welcomeDisp,
                             HW_UPDATE_ALL
                                & ~(HW_UPDATE_SWAP | HW_UPDATE_CLEAR
                                    | HW_UPDATE_TIME) );
#else
        welcomeDisp->update( welcomeDisp, HW_UPDATE_ALL & ~HW_UPDATE_TIME );
#endif
    }

    /* Update the window */
    if( ws->resized ) {
        welcomeDisp->viewport( welcomeDisp, 0, 0, ws->winWidth, ws->winHeight );
        ws->resized = 0;
    }

    /* Update timers */
    ws->lastFrameTime = ws->currTime - ws->prevTime;
    ws->prevTime = ws->currTime;
    if (ws->fixedStep) {
        ws->lastFrameTime = ws->stepTime;
    }

    if (ws->timeSkip != 0.0) {
        ws->elapsedTime += ws->timeSkip;
        ws->timeSkip = 0.0;
    }
    else {
        ws->elapsedTime += ws->lastFrameTime * ws->timeMag;
    }
    if (ws->elapsedTime < 0.0) {
        ws->elapsedTime += ws->totalTime;
    }
    if (ws->elapsedTime > ws->totalTime) {
        ws->elapsedTime -= ws->totalTime;
    }
    if (ws->doExit && (ws->elapsedTime >= ws->exitTime)) {
        exit(0);
    }

    welcomeDisp->setCurrTime(welcomeDisp, ws->elapsedTime);
    welcomeDisp->setElapsedTime(welcomeDisp, ws->elapsedTime);
}

static void recurseProp( hwObject obj, const char *prop, hwInt32 val )
{
    hwInt32
        isGroup,
        i, n,
        type;
    void
        *pVoid;
    hwObject
        *pObj;

    if( obj->parent == hwGroup ) {
        isGroup = 1;
    }
    else if( obj->parent == hwSpinner ) {
        isGroup = 1;
    }
    else if( obj->parent == hwTimer ) {
        isGroup = 1;
    }
    else if( obj->parent == hwFile ) {
        isGroup = 1;
    }
    else {
        isGroup = 0;
    }

    if( isGroup ) {
        type = obj->inquire( obj, hwStrChildren, &pVoid );

        if( HW_GET_BASE(type) == HW_TYPE_OBJECT )  {
            n = HW_GET_COUNT( type );
            pObj = pVoid;
            for( i = 0; i < n; i++ ) {
                recurseProp( pObj[i], prop, val );
            }
        }
    }
    else {
        HW_MODIFY_1I(obj, prop, val);
    }
}

static void recurseDraw( hwObject obj )
{
    hwInt32
        i, n,
        type;
    void
        *pVoid;
    hwObject
        *pObj;

    obj->draw( obj );
    type = obj->inquire( obj, hwStrChildren, &pVoid );

    if( HW_GET_BASE(type) == HW_TYPE_OBJECT )  {
        n = HW_GET_COUNT( type );
        pObj = pVoid;
        for( i = 0; i < n; i++ ) {
            recurseDraw( pObj[i] );
        }
    }
}

#if defined(MAC) || defined(WIN_GL) || defined(GLFW)
static void writeJpeg(char * filename, int quality,
                      unsigned char *img, int w, int h)
{
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    FILE *outfile;
    int i, row_stride;
    JSAMPROW row_pointer[1];

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    outfile = fopen(filename, "wb");
    if (!outfile) {
        fprintf(stderr, "can't open %s\n", filename);
        exit(1);
    }
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = w;
    cinfo.image_height = h;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);

    jpeg_start_compress(&cinfo, TRUE);

    row_stride = w * 3; /* JSAMPLEs per row in image_buffer */

    /* Start at end of image (ReadPixels reads upside-down :-) */
    img = img + (h - 1)*row_stride;

    for (i = 0; i < h; i++) {
        row_pointer[0] = (JSAMPROW) img;
        (void)jpeg_write_scanlines(&cinfo, row_pointer, 1);
        img -= row_stride;
    }

    jpeg_finish_compress(&cinfo);
    fclose(outfile);

    jpeg_destroy_compress(&cinfo);
}
#endif

#if !defined(ANDROID)
static void dumpScreen( char *dumpFmt )
{
#if defined(MAC) || defined(WIN_GL) || defined(GLFW)
    PngOutStruct pOut;
    char fileName[1024];
    static unsigned char *pix;
    static int pixW, pixH;
    int i, n;

    if (!pix || ((pixW*pixH) < (ws->winWidth*ws->winHeight))) {
        pixW = ws->winWidth; pixH = ws->winHeight;
        pix = realloc(pix, ws->winWidth*ws->winHeight*3);
        if (!pix) {
            fprintf(stderr, "Out of memory\n");
            exit(1);
        }
    }

    glReadBuffer(GL_BACK);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glFinish();

    sprintf(fileName, dumpFmt, ws->frameNum);
    ws->frameNum++;
printf("Writing %s\n", fileName);

    if (ws->dumpJpeg) {
        glReadPixels(0, 0, ws->winWidth, ws->winHeight,
                 GL_RGB, GL_UNSIGNED_BYTE, pix);
        writeJpeg(fileName, ws->dumpJpeg, pix, ws->winWidth, ws->winHeight);
    }
    else {
        for (i = 0; i < ws->winHeight; i++) {
            glReadPixels(0,ws->winHeight-i-1, ws->winWidth,1,
                         GL_RGB, GL_UNSIGNED_BYTE, pix+i*ws->winWidth*3);
        }

        if (!PngInitStdOut(&pOut, fileName)) return;
        n = WritePNG(&pOut, 0, ws->winWidth, ws->winHeight, 3, 8, pix, NULL);
        if (n < 0) {
            fprintf(stderr, "Error at line %d writing %s\n", -1, fileName);
            exit(1);
        }
        PngTermStdOut(&pOut);
    }
#endif
}
#endif

static int getChapter( hwDisplay disp, double *deltaT )
{
    double currTime;
    int i;

    if (!ws->chapters) return -1;

    currTime = fmod(welcomeDisp->getElapsedTime(disp), ws->totalTime);

    for (i = 0; currTime > ws->chapters[i]; i++) {
        /* NOTHING */
    }

    *deltaT = currTime - ws->chapters[i-1];
    return i-1;
}

static void handleKey( hwDisplay disp, int key )
{
    int chap, previous;
    double prevChap, currChap, nextChap, deltaT;
    double currTime;

    ws->keyPressed = 1;
    ws->keyPressTime = ws->currTime;

    switch (key) {
    case ' ' : 
        ws->timeMag = (ws->timeMag == 0.0) ? 1.0 : 0.0;
        break;
    case 'r' : case 'R' :
        ws->reset = 1;
        break;
    case '.' : case '>' :
        chap = getChapter(disp, &deltaT);
        if (chap < 0) {
            // No chapters
            ws->timeSkip = 60.0;
            break;
        }

        currChap = ws->chapters[chap];
        nextChap = ws->chapters[chap+1];
        ws->timeSkip = (nextChap - currChap) - deltaT;
        break;
    case ',' : case '<' :
        chap = getChapter(disp, &deltaT);
        if (chap < 0) {
            // No chapters
            ws->timeSkip = -60.0;
            break;
        }

        currTime = ws->chapters[chap] + deltaT;

        if (deltaT > 5.0) {
            // Skip to beginning of current chapter
            previous = chap;
        }
        else {
            // Skip to previous chapter - handle wraparaound
            previous = (chap + ws->numChapters - 1) % ws->numChapters;
            if (previous == (ws->numChapters-1)) {
                previous = ws->numChapters-2;
            }
        }

        prevChap = ws->chapters[previous];
        ws->timeSkip = prevChap - currTime;
        break;
    case '=': case '+':
        if (ws->timeMag == 0.0) {
            ws->timeSkip = (1.0 / 60.0);
        }
        else if ((ws->timeMag > -MAX_FAST) && (ws->timeMag < MAX_FAST)) {
            ws->timeMag *= 2.0;
        }
        break;
    case '_' : case '-':
        if (ws->timeMag == 0.0) {
            ws->timeSkip = -(1.0 / 60.0);
        }
        else if ((ws->timeMag < -MIN_SLOW) || (ws->timeMag > MIN_SLOW)) {
            ws->timeMag *= 0.5;
        }
        break;
    case '/' : case '?' :
        ws->timeMag *= -1.0;
        break;
    case 'q' : case 'Q' :
        exit( 0 );
        break;
    }
}

static void handleJoy( hwDisplay disp, hwWinEvent *event )
{
    switch (event->joystickButton.value) {
    case HW_JOY_DPAD_LEFT :  handleKey(disp, '-'); break;
    case HW_JOY_DPAD_RIGHT : handleKey(disp, '+'); break;
    case HW_JOY_DPAD_UP :    handleKey(disp, '>'); break;
    case HW_JOY_DPAD_DOWN :  handleKey(disp, '<'); break;
    case HW_JOY_A :          handleKey(disp, ' '); break;
    case HW_JOY_B :          handleKey(disp, '/'); break;
    case HW_JOY_X :          handleKey(disp, 'q'); break;
    case HW_JOY_Y :          handleKey(disp, 'r'); break;
    }
}

static void handleAxis( hwDisplay disp, hwWinEvent *event )
{
    float value = event->joystickAxis.value;
    hwWinEvent e;

    e.type = HW_INPUT_JOY_PRESS;
    e.joystickButton.joystickNum = event->joystickAxis.joystickNum;

    switch (event->joystickAxis.axis) {
    case HW_AXIS_HAT_X :
        if (value < -0.5) {
            if (ws->hatX >= -0.5) {
                e.joystickButton.value = HW_JOY_DPAD_LEFT;
                handleJoy(disp, &e);
            }
        }
        else if (value > 0.5) {
            if (ws->hatX <= 0.5) {
                e.joystickButton.value = HW_JOY_DPAD_RIGHT;
                handleJoy(disp, &e);
            }
        }
        ws->hatX = value;
        break;
    case HW_AXIS_HAT_Y :
        if (value < -0.5) {
            if (ws->hatY >= -0.5) {
                e.joystickButton.value = HW_JOY_DPAD_UP;
                handleJoy(disp, &e);
            }
        }
        else if (value > 0.5) {
            if (ws->hatY <= 0.5) {
                e.joystickButton.value = HW_JOY_DPAD_DOWN;
                handleJoy(disp, &e);
            }
        }
        ws->hatY = value;
        break;
    }
}

void welcomeEvent( hwDrawable draw, hwWinEvent *event )
{
    switch( event->type ) {
    case HW_INPUT_CONFIG :
        ws->winWidth = event->config.width;
        ws->winHeight = event->config.height;
        ws->resized = 1;
        break;
    case HW_INPUT_KEYBOARD :
        handleKey(welcomeDisp, event->keyboard.key);
        break;
    case HW_INPUT_JOY_PRESS :
        handleJoy(welcomeDisp, event);
        break;
    case HW_INPUT_JOY_AXIS :
        handleAxis(welcomeDisp, event);
        break;
    }
}
