#ifdef DO_TEST
#include <stdio.h>
#include "textgad.h"
#endif

/*
Current State:  Post-Race                                     Time Left:  00:007
                                                                               
[ NEXT STATE ]  [ ] Automatic Race Control  Course: [ Default ]                
                                                                               
Start stopwatch when: [ Start Line Crossed ]                      +----------+
                                                   Practice Time: | 00:00:00 |
       End race when: [ Finish Line Crossed ]                     +----------+
                                                                  +----------+
        Wreck Action: [ Upright ]                  Pre-Race Time: | 00:00:00 |
                                                                  +----------+
    Explosion Action: [ Random Force ]                            +----------+
                                                       Race Time: | 00:00:00 |
          Turbo Mode: [ Unlimited ]                               +----------+
                      +-------+                                   +----------+
        Turbo Boosts: |     0 |                   Post-Race Time: | 00:00:00 |
                      +-------+                                   +----------+
                                               Time-of-Day:       
           Guns Mode: [ Unlimited Ammo ]       [--#---------------------]
                      +-------+                +-------+
                Ammo: |     0 |                | 02:00 |  [ ] Freeze
                      +-------+                +-------+              [ QUIT ]



*/

#define MAX_COURSES     128
static char numchars[] = "0123456789";
static char timechars[] = "0123456789:";
static char *courses[MAX_COURSES] = {
    "airport", "alpine_ralley", "ariel", "beam_me_up", "bridge", "bullwinkle",
    "bumpy", "chefs_house", "collins_point", "croquet", "fahrvergnugen",
    "figure8", "four_seasons", "greek_temple", "grrville", "head_on", "highway",
    "indy", "interpl_tour", "jedi_training", "jr_dobbs", "lemans",
    "mike-o-rama", "mikeville", "monaco", "noobyville", "oval", "proving",
    "range", "reaction", "school", "sherman", "skytropolis", "sunday",
    "tubular", "twilight_zone", "wall_street", "white_sands", NULL
};
static char *turbo_modes[] = {
    "Off", "Limited", "Unlimited", NULL
};
static char *guns_modes[] = {
    "Off", "Limited Ammo", "Unlimited Ammo", NULL
};
static char *wreck_actions[] = {
    "None", "Upright", "Restart", NULL
};
static char *explosion_actions[] = {
    "Disabled", "Visual Only", "Random Force", "Restart", NULL
};
static char *start_actions[] = {
    "Race Starts", "Start Line Crossed", NULL
};
static char *finish_actions[] = {
    "Race Time Over", "Finish Line Crossed", "Either", NULL
};
#define MAX_EDIT 16
static char editBuffs[7][MAX_EDIT+1] = {
    "0",
    "0",
    "00:00:00",
    "00:00:00",
    "00:00:00",
    "00:00:00",
    "02:00"
};
static char messageBuffs[3][80] = {"", "", ""};

TextGadget glist[] = {
/* 0*/{ TG_CONTAINER },   /* First one is always container */
/* 1*/{ TG_LABEL,     0,  0,  0, "Current State:" },
/* 2*/{ TG_LABEL,     0, 16,  0, "Post-Race" },
/* 3*/{ TG_LABEL,     0, 58,  0, "Time Left" },
/* 4*/{ TG_LABEL,     0, 70,  0, "00:00" },

/* 5*/{ TG_BUTTON,    2,  0, 12, "NEXT STATE" },
/* 6*/{ TG_CHECKBOX , 2, 16,  0, "Automatic Race Control", 1 },
/* 7*/{ TG_LABEL,     2, 44,  0, "Course:" },
/* 8*/{ TG_MENU,      2, 52, 25, courses },

/* 9*/{ TG_LABEL,     4,  0,  0, "Start stopwatch when:" },
/*10*/{ TG_MENU,      4, 22, 20, start_actions },
/*11*/{ TG_LABEL,     6,  7,  0, "End race when:" },
/*12*/{ TG_MENU,      6, 22, 21, finish_actions },
/*13*/{ TG_LABEL,     8,  8,  0, "Wreck Action:" },
/*14*/{ TG_MENU,      8, 22,  9, wreck_actions },
/*15*/{ TG_LABEL,    10,  4,  0, "Explosion Action:" },
/*16*/{ TG_MENU,     10, 22, 14, explosion_actions },
/*17*/{ TG_LABEL,    12, 10,  0, "Turbo Mode:" },
/*18*/{ TG_MENU,     12, 22, 11, turbo_modes },
/*19*/{ TG_LABEL,    14,  8,  0, "Turbo Boosts:" },
/*20*/{ TG_TEXTEDIT, 13, 22,  7, editBuffs[0], MAX_EDIT, numchars },
/*21*/{ TG_LABEL,    17, 11,  0, "Guns Mode:" },
/*22*/{ TG_MENU,     17, 22, 16, guns_modes },
/*23*/{ TG_LABEL,    19, 16,  0, "Ammo:" },
/*24*/{ TG_TEXTEDIT, 18, 22,  7, editBuffs[1], MAX_EDIT, numchars },

/*25*/{ TG_LABEL,     5, 51,  0, "Practice Time:" },
/*26*/{ TG_TEXTEDIT,  4, 66, 10, editBuffs[2], MAX_EDIT, timechars },
/*27*/{ TG_LABEL,     8, 51,  0, "Pre-Race Time:" },
/*28*/{ TG_TEXTEDIT,  7, 66, 10, editBuffs[3], MAX_EDIT, timechars },
/*29*/{ TG_LABEL,    11, 55,  0, "Race Time:" },
/*30*/{ TG_TEXTEDIT, 10, 66, 10, editBuffs[4], MAX_EDIT, timechars },
/*31*/{ TG_LABEL,    14, 50,  0, "Post-Race Time:" },
/*32*/{ TG_TEXTEDIT, 13, 66, 10, editBuffs[5], MAX_EDIT, timechars },

/*33*/{ TG_LABEL,    16, 47,  0, "Time of day:" },
/*34*/{ TG_HSLIDE,   17, 47, 24, NULL, 2 },
/*35*/{ TG_TEXTEDIT, 18, 47,  7, editBuffs[6], MAX_EDIT, timechars },
/*36*/{ TG_CHECKBOX, 19, 58,  0, "Freeze" },

/*37*/{ TG_BUTTON,   20, 70,  0, "Quit" },

      /* three lines of text output as labels */
/*38*/{ TG_LABEL,    21,  0,  0, messageBuffs[0], 79 },
/*39*/{ TG_LABEL,    22,  0,  0, messageBuffs[1], 79 },
/*40*/{ TG_LABEL,    23,  0,  0, messageBuffs[2], 79 },

/*41*/{ TG_NULL }
};
#define GAD_STATE               2
#define GAD_TIME                4
#define GAD_NEXT_STATE          5
#define GAD_AUTO_CONTROL        6
#define GAD_COURSE              8
#define GAD_START               10
#define GAD_FINISH              12
#define GAD_WRECK               14
#define GAD_EXPLOSION           16
#define GAD_TURBO               18
#define GAD_BOOSTS              20
#define GAD_GUNS                22
#define GAD_AMMO                24
#define GAD_PRACTICE            26
#define GAD_PRE_RACE            28
#define GAD_RACE                30
#define GAD_POST_RACE           32
#define GAD_TOD_SLIDER          34
#define GAD_TOD_BOX             35
#define GAD_TOD_FREEZE          36
#define GAD_QUIT                37

#define GAD_MESSAGE             38
#define GAD_MESSAGE_SIZE        3

#ifdef DO_TEST
int done;

void callback(TextGadget *glist, int which)
{
    switch (which) {
    case GAD_QUIT :
        done = 1;
        break;
    case GAD_TIME_SLIDE :
        sprintf((char *) glist[GAD_TIME_BOX].pval, "%02d:00",
                glist[which].ival);
        tgUpdate(glist, GAD_TIME_BOX);
        tgFlush(glist);
        break;
    }
}

int main(int argc, char **argv)
{
    if (!tgInit()) {
        exit(1);
    }

    tgCompile(glist);

    tgPaintAll(glist);

    while (!done) {
        tgCheckInput(glist, callback);
    }

    tgTerm();
}
#endif
