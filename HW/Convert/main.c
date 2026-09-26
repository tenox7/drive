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

struct Object
    *Objects,
    *ObjTail,
    *Textures,
    *TextureTail;

void OutOfMemory( void )
{
    fputs( "Out of memory\n", stderr );
    exit( 1 );
}

main( int argc, char **argv )
{
    hwObject
        obj, other, tm,
        *others,
        *children;
    struct Object
        *ptr,
        **arr;
    struct Graphic
        *gr;
    hwInt32
        v;
    hwFloat
        tmp;
    int
        doBinary = 0,
        i, j, k, n, vn;
    char
        *inName = 0;

    for( i = 1; i < argc; i++ ) {
        if( strcmp( argv[i], "-binary" ) == 0 ) {
            doBinary = 1;
        }
        else if( !inName ) {
            inName = argv[i];
        }
        else {
            (void)fprintf( stderr, "Usage: %s [-binary] infile\n", argv[0] );
            exit( 1 );
        }
    }
    if( !hwInit( argc, argv ) ) {
        exit( 1 );
    }

    if( !inName ) {
        (void)fprintf( stderr, "Usage: %s [-binary] infile\n", argv[0] );
        exit( 1 );
    }

    if( !ParseFile( inName ) )  exit( 1 );

    /* First, create all of the textures */
    for( ptr = Textures; ptr; ptr = ptr->Next ) {
        /* Create the texture object for this object */
        obj = hwTexture->create( hwTexture );
        if( !obj )      OutOfMemory();
        obj->name = ptr->Name;

        /* Copy relevant information */
        gr = ptr->Graphic;
        obj->modify( obj, hwStrFileName, HW_TYPE_STRING, (void *)gr->Data );
        obj->modify( obj, hwStrScale, HW_TYPE_3F, gr->Scale );
        obj->modify( obj, hwStrRotate, HW_TYPE_3F, gr->Rotate );
        obj->modify( obj, hwStrPos, HW_TYPE_3F, gr->Pos );

        /* Save off converted object */
        ptr->converted = obj;
    }

    /* Next, create all of the other objects */
    for( ptr = Objects; ptr; ptr = ptr->Next ) {
        /* Count the children of this group */
        for( n = 0, gr = ptr->Graphic; gr; n++, gr = gr->Next ) {
            /* NOTHING */
        }

        if( n > 1 ) {
            /* Create a group to hold this object */
            obj = hwGroup->create( hwGroup );
            if( !obj )  OutOfMemory();
            obj->name = ptr->Name;

            /* Allocate the child pointer */
            children = malloc( n * sizeof(hwObject) );
            if( !children )     OutOfMemory();
        }
        else {
            obj = 0;
            children = 0;
        }

        /* Create all of the child objects */
        for( n = 0, gr = ptr->Graphic; gr; n++, gr = gr->Next ) {
            other = 0;
            switch( gr->Type ) {
            case G_MESH :
                other = hwMesh->create( hwMesh );
                break;
            case G_POLYGON :
                other = hwPolygon->create( hwPolygon );
                break;
            case G_POLYLINE :
                other = hwPolyline->create( hwPolyline );
                break;
            case G_POLYMARKER :
                other = hwPolymarker->create( hwPolymarker );
                break;
            case G_SPHERE :
                other = hwSphere->create( hwSphere );
                break;
            case G_CONE :
                other = hwCone->create( hwCone );
                break;
            case G_RING :
                if( gr->Data[0] == gr->Data[1] ) {
                    /* It's a hoop! */
                    other = hwDisc->create( hwDisc );
                    gr->SaveFlags |= GF_WIREFRAME;
                }
                else if( (gr->m < 2) && (gr->Data[0] == 0.0)) {
                    /* It's a disc! */
                    other = hwDisc->create( hwDisc );
                }
                else {
                    /* It's a ring! */
                    other = hwRing->create( hwRing );
                }
                break;
            case G_TORUS :
                other = hwTorus->create( hwTorus );
                break;
            case G_BOX :
                other = hwBox->create( hwBox);
                break;
            case G_SURFREV :
                other = hwSurfRev->create( hwSurfRev );
                break;
            case G_GROUP :
                other = hwGroup->create( hwGroup );
                break;
#if 0
            /* Not yet... */
            case G_STRING :
                other = hwText->create( hwText );
                break;
            case G_TRIMESH :
                other = hwTriMesh->create( hwTriMesh );
                break;
#endif
            }
            if( !other ) {
                n--;
                continue;
            }
            other->name = 0;

            /* Store all of the flags, etc. before creation parms */
            HW_MODIFY_1I( other, hwStrGraphN, gr->n );
            HW_MODIFY_1I( other, hwStrGraphM, gr->m );
            HW_MODIFY_1I( other, hwStrVisibility, gr->AnimBits );

            vn = 3;
            v = (gr->SaveFlags & GF_NORMALS) ? HW_TRUE : HW_FALSE;
            HW_MODIFY_1B( other, hwStrHasNormals, v );
            if( v ) vn += 3;

            v = (gr->SaveFlags & GF_RGB) ? HW_TRUE : HW_FALSE;
            HW_MODIFY_1B( other, hwStrHasRGB, v );
            if( v ) vn += 3;

            v = (gr->SaveFlags & GF_UV) ? HW_TRUE : HW_FALSE;
            HW_MODIFY_1B( other, hwStrHasUV, v );
            if( v ) vn += 2;

            v = (gr->SaveFlags & GF_BACKFACE) ? HW_TRUE : HW_FALSE;
            HW_MODIFY_1B( other, hwStrBackface, v );

            v = (gr->SaveFlags & GF_TWOSIDED) ? HW_TRUE : HW_FALSE;
            HW_MODIFY_1B( other, hwStrTwoSided, v );

#if 0
            v = (gr->SaveFlags & GF_UNCOLORED) ? HW_TRUE : HW_FALSE;
            HW_MODIFY_1B( other, hwStrUnColored, v );
#endif

            v = (gr->SaveFlags & GF_BRIGHT) ? HW_TRUE : HW_FALSE;
            HW_MODIFY_1B( other, hwStrBright, v );

            v = (gr->SaveFlags & GF_INVISIBLE) ? HW_TRUE : HW_FALSE;
            HW_MODIFY_1B( other, hwStrInvisible, v );

            v = (gr->SaveFlags & GF_WIREFRAME) ? HW_TRUE : HW_FALSE;
            HW_MODIFY_1B( other, hwStrWireframe, v );

            if( gr->Texture ) {
                tm = gr->Texture->converted;
                if( tm ) {
                    other->modify( other, hwStrTexture, HW_TYPE_OBJECT, tm );
                    if( gr->SaveFlags & GF_TEXGENSPHERE ) {
                        HW_MODIFY_1I( tm, hwStrCoordMode, HW_TM_SPHERE );
                    }
                    if( gr->SaveFlags & GF_TEXGENPLANE ) {
                        HW_MODIFY_1I( tm, hwStrCoordMode, HW_TM_PLANAR );
                    }
                    if( gr->SaveFlags & GF_TEXGENCYL ) {
                        HW_MODIFY_1I( tm, hwStrCoordMode, HW_TM_CYLINDER );
                    }
                }
            }
            other->modify( other, hwStrScale, HW_TYPE_3F, gr->Scale );
            other->modify( other, hwStrRotate, HW_TYPE_3F, gr->Rotate );
            other->modify( other, hwStrPos, HW_TYPE_3F, gr->Pos );
            other->modify( other, hwStrColor, HW_TYPE_3F, gr->Color );
            other->modify( other, hwStrTransparency, HW_TYPE_1F,
                                        &gr->Transparency );
            other->modify( other, hwStrSpecColor, HW_TYPE_3F, gr->SpecColor );
            other->modify( other, hwStrShininess, HW_TYPE_1F, &gr->Shininess );

            /* Set up the data */
            switch( gr->Type ) {
            case G_MESH :
                other->modify( other, hwStrData,
                                HW_MAKE_TYPE(HW_TYPE_FLOAT,gr->n*gr->m*vn),
                                gr->Data );
                break;
            case G_POLYGON :
            case G_POLYMARKER :
                /* Reverse direction of vertices + normals for polygon */
                for( i = 0, k = gr->n - 1; i < k; i++, k-- ) {
                    for( j = 0; j < vn; j++ ) {
                        tmp = gr->Data[i*vn+j];
                        gr->Data[i*vn+j] = gr->Data[k*vn+j];
                        gr->Data[k*vn+j] = tmp;
                    }
                }
                if( gr->Flags & GF_NORMALS ) {
                    if( gr->Flags & GF_RGB ) {
                        for( i = 0; i < gr->n; i++ ) {
                            gr->Data[i*vn + 6] *= -1.0;
                            gr->Data[i*vn + 7] *= -1.0;
                            gr->Data[i*vn + 8] *= -1.0;
                        }
                    }
                    else {
                        for( i = 0; i < gr->n; i++ ) {
                            gr->Data[i*vn + 3] *= -1.0;
                            gr->Data[i*vn + 4] *= -1.0;
                            gr->Data[i*vn + 5] *= -1.0;
                        }
                    }
                }
                other->modify( other, hwStrData,
                                HW_MAKE_TYPE(HW_TYPE_FLOAT,gr->n*vn),
                                gr->Data );
                break;
            case G_POLYLINE :
                other->modify( other, hwStrData,
                                HW_MAKE_TYPE(HW_TYPE_FLOAT,3*gr->n),
                                gr->Data );
                break;
            case G_SPHERE :
                HW_MODIFY_1F( other, hwStrRadius, gr->Data[0] );
                HW_MODIFY_2F( other, hwStrLatRange, gr->Data[1], gr->Data[2] );
                HW_MODIFY_2F( other, hwStrLonRange, gr->Data[3], gr->Data[4] );
                break;
            case G_CONE :
                HW_MODIFY_2F( other, hwStrRadius, gr->Data[0], gr->Data[1] );
                HW_MODIFY_1F( other, hwStrHeight, gr->Data[2] );
                HW_MODIFY_2F( other, hwStrLonRange, gr->Data[3], gr->Data[4] );
                break;
            case G_RING :
                if( gr->Data[0] == gr->Data[1] ) {
                    /* It's a hoop! */
                    HW_MODIFY_1F( other, hwStrRadius, gr->Data[1] );
                }
                else if( (gr->m < 2) && (gr->Data[0] == 0.0)) {
                    /* It's a disc! */
                    HW_MODIFY_1F( other, hwStrRadius, gr->Data[1] );
                }
                else {
                    /* It's a ring! */
                    HW_MODIFY_2F( other, hwStrRadius, gr->Data[0],
                                                        gr->Data[1] );
                }
                HW_MODIFY_2F( other, hwStrLonRange, gr->Data[2], gr->Data[3] );
                break;
            case G_TORUS :
                HW_MODIFY_2F( other, hwStrRadius, gr->Data[0], gr->Data[1] );
                HW_MODIFY_2F( other, hwStrLatRange, gr->Data[4], gr->Data[5] );
                HW_MODIFY_2F( other, hwStrLonRange, gr->Data[2], gr->Data[3] );
                break;
            case G_BOX :
                other->modify( other, hwStrData,
                                        HW_MAKE_TYPE(HW_TYPE_FLOAT,6),
                                        gr->Data );
                break;
            case G_SURFREV :
                HW_MODIFY_2F( other, hwStrLonRange, 0.0, gr->Data[0] );
                other->modify( other, hwStrData,
                                HW_MAKE_TYPE(HW_TYPE_FLOAT,2*gr->m),
                                gr->Data + 1 );
                break;
            case G_GROUP :
                others = malloc( gr->n * sizeof(hwObject) );
                if( !others ) OutOfMemory();
                arr = (struct Object **)gr->Data;
                for( i = 0; i < gr->n; i++ ) {
                    others[i] = arr[i]->converted;
                }
                other->modify( other, hwStrChildren,
                                HW_MAKE_TYPE(HW_TYPE_OBJECT,gr->n),
                                others );
                free( others );
                break;
            case G_STRING :
                /* TBD */
                break;
            case G_TRIMESH :
                /* TBD */
                break;
            }

            if( children ) {
                /* Save that object! */
                children[n] = other;
            }
            else {
                /* Save it *as* the object, and set its name */
                obj = other;
                obj->name = ptr->Name;
            }
        }

        if( n > 1 ) {
            obj->modify( obj, hwStrChildren,
                        HW_MAKE_TYPE(HW_TYPE_OBJECT,n),
                        children );
            free( children );
        }

        ptr->converted = obj;
    }

    /* Finally!  Save all objects. */
    if( doBinary ) {
#if 0   /* TBD */
        (void)hwBeginBinary( stdout );

        for( ptr = Textures; ptr; ptr = ptr->Next ) {
            hwWriteBinary( ptr->converted, stdout );
        }
        for( ptr = Objects; ptr; ptr = ptr->Next ) {
            hwWriteBinary( ptr->converted, stdout );
        }

        (void)hwEndBinary( stdout );
#endif
    }
    else {
        for( ptr = Textures; ptr; ptr = ptr->Next ) {
            hwWriteAscii( ptr->converted, stdout );
        }
        for( ptr = Objects; ptr; ptr = ptr->Next ) {
            hwWriteAscii( ptr->converted, stdout );
        }
    }

    /* All done! */
    return 0;
}
