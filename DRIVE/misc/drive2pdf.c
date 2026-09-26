#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "global.h"
#include "message.h"
#include "newMessage.h"
#include "sockets.h"
#include "connection.h"
#include "object.h"
#include "controls.h"
#include "scene.h"
#include "drive_server.h"
#include "filenames.h"
#include "demo_physics.h"
#include "drive.h"
#include "obj_common.h"

#include "pdf.h"

int oneScenePerPage = 0;
int oneFilePerPage = 0;
int showSceneBounds = 0;
int showGrid = 0;
int drawGround = 1;
int xmin = -20;
int xmax = 200;
int zmin = -20;
int zmax = 200;
int doLabel = 0;
char *labelString = NULL;
int appendLabelCoords = 0;
char *outName = NULL;
char **inNames = NULL;
int inCount = 0;
void *pdfFile;

void parseArgs(int argc, char **argv);
void doArc(float cx, float cy, float rad, float a0, float a1);
void drawBBox(void);
void drawGrid(void);
void drawAxis(void);

SCENE *scene_head = NULL;
hwDisplay hw_disp;

#define ROAD_COLOR      0.1, 0.1, 0.1
#define GRASS_COLOR     0.88, 1.0, 0.63
#define BERM_COLOR      0.44, 0.5, 0.32
#define POND_COLOR      0.75, 0.75, 1.0
#define SHADOW_COLOR    0.5, 0.5, 0.5
#define WALL_COLOR      0.5, 0.5, 0.5
#define ICE_COLOR       1.0, 1.0, 1.0
#define BUMP_COLOR      0.7, 0.7, 0.0
#define TUBE_COLOR      0.25, 0.25, 0.25
#define START_COLOR     0.75,1.0,0.75
#define FINISH_COLOR    1.0,0.75,0.75
#define BLACK_COLOR     0.0,0.0,0.0
#define WHITE_COLOR     1.0,1.0,1.0

int main(int argc, char **argv)
{
    int i, n, ret;
    SCENE *scene;
    DRIVE_OBJECT *obj;
    char *prevScene = NULL;
    int pageStarted = 0;
    int minx, minz, maxx, maxz, dx, dz, pw, ph;
    float scale;
    float length, width, mat[6], cosA, sinA, x, z;
    float *ptr;
    SceneEd_OBJECT *se;
    static float gridDash[] = {2.,18.};

    /* Init everything */
    parseArgs(argc, argv);

    if (!hwInit(argc, argv)) exit(1);
    hw_disp = hwDefaultDisplay->create( hwDefaultDisplay, NULL, NULL );

    /* Read in the scenes */
    for (i = 0; i < inCount; i++) {
        preview_scenefile(inNames[i]);
    }

    for (i = 0; i < inCount; i++) {
        read_scenefile(inNames[i], &ret);
    }

    /* Calculate bounding box */
    minx = minz = 100000;
    maxx = maxz = -100000;
    for (scene = scene_head; scene; scene = scene->next) {
        if ((scene->xscene < xmin) || (scene->zscene < zmin) ||
            (scene->xscene > xmax) || (scene->zscene > zmax)) {
            /* Ignore this scene */
            continue;
        }

        if (scene->xscene < minx) {
            minx = scene->xscene;
        }
        if (scene->xscene > maxx) {
            maxx = scene->xscene;
        }
        if (scene->zscene < minz) {
            minz = scene->zscene;
        }
        if (scene->zscene > maxz) {
            maxz = scene->zscene;
        }
    }

    /* Scale so it fits - tbd - do per page */
    dx = maxx - minx + 1;
    dz = maxz - minz + 1;

    if (dx > dz) {
        pw = 11 * 72; ph = 8.5 * 72;
        scale = pw * 0.0005 / dx;
    } else {
        pw = 8.5 * 72; ph = 11 * 72;
        scale = ph * 0.0005 / dz;
    }

    pdfFile = pdfInit(outName, pw, ph);
    if (!pdfFile) {
        fprintf(stderr, "Cannot open %s for writing\n", outName);
        exit(1);
    }
    pdfConcatMatrix(pdfFile, scale, 0., 0., scale,
                              -minx*2000.*scale, -minz*2000.*scale);

    /* Print them out to the PDF */
    for (scene = scene_head; scene; scene = scene->next) {
        if ((scene->xscene < xmin) || (scene->zscene < zmin) ||
            (scene->xscene > xmax) || (scene->zscene > zmax)) {
            /* Ignore this scene */
            continue;
        }

        if (oneScenePerPage) {
            if (pageStarted) {
                pdfNextPage(pdfFile);
            }
        }
        if (prevScene && (strcmp(scene->scenefilename, prevScene) != 0)) {
            if (pageStarted && oneFilePerPage) {
                pdfNextPage(pdfFile);
            }
        }

        pdfSaveState(pdfFile);
        pdfConcatMatrix( pdfFile, 1., 0., 0., 1.,
                                2000.*scene->xscene,
                                2000.*scene->zscene);

        if (drawGround) {
            pdfRgbColor(pdfFile, PDF_PAINT_FILL, GRASS_COLOR);
            drawBBox();
            pdfPaintPath(pdfFile, PDF_PAINT_FILL);
        }

        /* Draw non-road non-checkpoint objects */
        for (obj = scene->object_head; obj; obj = obj->next) {
            se = obj->scene_ed;
            if (!se) continue;

            /* Precook matrix in case things need it */
            sinA = sin(se->yrot deg);
            cosA = cos(se->yrot deg);
            mat[0] = cosA; mat[1] = sinA;
            mat[2] = -sinA; mat[3] = cosA;
            mat[4] = se->x; mat[5] = se->z;

            switch (obj->idptr->number) {
            case POND_OBJECT :
            case MOUND_OBJECT :
            case SPHERE_OBJECT :
            case OVAL_OBJECT :
                if (se->width > se->length) {
                    if (se->length == 0) break;
                    z = se->width / (float)se->length;
                    mat[0] *= z; mat[1] *= z;
                    x = se->length;
                } else {
                    if (se->width == 0) break;
                    z = se->length / (float)se->width;
                    mat[2] *= z; mat[3] *= z;
                    x = se->width;
                }
                pdfSaveState(pdfFile);
                pdfConcatMatrix(pdfFile, mat[0], mat[1], mat[2], mat[3],
                                      mat[4], mat[5]);
                if (obj->idptr->number == POND_OBJECT) {
                    pdfRgbColor(pdfFile, PDF_PAINT_FILL, POND_COLOR);
                } else if (obj->idptr->number == SHADOW_OBJECT) {
                    pdfRgbColor(pdfFile, PDF_PAINT_FILL, SHADOW_COLOR);
                } else {
                    pdfRgbColor(pdfFile, PDF_PAINT_FILL,
                                obj->color[0], obj->color[1], obj->color[2]);
                }
                doArc(0., 0., x * 0.5, 0., 360.);
                pdfPaintPath(pdfFile, PDF_PAINT_FILL);
                pdfRestoreState(pdfFile);
                break;
            case WALL_OBJECT :
                pdfSaveState(pdfFile);
                pdfConcatMatrix(pdfFile, mat[0], mat[1], mat[2], mat[3],
                                      mat[4], mat[5]);
                length = cos(se->xrot deg) * se->length;
                width = cos(se->zrot deg) * se->width;
                if (width < 0.0) width = -width;
                if ((obj->color[0] < 0.1) &&
                    (obj->color[1] < 0.1) &&
                    (obj->color[2] < 0.1)) {
                    pdfRgbColor(pdfFile, PDF_PAINT_STROKE, WALL_COLOR);
                } else {
                    pdfRgbColor(pdfFile, PDF_PAINT_STROKE,
                                obj->color[0], obj->color[1], obj->color[2]);
                }
                pdfLineWidth(pdfFile, width);
                pdfMoveTo(pdfFile, 0., 0.);
                pdfLineTo(pdfFile, 0., length);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
                pdfRestoreState(pdfFile);
                break;
            case CURVEWALL_OBJECT :
                if (se->xrot == 180.0) break;
                if ((obj->color[0] < 0.1) &&
                    (obj->color[1] < 0.1) &&
                    (obj->color[2] < 0.1)) {
                    pdfRgbColor(pdfFile, PDF_PAINT_STROKE, WALL_COLOR);
                } else {
                    pdfRgbColor(pdfFile, PDF_PAINT_STROKE,
                                obj->color[0], obj->color[1], obj->color[2]);
                }
                pdfLineWidth(pdfFile, se->width);
                doArc(se->x, se->z, obj->radius-se->width*0.5,
                        se->yrot, se->yrot+se->angle);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
                break;
            case CURVEGUARDRAIL_OBJECT :
                break;
            case FLAT_OBJECT :
            case ICE_OBJECT :
            case BUMP_OBJECT :
                pdfSaveState(pdfFile);
                mat[4] = se->x - se->width*0.5;
                mat[5] = se->z - se->length*0.5;
                pdfConcatMatrix(pdfFile, mat[0], mat[1], mat[2], mat[3],
                                      mat[4], mat[5]);

                if (obj->idptr->number == ICE_OBJECT) {
                    pdfRgbColor(pdfFile, PDF_PAINT_FILL, ICE_COLOR);
                } else if (obj->idptr->number == BUMP_OBJECT) {
                    pdfRgbColor(pdfFile, PDF_PAINT_FILL, BUMP_COLOR);
                } else {
                    pdfRgbColor(pdfFile, PDF_PAINT_FILL,
                                obj->color[0], obj->color[1], obj->color[2]);
                }
                pdfMoveTo(pdfFile, 0., 0.);
                pdfLineTo(pdfFile, 0., se->length);
                pdfLineTo(pdfFile, se->width, se->length);
                pdfLineTo(pdfFile, se->width, 0.);
                pdfClosePath(pdfFile);
                pdfPaintPath(pdfFile, PDF_PAINT_FILL);
                pdfRestoreState(pdfFile);
                break;
            case STOP_SIGN_OBJECT :
            case SPEED_LIMIT_SIGN_OBJECT :
            case LEFT_T_SIGN_OBJECT :
            case RIGHT_T_SIGN_OBJECT :
            case TOP_T_SIGN_OBJECT :
            case LEFT_CURVE_SIGN_OBJECT :
            case RIGHT_CURVE_SIGN_OBJECT :
            case GENERIC_SIGN_OBJECT :
            case TWOPOLE_SIGN_OBJECT :
            case POWERSHIFT_SIGN_OBJECT :
                break;
            case HILL_OBJECT :
                break;
            case PARKING_LOT_OBJECT :
                break;
            case RAILROAD_OBJECT :
                break;
            case TREE_OBJECT :
            case CONIFER_OBJECT :
            case DECIDUOUS_OBJECT :
                break;
            case FOREST_OBJECT :
                break;
            case WEEDPATCH_OBJECT :
            case GRASS_OBJECT :
            case LAWN_OBJECT :
                break;
            case BUSH_OBJECT :
                break;
            case SKYSCRAPER_OBJECT :
                break;
            case SILO_OBJECT :
                break;
            case HOUSE_OBJECT :
            case BARN_OBJECT :
                break;
            }
        }

        /* Draw the scene roads */
        switch (scene->type) {
        case SCENE_NONRANDOM :
        case SCENE_WHATEVER_FITS :
        case SCENE_FLAT_NO_ROADS :
            /* Nothing for these three */
            break;
        case SCENE_FLAT_XHIGHWAY :
        case SCENE_FLAT_XROAD :
        case SCENE_HILL_XROAD :
            /* TBD: Hills as appropriate */
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            if (scene->type == SCENE_FLAT_XHIGHWAY) {
                pdfLineWidth(pdfFile, 60.);
            } else {
                pdfLineWidth(pdfFile, 30.);
            }
            pdfMoveTo(pdfFile, -1000., 0.);
            pdfLineTo(pdfFile,  1000., 0.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_ZHIGHWAY :
        case SCENE_FLAT_ZROAD :
        case SCENE_HILL_ZROAD :
            /* TBD: Hills as appropriate */
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            if (scene->type == SCENE_FLAT_ZHIGHWAY) {
                pdfLineWidth(pdfFile, 60.);
            } else {
                pdfLineWidth(pdfFile, 30.);
            }
            pdfMoveTo(pdfFile, 0., -1000.);
            pdfLineTo(pdfFile, 0.,  1000.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_LEFT_T :
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 30.);
            pdfMoveTo(pdfFile,     0., -1000.);
            pdfLineTo(pdfFile,     0.,  1000.);
            pdfMoveTo(pdfFile,     0.,  0.);
            pdfLineTo(pdfFile, -1000.,  0.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_RIGHT_T :
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 30.);
            pdfMoveTo(pdfFile,     0., -1000.);
            pdfLineTo(pdfFile,     0.,  1000.);
            pdfMoveTo(pdfFile,     0.,  0.);
            pdfLineTo(pdfFile,  1000.,  0.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_DOWN_T :
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 30.);
            pdfMoveTo(pdfFile, -1000.,  0.);
            pdfLineTo(pdfFile,  1000.,  0.);
            pdfMoveTo(pdfFile,     0., -1000.);
            pdfLineTo(pdfFile,     0.,  0.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_UP_T :
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 30.);
            pdfMoveTo(pdfFile, -1000.,  0.);
            pdfLineTo(pdfFile,  1000.,  0.);
            pdfMoveTo(pdfFile,     0.,  1000.);
            pdfLineTo(pdfFile,     0.,  0.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_CROSSROADS :
        case SCENE_FLAT_OVERPASS :
        case SCENE_HILL_CROSSROADS :
        case SCENE_FLAT_RANDOM_TOWN :
        case SCENE_FLAT_VILLAGE :
        case SCENE_FLAT_TOWN :
        case SCENE_FLAT_CITY :
            /* The flatoverpass is random, so we just draw it as
             * an intersection.  Likeweise, villages, cities, etc.
             * are just drawn as a crossroads
             *
             * TBD: Hills as appropriate
             */
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 30.);
            pdfMoveTo(pdfFile, -1000.,  0.);
            pdfLineTo(pdfFile,  1000.,  0.);
            pdfMoveTo(pdfFile, 0., -1000.);
            pdfLineTo(pdfFile, 0.,  1000.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_TURN_RIGHT_DOWN :
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 30.);
            doArc(1000.,-1000.,1000.,90.,180.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_TURN_RIGHT_UP :
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 30.);
            doArc(1000.,1000.,1000.,180.,270.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_TURN_LEFT_DOWN :
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 30.);
            doArc(-1000.,-1000.,1000.,0.,90.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_TURN_LEFT_UP :
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 30.);
            doArc(-1000.,1000.,1000.,-90.,0.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_XHIGHWAY_OVERPASS :
            pdfSaveState(pdfFile);
                pdfRgbColor(pdfFile, PDF_PAINT_FILL, BERM_COLOR);
                pdfMoveTo(pdfFile, -100.,  30.);
                pdfLineTo(pdfFile,    0.,  50.);
                pdfLineTo(pdfFile,  100.,  30.);
                pdfClosePath(pdfFile);
                pdfMoveTo(pdfFile, -100., -30.);
                pdfLineTo(pdfFile,    0., -50.);
                pdfLineTo(pdfFile,  100., -30.);
                pdfClosePath(pdfFile);
                pdfPaintPath(pdfFile, PDF_PAINT_FILL);
            pdfRestoreState(pdfFile);

            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 60.);

            pdfMoveTo(pdfFile, -1000., 0.);
            pdfLineTo(pdfFile,  1000., 0.);
            pdfMoveTo(pdfFile, 0., -1000.);
            pdfLineTo(pdfFile, 0.,  1000.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_FLAT_ZHIGHWAY_OVERPASS :
            pdfSaveState(pdfFile);
                pdfRgbColor(pdfFile, PDF_PAINT_FILL, BERM_COLOR);
                pdfMoveTo(pdfFile, 30., -100.);
                pdfLineTo(pdfFile, 50.,    0.);
                pdfLineTo(pdfFile, 30.,  100.);
                pdfClosePath(pdfFile);
                pdfMoveTo(pdfFile, -30., -100.);
                pdfLineTo(pdfFile, -50.,    0.);
                pdfLineTo(pdfFile, -30.,  100.);
                pdfClosePath(pdfFile);
                pdfPaintPath(pdfFile, PDF_PAINT_FILL);
            pdfRestoreState(pdfFile);

            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
            pdfLineWidth(pdfFile, 60.);

            pdfMoveTo(pdfFile, -1000., 0.);
            pdfLineTo(pdfFile,  1000., 0.);
            pdfMoveTo(pdfFile, 0., -1000.);
            pdfLineTo(pdfFile, 0.,  1000.);
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
            break;
        case SCENE_HILL_NO_ROADS :
            /* TBD: Hills */
            break;
        }

        /* Draw roads */
        for (obj = scene->object_head; obj; obj = obj->next) {
            se = obj->scene_ed;
            if (!se) continue;

            /* Precook matrix in case things need it */
            sinA = sin(se->yrot deg);
            cosA = cos(se->yrot deg);
            mat[0] = cosA; mat[1] = sinA;
            mat[2] = -sinA; mat[3] = cosA;
            mat[4] = se->x; mat[5] = se->z;

            switch (obj->idptr->number) {
            case ROAD_OBJECT :
            case HILLROAD_OBJECT :
                pdfSaveState(pdfFile);

                pdfConcatMatrix(pdfFile, mat[0], mat[1], mat[2], mat[3],
                                      mat[4], mat[5]);
                length = cos(se->xrot deg) * se->length;
                width = cos(se->zrot deg) * se->width;
                if (width < 0.f) width = -width;
                pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
                pdfLineWidth(pdfFile, width);
                pdfMoveTo(pdfFile, 0.0, 0.0);
                pdfLineTo(pdfFile, 0.0, length);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
                pdfRestoreState(pdfFile);
                break;
            case SPLROAD_OBJECT :
                pdfSaveState(pdfFile);
                pdfConcatMatrix(pdfFile, mat[0], mat[1], mat[2], mat[3],
                                      mat[4], mat[5]);
                pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
                pdfLineWidth(pdfFile, se->width);

                ptr = obj->data;
                n = obj->dimension;
                pdfMoveTo(pdfFile, ptr[0], ptr[2]);
                for (i = 1; i < obj->count; i++) {
                    x = (ptr[0] + ptr[n]) * 0.5;
                    z = (ptr[2] + ptr[n+2]) * 0.5;
                    pdfLineTo(pdfFile, x, z);
                    ptr += n;
                }
                pdfLineTo(pdfFile, ptr[0], ptr[2]);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
                pdfRestoreState(pdfFile);
                break;
            case CURVE_OBJECT :
            case SPIRAL_OBJECT :
                pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
                pdfLineWidth(pdfFile, se->width);
                doArc(se->x, se->z, obj->radius-se->width*0.5,
                        se->yrot, se->yrot+se->angle);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
                break;
            case RAMP_OBJECT :
            case TWISTRAMP_OBJECT :
                pdfSaveState(pdfFile);
                pdfConcatMatrix(pdfFile, mat[0], mat[1], mat[2], mat[3],
                                      mat[4], mat[5]);
                pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
                pdfLineWidth(pdfFile, se->width);
                pdfMoveTo(pdfFile, 0.0, 0.0);
                pdfLineTo(pdfFile, 0.0, se->length);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE);

                pdfRgbColor(pdfFile, PDF_PAINT_FILL, BERM_COLOR);
                pdfMoveTo(pdfFile, -se->width*.5, 0.);
                pdfLineTo(pdfFile, -se->width*.5-10., se->length);
                pdfLineTo(pdfFile, -se->width*.5, se->length);
                pdfClosePath(pdfFile);
                pdfMoveTo(pdfFile, se->width*.5, 0.);
                pdfLineTo(pdfFile, se->width*.5+10., se->length);
                pdfLineTo(pdfFile, se->width*.5, se->length);
                pdfClosePath(pdfFile);
                pdfPaintPath(pdfFile, PDF_PAINT_FILL);

                pdfRestoreState(pdfFile);
                break;
            case BRIDGE_OBJECT :
                pdfSaveState(pdfFile);
                pdfConcatMatrix(pdfFile, mat[0], mat[1], mat[2], mat[3],
                                      mat[4], mat[5]);
                pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
                pdfLineWidth(pdfFile, se->width);
                pdfMoveTo(pdfFile, 0., -se->length*0.5);
                pdfLineTo(pdfFile, 0.,  se->length*0.5);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE);

                pdfRgbColor(pdfFile, PDF_PAINT_FILL, BERM_COLOR);
                pdfMoveTo(pdfFile, -se->width/2., -se->length/2.);
                pdfLineTo(pdfFile, -se->width/2.-10., -se->width/2.);
                pdfLineTo(pdfFile, -se->width/2., -se->width/2.);
                pdfClosePath(pdfFile);
                pdfMoveTo(pdfFile, se->width/2., -se->length/2.);
                pdfLineTo(pdfFile, se->width/2.+10., -se->width/2.);
                pdfLineTo(pdfFile, se->width/2., -se->width/2.);
                pdfClosePath(pdfFile);
                pdfMoveTo(pdfFile, -se->width/2., se->length/2.);
                pdfLineTo(pdfFile, -se->width/2.-10., se->width/2.);
                pdfLineTo(pdfFile, -se->width/2., se->width/2.);
                pdfClosePath(pdfFile);
                pdfMoveTo(pdfFile, se->width/2., se->length/2.);
                pdfLineTo(pdfFile, se->width/2.+10., se->width/2.);
                pdfLineTo(pdfFile, se->width/2., se->width/2.);
                pdfClosePath(pdfFile);
                pdfPaintPath(pdfFile, PDF_PAINT_FILL);

                pdfRestoreState(pdfFile);
                break;
            case BANK_OBJECT :
                pdfRgbColor(pdfFile, PDF_PAINT_STROKE, ROAD_COLOR);
                pdfLineWidth(pdfFile, se->width);
                doArc(se->x, se->z, obj->radius-se->width*0.5,
                        se->yrot, se->yrot+se->angle);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
                break;
            case LOWER_TUBE_OBJECT :
                pdfSaveState(pdfFile);
                pdfConcatMatrix(pdfFile, mat[0], mat[1], mat[2], mat[3],
                                      mat[4], mat[5]);
                pdfRgbColor(pdfFile, PDF_PAINT_STROKE, TUBE_COLOR);
                pdfLineWidth(pdfFile, obj->radius*2.);
                pdfMoveTo(pdfFile, 0., -se->length*.5);
                pdfLineTo(pdfFile, 0., se->length*.5);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
                pdfRestoreState(pdfFile);
                break;
            }
        }

        /* Draw checkpoints */
        for (obj = scene->object_head; obj; obj = obj->next) {
            se = obj->scene_ed;
            if (!se) continue;

            /* Precook matrix in case things need it */
            sinA = sin(se->yrot deg);
            cosA = cos(se->yrot deg);
            mat[0] = cosA; mat[1] = sinA;
            mat[2] = -sinA; mat[3] = cosA;
            mat[4] = se->x; mat[5] = se->z;

            switch (obj->idptr->number) {
            case START_OBJECT :
            case FINISH_OBJECT :
            case CHECKPOINT_OBJECT :
                pdfSaveState(pdfFile);
                pdfConcatMatrix(pdfFile, mat[0], mat[1], mat[2], mat[3],
                                      mat[4], mat[5]);
                if (obj->idptr->number == START_OBJECT) {
                    pdfRgbColor(pdfFile, PDF_PAINT_FILL, START_COLOR);
                } else if (obj->idptr->number == FINISH_OBJECT) {
                    pdfRgbColor(pdfFile, PDF_PAINT_FILL, FINISH_COLOR);
                } else {
                    pdfRgbColor(pdfFile, PDF_PAINT_FILL, WHITE_COLOR);
                }
                pdfRgbColor(pdfFile, PDF_PAINT_STROKE, BLACK_COLOR);
                pdfLineWidth(pdfFile, 5.);
                doArc(0.,0.,30.,9.,360.);
                pdfPaintPath(pdfFile, PDF_PAINT_STROKE | PDF_PAINT_FILL);
                /* TBD: Text label */
                pdfRestoreState(pdfFile);
                break;
            }
        }

        /* Grids / axes / boundaries / etc. */
        if (showSceneBounds) {
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, BLACK_COLOR);
            pdfLineWidth(pdfFile, 10.);
            drawBBox();
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
        }

        if (showGrid) {
            pdfRgbColor(pdfFile, PDF_PAINT_STROKE, BLACK_COLOR);
            pdfLineWidth(pdfFile, 1.);

            drawAxis();
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);

            pdfLineDash(pdfFile, 2, gridDash, 0.);
            drawGrid();
            pdfPaintPath(pdfFile, PDF_PAINT_STROKE);
        }

        pdfRestoreState(pdfFile);

        /* Track things for pagination */
        pageStarted = 1;
        prevScene = scene->scenefilename;
    }

    pdfFinish(pdfFile);
}

void drawBBox(void)
{
    pdfMoveTo(pdfFile, -1000.,  1000.);
    pdfLineTo(pdfFile,  1000.,  1000.);
    pdfLineTo(pdfFile,  1000., -1000.);
    pdfLineTo(pdfFile, -1000., -1000.);
    pdfClosePath(pdfFile);
}

void drawGrid(void)
{
    float i;

    for (i = -1000.; i <= 1000.; i += 100.) {
        pdfMoveTo(pdfFile, i, -1000.);
        pdfLineTo(pdfFile, i,  1000.);
        pdfMoveTo(pdfFile, -1000., i);
        pdfLineTo(pdfFile,  1000., i);
    }
}

void drawAxis(void)
{
    pdfMoveTo(pdfFile, 0., -1000.);
    pdfLineTo(pdfFile, 0.,  1000.);
    pdfMoveTo(pdfFile, -1000., 0.);
    pdfLineTo(pdfFile,  1000., 0.);
}

void cutBez(float res[8], float bez[8], float t, int backHalf)
{
    float i0[6], i1[4], i2[2];
    float t1;

    t1 = 1.0 - t;

    i0[0] = bez[0]*t1 + bez[2]*t;
    i0[1] = bez[1]*t1 + bez[3]*t;
    i0[2] = bez[2]*t1 + bez[4]*t;
    i0[3] = bez[3]*t1 + bez[5]*t;
    i0[4] = bez[4]*t1 + bez[6]*t;
    i0[5] = bez[5]*t1 + bez[7]*t;

    i1[0] = i0[0]*t1 + i0[2]*t;
    i1[1] = i0[1]*t1 + i0[3]*t;
    i1[2] = i0[2]*t1 + i0[4]*t;
    i1[3] = i0[3]*t1 + i0[5]*t;

    i2[0] = i1[0]*t1 + i1[2]*t;
    i2[1] = i1[1]*t1 + i1[3]*t;

    if (backHalf) {
        res[0] = i2[0]; res[1] = i2[1];
        res[2] = i1[2]; res[3] = i1[3];
        res[4] = i0[4]; res[5] = i0[5];
        res[6] = bez[6]; res[7] = bez[7];
    } else {
        res[0] = bez[0]; res[1] = bez[1];
        res[2] = i0[0]; res[3] = i0[1];
        res[4] = i1[0]; res[5] = i1[1];
        res[6] = i2[0]; res[7] = i2[1];
    }
}

void xform(float *dst, float *src, int np, float mat[6])
{
    while (np--) {
        dst[0] = mat[0]*src[0] + mat[2]*src[1] + mat[4];
        dst[1] = mat[1]*src[0] + mat[3]*src[1] + mat[5];
        dst += 2; src += 2;
    }
}

#define CBEZ    0.552285    /* (4*sqrt(2) - 4)/3 */

void doArc(float cx, float cy, float rad, float a0, float a1)
{
    static float quads[4][8] = {
        {  1.,0.,   1.,CBEZ,    CBEZ,1.,    0.,1.  },
        {  0.,1.,   -CBEZ,1.,   -1.,CBEZ,   -1.,0., },
        { -1.,0.,   -1.,-CBEZ,  -CBEZ,-1.,  0.,-1.  },
        {  0.,-1.,  CBEZ,-1.,   1.,-CBEZ,   1.,0., }
    };
    float bez[8], bez1[8], *qp;
    int startQ, endQ, Q;
    float startT, endT;
    float mat[6];

    /* Normalize to 0..360 range */
    while (a0 < 0.) {
        a0 += 360.; a1 += 360.;
    }
    while (a0 >= 360.0) {
        a0 -= 360.; a1 -= 360.;
    }

    /* Create matrix */
    mat[0] = rad; mat[1] = 0.0;
    mat[2] = 0.0; mat[3] = rad;
    mat[4] = cx;  mat[5] = cy;

    startQ = (int) (a0 / 90.);
    startT = (a0 - 90. * startQ) / 90.;

    endQ = (int) (a1 / 90.);
    endT = (a1 - 90. * endQ) / 90.;


    if (startQ == endQ) {
        if (a0 < a1) {
            /* partial arc */
            endT = (endT - startT) / (1. - startT);
            cutBez(bez, quads[startQ], startT, 1);
            cutBez(bez1, bez, endT, 0);
            xform(bez, bez1, 4, mat);
            pdfMoveTo(pdfFile, bez[0], bez[1]);
            pdfCurveTo(pdfFile, bez[2], bez[3], bez[4], bez[5], bez[6], bez[7]);
            return;
        } else {
            /* Wrap around */
            endQ += 4;
        }
    }

    /* Put out partial first arc */
    cutBez(bez1, quads[startQ], startT, 1);
    xform(bez, bez1, 4, mat);
    pdfMoveTo(pdfFile, bez[0], bez[1]);
    pdfCurveTo(pdfFile, bez[2], bez[3], bez[4], bez[5], bez[6], bez[7]);

    for (Q = startQ + 1; Q < endQ; Q++) {
        xform(bez, quads[Q % 4], 4, mat);
        pdfCurveTo(pdfFile, bez[2], bez[3], bez[4], bez[5], bez[6], bez[7]);
    }

    /* Put out partial last arc */
    cutBez(bez1, quads[Q % 4], endT, 0);
    xform(bez, bez1, 4, mat);
    pdfCurveTo(pdfFile, bez[2], bez[3], bez[4], bez[5], bez[6], bez[7]);
    
}

void usage(void)
{
    fprintf(stderr, "Usage: drive2pdf [opts] out.pdf scenefile scenefile...\n");
    fprintf(stderr, "\t-s         Print one scene per page\n");
    fprintf(stderr, "\t-f         Print one file per page\n");
    fprintf(stderr, "\t-b         Show scene boundaries\n");
    fprintf(stderr, "\t-g         Show 100ft grid\n");
    fprintf(stderr, "\t-G         Don't draw the ground\n");
    fprintf(stderr, "\t-xmin x    Don't print scenes below xmin (def=-20)\n");
    fprintf(stderr, "\t-xmax x    Don't print scenes above xmax (def=200)\n");
    fprintf(stderr, "\t-zmin z    Don't print scenes below zmin (def=-20)\n");
    fprintf(stderr, "\t-zmax z    Don't print scenes above zmax (def=200)\n");
    fprintf(stderr, "\t-l         Label with filename\n");
    fprintf(stderr, "\t-L string  Label with supplied string\n");
    fprintf(stderr, "\t-c         Add coordinates to existing label\n");
    exit(1);
}

void parseArgs(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
        if (*argv[i] == '-') {
            if (strcmp(argv[i], "-s") == 0) {
                oneScenePerPage = 1;
            } else if (strcmp(argv[i], "-f") == 0) {
                oneFilePerPage = 1;
            } else if (strcmp(argv[i], "-b") == 0) {
                showSceneBounds = 1;
            } else if (strcmp(argv[i], "-g") == 0) {
                showGrid = 1;
            } else if (strcmp(argv[i], "-G") == 0) {
                drawGround = 0;
            } else if (strcmp(argv[i], "-xmin") == 0) {
                if (i >= (argc - 1)) usage();
                xmin = atoi(argv[++i]);
            } else if (strcmp(argv[i], "-xmax") == 0) {
                if (i >= (argc - 1)) usage();
                xmax = atoi(argv[++i]);
            } else if (strcmp(argv[i], "-zmin") == 0) {
                if (i >= (argc - 1)) usage();
                zmin = atoi(argv[++i]);
            } else if (strcmp(argv[i], "-zmax") == 0) {
                if (i >= (argc - 1)) usage();
                zmax = atoi(argv[++i]);
            } else if (strcmp(argv[i], "-l") == 0) {
                doLabel = 1;
                labelString = NULL;
            } else if (strcmp(argv[i], "-L") == 0) {
                if (i >= (argc - 1)) usage();
                doLabel = 1;
                labelString = argv[++i];
            } else if (strcmp(argv[i], "-c") == 0) {
                appendLabelCoords = 1;
            } else {
                usage();
            }
        } else if (outName) {
            inNames = argv + i;
            inCount = argc - i;
            break;
        } else {
            outName = argv[i];
        }
    }
    if (!(inNames && outName)) {
        usage();
    }
}

/* Various drive server things that we have to do to link everything */
#define MAX_VEHICLE_TYPES 20

char construct_dir[1024];
char object_block_filename[MAXPATHLEN];
int scene_ed = 1;

unsigned int allowable_vehicles;
int cur_vehicle_types = 0;/* number of vehicle types known by server */
int updates;
Driveable driveables[MAX_VEHICLE_TYPES];
unsigned int current_nameset_bit;
COURSE default_course = {
    "Default",                  /* name */
    0, -2,                      /* start_scene_[xz] */
    0.0,                        /* start angle */
    { 88.0, 1.0, -110.0 },      /* start position */
    8,                          /* num positions */
    15.0, 30.0,                 /* hspacing, vspacing */
    TURBO_UNLIMITED, 0,         /* turbo mode, boosts */
    GUNS_UNLIMITED, 0,          /* guns mode, ammo */
    120, 5, 120, 5,             /* times */
    NULL                        /* next */
};
boolean_type server_reread_scenefiles = FALSE;
boolean_type debug=0;
int do_textures;
int objPrinting = 0;
int img_fildes = 0;

void update_all_clients_state(int client_state) {}
void reset_all_clients(void) {}
void update_leader_board(int standings_type) {}
unsigned int server_mode;
void shutdown_server(void) {}
void change_virtual_time(boolean_type time_frozen, time_value virtual_time) {}
void _hp_high_res_sleep( double t ) {}

boolean_type server_upright(connection_type *c, struct IPCMsg *Msg)
{
    return FALSE;
}

boolean_type server_restart_client(connection_type *c, struct IPCMsg *Msg)
{
    return FALSE;
}

void cause_explosion(
    SCENE *scene,
    float x, float y, float z,
    float radius, float base_force, float base_torque)
{
}

void self_lit_on( int fildes ) {}
void self_lit_off( int fildes ) {}

void _hp_invert( float a[4][4], float b[4][4], int n )
{
    hwInvertMat( a, b );
}

void concat_matrix( float m1[4][4], float m2[4][4], float m3[4][4] )
{
    float
        t1[4][4],
        t2[4][4];

    (void)memcpy( t1, m1, 4*4*sizeof(float) );
    (void)memcpy( t2, m2, 4*4*sizeof(float) );
    hwMatMult( m3, t1, t2 );
}

void _hp_identity( float mat[4][4] )
{
    hwIdentity( mat );
}

void add_names_to_set( int dl, int a, int b[] ) {}
void remove_all_names_from_set( int seg ) {}
void concat_transformation3d( int gfd, float m[4][4], int a, int b ) {}
void pop_matrix( int gfd ) {}
int hidden_surface( int a, int b, int c ) {}
void fill_color( int fildes, float a, float b, float c ) {}
void line_color( int gfd, float r, float g, float b ) {}
void c_line_color( int fildes, float r, float g, float b ) {}
void c_fill_color( int gfd, double r, double g, double b ) {}
#undef vertex_format
void vertex_format( int g, int c, int u, int rgb, int normals, int order ) {}
void inquire_vertex_format( int g, int *c, int *u, int *a, int *n, int *b) {}
void quadrilateral_mesh( int a, float b[], int c, int d, float e[] ) {}
void triangular_strip( int a, float b[], int d, float e[] ) {}
void partial_polygon3d( int f, float c[], int n, int f1, int cl) {}
void polygon3d( int gfd, float clist[], int n, int m ) {}
void polyline3d( int gfd, float clist[], int n, int m ) {}
void push_matrix3d( int fildes, float xform3[4][4] ) {}
void cond_return( int fildes, int cond_index_select, int comp_flag ) {}
void set_ele_ptr_relative( int fildes, int offset ) {}
void interpret_ele( int fildes, int *ele ) {}
void interior_style( int fildes, int style, int edged ) {}

int gopen( char *path, int kind, char *driver, int mode )
{
    return 255;
}

void inq_ele( int a, int *b ) {}
void cond_execute_segment( int a, int b, int c, int d ) {}
void inq_ele_size( int a, int *b ) {}
void surface_model( int gfd, int a1, int a2, float r, float g, float b ) {}
void execute_segment( int a, int b ) {}
void move3d( int gfd, float a, float b, float c ) {}
void cond_call_segment( int a, int b, int c, int d ) {}
void c_set_cull_size( int gfd, double zzz ) {}
void set_cull_size( int gfd, double zzz ) {}
void set_extent( int gfd, float a[2][3] ) {}
void call_segment( int a, int b ) {}
void surface_coefficients( int gfd, float amb, float diff, float spec ) {}
void close_segment( int gfd ) {}
void inq_ele_ptr_at_bound( int gfd, int *a, int *b ) {}
void open_segment( int a, int b, int c, int d ) {}
void draw3d( int a, float b, float c, float d ) {}
void set_ele_ptr( int a, int b ) {}
