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



#include "object.h"
#include "obj_common.h"

/********* declarations for all object initialization routines ********/
extern void init_road_object(void *obj);
extern void init_hillroad_object(void *obj);
extern void init_hillxroad_object(void *obj);
extern void init_railroad_object(void *obj);
extern void init_ground_object(void *obj);
extern void init_hill_object(void *obj);
extern void init_mound_object(void *obj);
extern void init_bump_object(void *obj);
extern void init_bridge_object(void *obj);
extern void init_countach_object(void *obj);
extern void init_rocket_car_object(void *obj);
extern void init_mcycle_object(void *obj);
extern void init_sedan_object(void *obj);
extern void init_police_object(void *obj);
extern void init_T34_object(void *obj);
extern void init_minivan_object(void *obj);
extern void init_FokD7_object(void *obj);
extern void init_ufo_object(void *obj);
extern void init_xfighter_object(void *obj);
extern void init_curve_object(void *obj);
extern void init_bank_object(void *obj);
extern void init_ramp_object(void *obj);
extern void init_twistramp_object(void *obj);
extern void init_skyscraper_object(void *obj);
extern void init_barn_object(void *obj);
extern void init_silo_object(void *obj);
extern void init_lamp_object(void *obj);
extern void init_pillar_object(void *obj);
extern void init_cone_object(void *obj);
extern void init_cylinder_object(void *obj);
extern void init_teleline_object(void *obj);
extern void init_forest_object(void *obj);
extern void init_weedpatch_object(void *obj);
extern void init_grass_object(void *obj);
extern void init_flat_object(void *obj);
extern void init_light_object(void *obj);
extern void init_lawn_object(void *obj);
extern void init_sidewalk_object(void *obj);
extern void init_shadow_object(void *obj);
extern void init_ice_object(void *obj);
extern void init_pond_object(void *obj);
extern void init_bush_object(void *obj);
extern void init_stop_sign_object(void *obj);
extern void init_speed_limit_sign_object(void *obj);
extern void init_left_t_sign_object(void *obj);
extern void init_right_t_sign_object(void *obj);
extern void init_top_t_sign_object(void *obj);
extern void init_left_curve_sign_object(void *obj);
extern void init_generic_sign_object(void *obj);
extern void init_twopole_sign_object(void *obj);
extern void init_right_curve_sign_object(void *obj);
extern void init_powershift_sign_object(void *obj);
extern void init_stoplight_object(void *obj);
extern void init_house_object(void *obj);
extern void init_tree_object(void *obj);
extern void init_conifer_object(void *obj);
extern void init_deciduous_object(void *obj);
extern void init_parking_lot_object(void *obj);
extern void init_mailbox_object(void *obj);
extern void init_ltube_object(void *obj);
extern void init_checkpt_object(void *obj);
extern void init_wall_object(void *obj);
extern void init_fillwall_object(void *obj);
extern void init_curvewall_object(void *obj);
extern void init_guardrail_object(void *obj);
extern void init_curveguardrail_object(void *obj);
extern void init_wormhole_object(void *obj);
extern void init_black_wormhole_object(void *obj);
extern void init_invis_wormhole_object(void *obj);
extern void init_shell_object(void *obj);
extern void init_spiral_object(void *obj);
extern void init_sphere_object(void *obj);
extern void init_oval_object(void *obj);
extern void init_splroad_object(void *obj);
extern void init_oval_light_object(void *obj);
extern void init_oval_shadow_object(void *obj);
extern void init_land_mine_object(void *obj);
extern void init_flames_object(void *obj);
extern void init_rubble_object(void *obj);
extern void init_fence_object(void *obj);
extern void init_polyline_object(void *obj);

OBJECT_ID object_id[] = {
    /* Position in list must match SHADOW_ID_NUMBER */
    { "Shadow",		"shadow",
	OBJECTCLASS_DYNAMIC, SHADOW_OBJECT,	init_shadow_object,0	},

    { "Ground",		"ground",
	0, GROUND_OBJECT,		init_ground_object	,
	OBJ_POSITION_FIELDS 						},

    { "Hill",		"hill",
	0, HILL_OBJECT,			init_hill_object	,0	},

    { "Mound",		"mound",
	0, MOUND_OBJECT,		init_mound_object	,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_COLOR			},

    { "Road","road", 0, ROAD_OBJECT,	init_road_object,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_COLOR			},

    { "Hillroad","hillroad", 0, HILLROAD_OBJECT,init_hillroad_object,
       0	},

    { "Hillcrossroad",	"hillcrossroad",
	0, HILLXROAD_OBJECT,		init_hillxroad_object	,0	},

    { "Railroad",	"railroad",
	0, RAILROAD_OBJECT,		init_railroad_object	,
	OBJ_DEFAULT | OBJ_LENGTH | OBJ_WIDTH 				},

    { "Curve",	"curve", 0, CURVE_OBJECT,	init_curve_object,
       OBJ_DEFAULT | OBJ_WIDTH | OBJ_HEIGHT | OBJ_RADIUS | OBJ_ANGLE |
       OBJ_COLOR 							},

    { "Bump",	"bump", 0, BUMP_OBJECT,	init_bump_object,
       OBJ_DEFAULT | OBJ_SIZE_FIELDS					},

    { "Ramp",	"ramp", 0, RAMP_OBJECT,		init_ramp_object,
       OBJ_DEFAULT | OBJ_SIZE_FIELDS					},

    { "Twistramp","twistramp", 0, TWISTRAMP_OBJECT,init_twistramp_object,
      OBJ_DEFAULT | OBJ_SIZE_FIELDS					},

    { "Skyscraper",	"skyscraper", 0, SKYSCRAPER_OBJECT,
       init_skyscraper_object,	OBJ_DEFAULT | OBJ_SIZE_FIELDS		},

    { "Bank",	"bank", 0, BANK_OBJECT,		init_bank_object,
      OBJ_DEFAULT | OBJ_WIDTH | OBJ_HEIGHT | OBJ_ANGLE | OBJ_RADIUS	},

    { "Bridge",	"bridge", 0, BRIDGE_OBJECT,	init_bridge_object,
      OBJ_DEFAULT | OBJ_SIZE_FIELDS					},

    { "Barn",	"barn", 0, BARN_OBJECT,		init_barn_object,
      OBJ_DEFAULT | OBJ_SIZE_FIELDS					},

    { "Silo",	"silo", 0, SILO_OBJECT,		init_silo_object,
      OBJ_DEFAULT | OBJ_RADIUS | OBJ_HEIGHT				},

    { "Lamp",	"lamp", 0, LAMP_OBJECT,		init_lamp_object,
      OBJ_DEFAULT | OBJ_RADIUS | OBJ_HEIGHT | OBJ_LENGTH | OBJ_TYPE	},

    { "Pillar",	"pillar", 0, PILLAR_OBJECT,	init_pillar_object,
      OBJ_DEFAULT | OBJ_RADIUS | OBJ_HEIGHT | OBJ_WIDTH	| OBJ_COLOR 	},

    { "Cylinder","cylinder", 0, PILLAR_OBJECT,	init_pillar_object,
      OBJ_DEFAULT | OBJ_RADIUS | OBJ_HEIGHT | OBJ_WIDTH | OBJ_COLOR 	},

    { "Cone",	"cone", 0, CONE_OBJECT,		init_cone_object,
      OBJ_DEFAULT | OBJ_RADIUS | OBJ_HEIGHT | OBJ_WIDTH | OBJ_COLOR	},

    { "Sphere",	"sphere", 0, SPHERE_OBJECT,	init_sphere_object,
      OBJ_DEFAULT | OBJ_RADIUS | OBJ_COLOR     				},

    { "Telephone Line","telephone line",0, TELELINE_OBJECT,init_teleline_object,
      OBJ_DEFAULT | OBJ_RADIUS | OBJ_HEIGHT | OBJ_LENGTH | OBJ_SPACING 	},

    { "House",	"house", 0, HOUSE_OBJECT,	init_house_object,
      OBJ_DEFAULT | OBJ_TYPE  						},

    { "Parking Lot","parking lot", 0,PARKING_LOT_OBJECT,init_parking_lot_object,
      OBJ_DEFAULT | OBJ_LENGTH | OBJ_WIDTH | OBJ_HEIGHT 		},

    { "Tree",		"tree", 0, TREE_OBJECT,	init_tree_object,
      OBJ_DEFAULT | OBJ_HEIGHT 						},

    { "Conifer", "conifer", 0, CONIFER_OBJECT,	init_conifer_object,
      OBJ_DEFAULT | OBJ_RADIUS | OBJ_HEIGHT | OBJ_COLOR			},

    { "Deciduous", "deciduous", 0, DECIDUOUS_OBJECT,init_deciduous_object,
      OBJ_DEFAULT | OBJ_RADIUS | OBJ_LENGTH | OBJ_WIDTH | OBJ_HEIGHT |
      OBJ_COLOR   | OBJ_TYPE                                            },

    { "Spline Road", "spline road", 0, SPLROAD_OBJECT, init_splroad_object,
      OBJ_DEFAULT | OBJ_WIDTH | OBJ_HEIGHT | OBJ_DATA | OBJ_COLOR 	},

    { "Polyline", "polyline", 0, POLYLINE_OBJECT, init_polyline_object,
      OBJ_DEFAULT | OBJ_COLOR | OBJ_DATA |OBJ_DIMENSION | OBJ_COUNT 	},

    { "Mailbox",  	"mailbox", 0, MAILBOX_OBJECT, init_mailbox_object,
      OBJ_DEFAULT 	},
    { "Lower Tube", "lower tube", 0, LOWER_TUBE_OBJECT,init_ltube_object,
	OBJ_DEFAULT | OBJ_LENGTH | OBJ_RADIUS				}, 
	
    { "Forest",	"forest", 0, FOREST_OBJECT, init_forest_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_HEIGHT | OBJ_LENGTH | OBJ_COUNT	},

    { "Weed Patch", "weed patch", 0, WEEDPATCH_OBJECT,init_weedpatch_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_HEIGHT | OBJ_LENGTH | OBJ_COUNT	},

    { "Grass", "grass", 0, GRASS_OBJECT, init_grass_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_HEIGHT | OBJ_LENGTH |
	OBJ_RADIUS  | OBJ_COUNT						},

    { "Flat", "flat", 0, FLAT_OBJECT, init_flat_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_LENGTH | OBJ_COLOR		},

    { "Light", 		"light", 0, LIGHT_OBJECT, init_light_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_LENGTH | OBJ_COLOR		},

    { "Lawn",		"lawn", 0, LAWN_OBJECT,	init_lawn_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_LENGTH 				},

    { "Sidewalk",	"sidewalk", 0, SIDEWALK_OBJECT,	init_sidewalk_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_LENGTH 				},

    { "Ice",		"ice", 0, ICE_OBJECT, init_ice_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_LENGTH				},

    { "Pond", "pond", 0, POND_OBJECT, init_pond_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_HEIGHT | OBJ_LENGTH		},

    { "Bush", "bush", 0, BUSH_OBJECT, init_bush_object, 
	OBJ_DEFAULT | OBJ_HEIGHT					},

    { "Oval", "oval", 0, OVAL_OBJECT, init_oval_object,
	OBJ_DEFAULT | OBJ_COLOR 					},

    { "Oval Light", "oval light", 0, OVAL_OBJECT, init_oval_light_object,
	OBJ_DEFAULT | OBJ_COLOR						},

    { "Oval Shadow", "oval shadow", 0, OVAL_OBJECT, init_oval_shadow_object,
	OBJ_DEFAULT | OBJ_COLOR						},

    { "Land Mine", "land mine", 0, LAND_MINE_OBJECT, init_land_mine_object,
	OBJ_DEFAULT | OBJ_HEIGHT | OBJ_RADIUS				},

    { "Flames",	"flames", 0, FLAMES_OBJECT, init_flames_object,
	OBJ_DEFAULT | OBJ_COUNT	| OBJ_SIZE_FIELDS			},

    { "Rubble",	"rubble", 0, RUBBLE_OBJECT, init_rubble_object,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_COLOR			},

    { "Fence", "fence", 0, FENCE_OBJECT, init_fence_object,
	OBJ_DEFAULT | OBJ_COLOR | OBJ_LENGTH | OBJ_HEIGHT | OBJ_TYPE |
	OBJ_SPACING  	},

    { "Start",	"start",
	OBJECTCLASS_CHECKPOINT, START_OBJECT,
	init_checkpt_object,
	OBJ_DEFAULT | OBJ_LABEL | OBJ_TYPE | OBJ_LENGTH | OBJ_HEIGHT | 
	OBJ_WIDTH   | OBJ_RADIUS | OBJ_DATA				},

    { "Finish",	"finish",
	OBJECTCLASS_CHECKPOINT, FINISH_OBJECT,
	init_checkpt_object,
	OBJ_DEFAULT | OBJ_LABEL | OBJ_TYPE | OBJ_LENGTH | OBJ_HEIGHT | 
	OBJ_WIDTH   | OBJ_RADIUS | OBJ_DATA				},

    { "Checkpoint",	"checkpoint",
	OBJECTCLASS_CHECKPOINT, CHECKPOINT_OBJECT,
	init_checkpt_object,
	OBJ_DEFAULT | OBJ_LABEL | OBJ_TYPE | OBJ_LENGTH | OBJ_HEIGHT | 
	OBJ_WIDTH   | OBJ_RADIUS | OBJ_DATA				},

    { "Wormhole",	"wormhole",
	OBJECTCLASS_WORMHOLE, WORMHOLE_OBJECT, init_wormhole_object,
	OBJ_DEFAULT | OBJ_TYPE | OBJ_LENGTH | OBJ_WIDTH | OBJ_HEIGHT	},

    { "Black Wormhole",	"black wormhole",
	OBJECTCLASS_WORMHOLE, BKWORMHOLE_OBJECT, init_black_wormhole_object,
	OBJ_DEFAULT | OBJ_TYPE | OBJ_LENGTH | OBJ_WIDTH | OBJ_HEIGHT},

    { "Invisible Wormhole",	"invisible wormhole",
	OBJECTCLASS_WORMHOLE, INVISWORMHOLE_OBJECT, init_invis_wormhole_object,
	OBJ_DEFAULT | OBJ_TYPE | OBJ_LENGTH | OBJ_WIDTH | OBJ_HEIGHT},

    { "Stop Sign",	"stop sign", 0, STOP_SIGN_OBJECT,init_stop_sign_object,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR | OBJ_TYPE },

    { "Speed Limit Sign", 	"speed limit sign",
	0, SPEED_LIMIT_SIGN_OBJECT,	init_speed_limit_sign_object,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR | OBJ_TYPE | 
	OBJ_LABEL },

    { "Left T Sign",	"left t sign",
	0, LEFT_T_SIGN_OBJECT,	init_left_t_sign_object	,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR | OBJ_TYPE },

    { "Right T Sign",	"right t sign",
	0, RIGHT_T_SIGN_OBJECT,      init_right_t_sign_object ,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR | OBJ_TYPE },

    { "Top T Sign",	"top t sign",
	0, TOP_T_SIGN_OBJECT,      init_top_t_sign_object,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR | OBJ_TYPE },

    { "Left Curve Sign",	"left curve sign",
	0, LEFT_CURVE_SIGN_OBJECT,     init_left_curve_sign_object ,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR | OBJ_TYPE | 
	OBJ_LABEL },

    { "Right Curve Sign",	"right curve sign",
	0, RIGHT_CURVE_SIGN_OBJECT,   init_right_curve_sign_object ,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR | OBJ_TYPE | 
	OBJ_LABEL },

    { "Generic Sign",	"generic sign",
	0, GENERIC_SIGN_OBJECT,   init_generic_sign_object,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR | OBJ_TYPE|OBJ_LABEL},

    { "Twopole Sign",	"twopole sign",
	0, TWOPOLE_SIGN_OBJECT,   init_twopole_sign_object ,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR|OBJ_LABEL},

    { "Powershift Sign",	"powershift sign",
	OBJECTCLASS_DYNAMIC, POWERSHIFT_SIGN_OBJECT,
	init_powershift_sign_object,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_RADIUS | OBJ_COLOR		},

    { "Stoplight",	"stoplight",
	OBJECTCLASS_DYNAMIC, STOPLIGHT_OBJECT,	init_stoplight_object,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS					},

    { "Wall",		"wall",
	0, WALL_OBJECT,	init_wall_object ,
	OBJ_DEFAULT | OBJ_SIZE_FIELDS | OBJ_COLOR			},

    { "Fillwall",	"fillwall",
	0, WALL_OBJECT,	init_fillwall_object ,0},

    { "Curvewall",		"curvewall",
	0, CURVEWALL_OBJECT,	init_curvewall_object ,
	OBJ_DEFAULT | OBJ_HEIGHT | OBJ_WIDTH | OBJ_COLOR | OBJ_RADIUS | 
	OBJ_ANGLE							},

    { "Guardrail",		"guardrail",
	0, FENCE_OBJECT,	init_guardrail_object,
	OBJ_DEFAULT | OBJ_LENGTH | OBJ_HEIGHT | OBJ_SPACING		},

    { "Curveguardrail",		"curveguardrail",
	0, CURVEGUARDRAIL_OBJECT,init_curveguardrail_object,
	OBJ_DEFAULT | OBJ_ANGLE | OBJ_RADIUS				},

    { "Spiral",		"spiral",
	0, SPIRAL_OBJECT,	init_spiral_object,
	OBJ_DEFAULT | OBJ_WIDTH | OBJ_HEIGHT | OBJ_ANGLE | OBJ_RADIUS	},

    { "Sports Car",	"countach",
	OBJECTCLASS_DYNAMIC|OBJECTCLASS_DRIVEABLE|OBJECTCLASS_CAR, 
	COUNTACH_OBJECT, init_countach_object,0	},

    { "Rocket Car",	"rocket car",
	OBJECTCLASS_DYNAMIC|OBJECTCLASS_DRIVEABLE|OBJECTCLASS_CAR, 
	ROCKET_CAR_OBJECT, init_rocket_car_object,0	},

    { "Sedan",		"sedan",
	OBJECTCLASS_DYNAMIC|OBJECTCLASS_DRIVEABLE|OBJECTCLASS_CAR, SEDAN_OBJECT,
	init_sedan_object,0	},

    { "Minivan",	"minivan",
	OBJECTCLASS_DYNAMIC|OBJECTCLASS_DRIVEABLE|OBJECTCLASS_CAR, 
	MINIVAN_OBJECT, init_minivan_object	,0},

    { "Tank",		"t34",
	OBJECTCLASS_DYNAMIC|OBJECTCLASS_DRIVEABLE|OBJECTCLASS_TANK, T34_OBJECT,
	init_T34_object	,0	},

    { "Fokker D.VII",	"fokd7",
	0 | OBJECTCLASS_DYNAMIC /* | OBJECTCLASS_DRIVEABLE */ 
	| OBJECTCLASS_AIRPLANE, FOKD7_OBJECT, init_FokD7_object	,0},

    { "UFO",	"ufo",
	OBJECTCLASS_DYNAMIC | OBJECTCLASS_DRIVEABLE | OBJECTCLASS_UFO, 
	UFO_OBJECT, init_ufo_object,0	},

    { "X Fighter",	"xfighter",
	OBJECTCLASS_DYNAMIC | OBJECTCLASS_DRIVEABLE | OBJECTCLASS_SPACESHIP, 
	XFIGHTER_OBJECT, init_xfighter_object	,0},

    { "Police Car",	"police",
	OBJECTCLASS_DYNAMIC|OBJECTCLASS_DRIVEABLE|OBJECTCLASS_CAR,POLICE_OBJECT,
	init_police_object,0	},

    { "Motorcycle",	"motorcycle",
	OBJECTCLASS_DYNAMIC|OBJECTCLASS_DRIVEABLE|OBJECTCLASS_MOTORCYCLE,
	MCYCLE_OBJECT, init_mcycle_object,0	},

    { "SHELL",	"shell",
	OBJECTCLASS_DYNAMIC, SHELL_OBJECT,
	init_shell_object,0	},

    /* This one must be last */
    { "",		 "",
	0, INVALID,			NULL		,0	}
};
