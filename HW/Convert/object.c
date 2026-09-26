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

#include "convert.h"

static int
    DataCount;
static struct Graphic
    Proto = {
        -1, -1,         /* Type, CookedType */
        0, 0, 0, 0, 0,  /* n, m, segment, doneBeenCooked, flags */
        0,              /* SaveFlags */
        0,              /* AnimBits */
        0,              /* TextureID */
        0,              /* Selected */
        {1,1,1},        /* Scale */
        {0,0,0},        /* Rotate */
        {0,0,0},        /* Pos */
        {0,0,0},        /* Color */
        0,              /* Transparency */
        {1,1,1},        /* SpecColor */
        0,              /* Shininess */
        0, 0,           /* Data, Cooked */
        0, 0,           /* Texture, Next */
    };

int PropType( ObjMethod Prop )
{
    int
        Base, Count;

    switch( Prop ) {
    case PR_COLOR :
    case PR_SPEC_COLOR :
    case PR_SCALE :
    case PR_ROTPOS :
    case PR_POSITION :
        return CT_ARRAY | BT_FLOAT | 3;

    case PR_TRANSPARENCY :
    case PR_SHININESS :
        return CT_SCALAR | BT_FLOAT;

    case PR_GRAPHTYPE :
        return CT_SCALAR | BT_STRING;

    case PR_WHICHGRAPH :
        return CT_SCALAR | BT_VOID;

    case PR_GRAPHM :
    case PR_GRAPHN :
    case PR_NORMALS :
    case PR_RGB :
    case PR_UV :
    case PR_SPECULAR :
    case PR_CONVEX :
    case PR_BACKFACE :
    case PR_TWOSIDED :
    case PR_UNCOLORED :
    case PR_TEXTURED :
    case PR_BRIGHT :
    case PR_VISIBLE :
    case PR_COLLIDES :
    case PR_WIREFRAME :
    case PR_TEXGENSPHERE :
    case PR_TEXGENPLANE :
    case PR_TEXGENCYL :
        return CT_SCALAR | BT_INT;

    case PR_VOLUME :
    case PR_ANIMATE :
        return CT_SCALAR | BT_STRING;

    case PR_TEXTURE :
        return CT_SCALAR | BT_OBJECT;

    case PR_DATA :
        Base = CT_ARRAY | BT_FLOAT;
        Count = 3;
        if( Proto.SaveFlags & GF_RGB )  Count += 3;
        if( Proto.SaveFlags & GF_NORMALS )      Count += 3;
        if( Proto.SaveFlags & GF_UV )   Count += 2;

        switch( Proto.Type ) {
        case G_TEXTURE :
        case G_STRING :
            Base = CT_SCALAR | BT_STRING;
            Count = 0;
            break;

        case G_MESH :
            Count *= Proto.n * Proto.m;
            break;

        case G_TRIMESH :
            Count = Count * Proto.n + 3 * Proto.m;
            break;

        case G_POLYGON :
        case G_POLYLINE :
        case G_POLYMARKER :
            Count *= Proto.n;
            break;

        case G_SPHERE :
        case G_CONE :
            Count = 5;
            break;

        case G_RING :
            Count = 4;
            break;

        case G_TORUS :
            Count = 6;
            break;

        case G_GROUP :
            Base = CT_ARRAY | BT_OBJECT;
            Count = Proto.n;
            break;

        case G_BOX :
            Count = 6;
            break;

        case G_SURFREV :
            Count *= Proto.m * 2 + 1;
            break;

        default :
            return -1;
        }
        DataCount = Count * sizeof(float);
        return Base | Count;

    default :
        return -1;
    }
}

int PropVal( struct Object *Obj, ObjMethod Prop, void *Val )
{
    static struct {
        char
            *GraphName;
        int
            GraphType;
    }
        GraphLookup[] = {
            { "Texture", G_TEXTURE },
            { "Mesh", G_MESH },
            { "Polygon", G_POLYGON },
            { "Polyline", G_POLYLINE },
            { "Polymarker", G_POLYMARKER },
            { "Sphere", G_SPHERE },
            { "Cone", G_CONE },
            { "Ring", G_RING },
            { "Torus", G_TORUS },
            { "Group", G_GROUP },
            { "Box", G_BOX },
            { "SurfRev", G_SURFREV },
            { "String", G_STRING },
            { "TriMesh", G_TRIMESH },
            { 0, 0 },
        };
    char
        *s;
    int
        i;
    struct Graphic
        *G, *Curr, *Next;

    switch( Prop ) {
    case PR_GRAPHTYPE :
        s = (char *)Val;
        for( i = 0; GraphLookup[i].GraphName; i++ ) {
            if( StrEQ( GraphLookup[i].GraphName, s ) ) {
                Proto.Type = GraphLookup[i].GraphType;
                free( s );
                return 1;
            }
        }
        free( s );
        return 0;

    case PR_WHICHGRAPH :
        /* Store it */
        G = malloc( sizeof(struct Graphic) );
        if( !G )        OutOfMemory();
        *G = Proto;

        if( Obj->GraphTail ) {
            Obj->GraphTail->Next = G;
        }
        else {
            Obj->Graphic = G;
        }
        Obj->GraphTail = G;

        Proto.Type = -1;
        Proto.n = Proto.m = Proto.SaveFlags = 0;
        Proto.Color[0] = Proto.Color[1] = Proto.Color[2] = 0;
        Proto.Transparency = 0;
        Proto.SpecColor[0] = Proto.SpecColor[1] = Proto.SpecColor[2] = 1;
        Proto.Shininess = 0;
        Proto.Scale[0] = Proto.Scale[1] = Proto.Scale[2] = 1.0;
        Proto.Rotate[0] = Proto.Rotate[1] = Proto.Rotate[2] = 0.0;
        Proto.Pos[0] = Proto.Pos[1] = Proto.Pos[2] = 0.0;
        Proto.Data = Proto.Cooked = 0;
        Proto.Next = 0;
        Proto.Texture = 0;
        Proto.AnimBits = 0xFFFFFFFF;
        Proto.segment = -1;
        DataCount = -1;
        break;

    case PR_GRAPHN :
        Proto.n = *(int *)Val;
        break;

    case PR_GRAPHM :
        Proto.m = *(int *)Val;
        break;

    case PR_NORMALS :
        if( *(int *)Val )       Proto.SaveFlags |= GF_NORMALS;
        else                    Proto.SaveFlags &= ~GF_NORMALS;
        break;

    case PR_RGB :
        if( *(int *)Val )       Proto.SaveFlags |= GF_RGB;
        else                    Proto.SaveFlags &= ~GF_RGB;
        break;

    case PR_UV :
        if( *(int *)Val )       Proto.SaveFlags |= GF_UV;
        else                    Proto.SaveFlags &= ~GF_UV;
        break;

    case PR_CONVEX :
        /* Obsolete - ignored */
        break;

    case PR_BACKFACE :
        if( *(int *)Val )       Proto.SaveFlags |= GF_BACKFACE;
        else                    Proto.SaveFlags &= ~GF_BACKFACE;
        break;

    case PR_TWOSIDED :
        if( *(int *)Val )       Proto.SaveFlags |= GF_TWOSIDED;
        else                    Proto.SaveFlags &= ~GF_TWOSIDED;
        break;

    case PR_UNCOLORED :
        if( *(int *)Val )       Proto.SaveFlags |= GF_UNCOLORED;
        else                    Proto.SaveFlags &= ~GF_UNCOLORED;
        break;

    case PR_TEXTURED :
        if( *(int *)Val )       Proto.SaveFlags |= GF_TEXTURED;
        else                    Proto.SaveFlags &= ~GF_TEXTURED;
        break;

    case PR_SPECULAR :
        if( *(int *)Val ) {
            Proto.Shininess = 0.0625;
            Proto.SpecColor[0] = 1.0;
            Proto.SpecColor[1] = 1.0;
            Proto.SpecColor[2] = 1.0;
        }
        else {
            Proto.Shininess = 0.0;
        }
        break;

    case PR_BRIGHT :
        if( *(int *)Val )       Proto.SaveFlags |= GF_BRIGHT;
        else                    Proto.SaveFlags &= ~GF_BRIGHT;
        break;

    case PR_VISIBLE :
        if( *(int *)Val )       Proto.SaveFlags &= ~GF_INVISIBLE;
        else                    Proto.SaveFlags |= GF_INVISIBLE;
        break;

    case PR_COLLIDES :
        if( *(int *)Val )       Proto.SaveFlags &= ~GF_INTANGIBLE;
        else                    Proto.SaveFlags |= GF_INTANGIBLE;
        break;

    case PR_TEXGENCYL :
        if( *(int *)Val )       Proto.SaveFlags |= GF_TEXGENCYL;
        else                    Proto.SaveFlags &= ~GF_TEXGENCYL;
        break;

    case PR_TEXGENSPHERE :
        if( *(int *)Val )       Proto.SaveFlags |= GF_TEXGENSPHERE;
        else                    Proto.SaveFlags &= ~GF_TEXGENSPHERE;
        break;

    case PR_TEXGENPLANE :
        if( *(int *)Val )       Proto.SaveFlags |= GF_TEXGENPLANE;
        else                    Proto.SaveFlags &= ~GF_TEXGENPLANE;
        break;

    case PR_WIREFRAME :
        if( *(int *)Val )       Proto.SaveFlags |= GF_WIREFRAME;
        else                    Proto.SaveFlags &= ~GF_WIREFRAME;
        break;

    case PR_DATA :
        if( (Proto.Type == G_TEXTURE) || (Proto.Type == G_STRING) ) {
            Proto.Data = Val;
            Proto.n = strlen( (char *)Val ) + 1;
        }
        else if( Proto.Type == G_GROUP ) {
            Proto.Data = malloc( Proto.n * sizeof(struct Object *) );
            if( !Proto.Data )   OutOfMemory();
            (void)memcpy( Proto.Data, Val, Proto.n*sizeof(struct Object *) );
        }
        else {
            if( DataCount <= 0 )        return 0;
            Proto.Data = malloc( DataCount );
            if( !Proto.Data )   OutOfMemory();
            (void)memcpy( Proto.Data, Val, DataCount );
        }
        break;

    case PR_POSITION :
        (void)memcpy( Proto.Pos, Val, 3*sizeof(float) );
        break;

    case PR_ROTPOS :
        Proto.Rotate[0] = ((float *)Val)[0];
        Proto.Rotate[1] = ((float *)Val)[1];
        Proto.Rotate[2] = ((float *)Val)[2];
        break;

    case PR_SCALE :
        (void)memcpy( Proto.Scale, Val, 3*sizeof(float) );
        break;

    case PR_COLOR :
        (void)memcpy( Proto.Color, Val, 3*sizeof(float) );
        break;

    case PR_TRANSPARENCY :
        (void)memcpy( &Proto.Transparency, Val, sizeof(float) );
        break;

    case PR_SPEC_COLOR :
        (void)memcpy( Proto.SpecColor, Val, 3*sizeof(float) );
        break;

    case PR_SHININESS :
        (void)memcpy( &Proto.Shininess, Val, sizeof(float) );
        break;

    case PR_TEXTURE :
        Proto.Texture = Val;
        break;

    case PR_VOLUME :
        Obj->Volume = Val;
        break;

    case PR_ANIMATE :
        s = (char *)Val;
        (void)sscanf( s, "%x", &i );
        Proto.AnimBits = i;
        free( s );
        break;

    default :
        return 0;
    }
    return 1;
}

/*** EOF object.c ***/
