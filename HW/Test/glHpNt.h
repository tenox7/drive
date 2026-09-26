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

#ifndef __glHpNt_h_
#define __glHpNt_h_

#ifdef __cplusplus
extern "C" {
#endif

/*  (c) Copyright Hewlett-Packard Company, 1997-2000.  All rights are reserved.
    (c) Copyright Silicon Graphics Inc., 1996.    All rights are reserved.
    Copying or other  reproduction  of this  program  except for  archival
    purposes  is  prohibited   without  the  prior   written   consent  of
    Hewlett-Packard Company.


                         RESTRICTED RIGHTS LEGEND

    Use,  duplication, or disclosure by the U.S.  Government is subject to
    restrictions as set forth in subdivision (b) (3) (ii) of the Rights in
    Technical Data and Computer Software clause at 52.227-7013.

              HEWLETT-PACKARD COMPANY Fort Collins, Colorado
*/

/* 1.1 Extensions */
#define GL_EXT_generate_mipmap                  1
#define GL_EXT_texture_border_clamp             1
#define GL_EXT_texture_edge_clamp               1
#define GL_EXT_shadow                           1
#define GL_EXT_depth_texture                    1
#define GL_EXT_texture3D                        1
#define GL_EXT_rescale_normal                   1
#define GL_HP_texture_lighting                  1
#define GL_HP_draw_array_set                    1
#define GL_HP_occlusion_test                    1
#define GL_HP_visibility_test                   1

/* 1.0 Extensions */
#define GL_EXT_polygon_offset                   1
#define GL_EXT_subtexture                       1


/*  Extension: GL_EXT_texture3D */

#define GL_UNPACK_SKIP_IMAGES_EXT               0x806D
#define GL_UNPACK_IMAGE_HEIGHT_EXT              0x806E
#define GL_TEXTURE_WRAP_R_EXT                   0x8072
#define GL_PACK_SKIP_IMAGES_EXT                 0x806B
#define GL_UNPACK_SKIP_IMAGES_EXT               0x806D
#define GL_PACK_IMAGE_HEIGHT_EXT                0x806C
#define GL_UNPACK_IMAGE_HEIGHT_EXT              0x806E
#define GL_MAX_3D_TEXTURE_SIZE_EXT              0x8073
#define GL_TEXTURE_BINDING_3D_EXT               0x806A
#define GL_TEXTURE_DEPTH_EXT                    0x8071
#define GL_TEXTURE_3D_EXT                       0x806F
#define GL_PROXY_TEXTURE_3D_EXT                 0x8070


/* Extension: GL_HP_texture_lighting */

#define GL_TEXTURE_LIGHTING_MODE_HP             0x8167
#define GL_TEXTURE_POST_SPECULAR_HP             0x8168
#define GL_TEXTURE_PRE_SPECULAR_HP              0x8169

/* These next two are OBSOLETE!  Do not use! */
#define GL_TEXTURE_POST_SPECULAR_HP_OLD         0x2102
#define GL_TEXTURE_PRE_SPECULAR_HP_OLD          0x2103


/* Extension: GL_EXT_texture_border_clamp */ 

#define GL_CLAMP_TO_BORDER_EXT                  0x812D


/* Extension: GL_HP_occlusion_test */

#define GL_OCCLUSION_TEST_HP                    0x8165
#define GL_OCCLUSION_RESULT_HP                  0x8166

/* These next two are OBSOLETE!  Do not use! */
#define GL_OCCLUSION_TEST_HP_OLD                0x816E
#define GL_OCCLUSION_RESULT_HP_OLD              0x816F



/* Extension: GL_HP_visibility_test */

#define GL_VISIBILITY_TEST_HP                   0x837a

/* function prototype typedef for glVisibilityBufferHP */
typedef void (APIENTRY * PFNGLVISIBILITYBUFFERHPPROC)  
        (GLsizei size, GLboolean *buffer, GLboolean waitOnGet);

/* function prototype typedef for glNextVisibilityTestHP */
typedef void (APIENTRY * PFNGLNEXTVISIBILITYTESTHPPROC)  (void);


/* Extension: GL_HP_draw_array_set */

/* function prototype typedef for glDrawArraySetHP */
typedef void (APIENTRY * PFNGLDRAWARRAYSETHPPROC)  
    (GLenum mode, const GLint * list, GLsizei count);


/* Extension: GL_EXT_generate_mipmap */

#define GL_GENERATE_MIPMAP_EXT                  0x8191
#define GL_GENERATE_MIPMAP_HINT_EXT             0x8192


/* Extension: GL_EXT_texture_edge_clamp */

#define GL_CLAMP_TO_EDGE_EXT                    0x812F


/* Extension: GL_EXT_shadow        */
/*            GL_EXT_depth_texture */

#define GL_TEXTURE_COMPARE_EXT                  0x819A
#define GL_TEXTURE_COMPARE_OPERATOR_EXT         0x819B
#define GL_DEPTH_COMPONENT16_EXT                0x81A5
#define GL_DEPTH_COMPONENT24_EXT                0x81A6
#define GL_DEPTH_COMPONENT32_EXT                0x81A7
#define GL_TEXTURE_LEQUAL_R_EXT                 0x819C
#define GL_TEXTURE_GEQUAL_R_EXT                 0x819D


/* Extension: GL_EXT_rescale_normal */

#define GL_RESCALE_NORMAL_EXT                   0x803A




#ifdef __cplusplus
}
#endif

#endif /* __glHpNt_h_ */
