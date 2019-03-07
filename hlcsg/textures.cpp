#include "csg.h"
#include "texturecollection.h"
#include "texturecollectionreader.h"
#include "pngtexture.h"

#define MAXWADNAME 16

#ifdef HLCSG_TEXMAP64_FIX
// The old buggy code in effect limit the number of brush sides to MAX_MAP_BRUSHES
#ifdef HLCSG_HLBSP_REDUCETEXTURE
static char *texmap[MAX_INTERNAL_MAP_TEXINFO];
#else
static char *texmap[MAX_MAP_TEXINFO];
#endif
static int numtexmap = 0;

static int texmap_store (char *texname, bool shouldlock = true)
	// This function should never be called unless a new entry in g_texinfo is being allocated.
{
	int i;
	if (shouldlock)
	{
		ThreadLock ();
	}
#ifdef HLCSG_HLBSP_REDUCETEXTURE
	hlassume (numtexmap < MAX_INTERNAL_MAP_TEXINFO, assume_MAX_MAP_TEXINFO); // This error should never appear.
#else
	hlassume (numtexmap < MAX_MAP_TEXINFO, assume_MAX_MAP_TEXINFO); // This error should never appear.
#endif
	i = numtexmap;
	texmap[numtexmap] = strdup (texname);
	numtexmap++;
	if (shouldlock)
	{
		ThreadUnlock ();
	}
	return i;
}

static char *texmap_retrieve (int index)
{
	hlassume (0 <= index && index < numtexmap, assume_first);
	return texmap[index];
}

static void texmap_clear ()
{
	int i;
	ThreadLock ();
	for (i = 0; i < numtexmap; i++)
	{
		free (texmap[i]);
	}
	numtexmap = 0;
	ThreadUnlock ();
}
#else
// fix for 64 bit machines
#if /* 64 bit */
    static char* texmap64[MAX_MAP_BRUSHES];
    static int   tex_max64=0;

    static inline int texmap64_store(char *texname)
    {
        int curr_tex;
        ThreadLock();
        if (tex_max64 >= MAX_MAP_BRUSHES)   // no assert?
        {
#ifdef ZHLT_CONSOLE
			Error ("MAX_MAP_BRUSHES exceeded!");
#else
            printf("MAX_MAP_BRUSHES exceeded!\n");
            exit(-1);
#endif
        }
        curr_tex = tex_max64;
        texmap64[tex_max64] = texname;
        tex_max64++;
        ThreadUnlock();
        return curr_tex;
    }

    static inline char* texmap64_retrieve( int index)
    {
        if(index > tex_max64)
        {
#ifdef ZHLT_CONSOLE
			Error ("retrieving bogus texture index %d", index);
#else
            printf("retrieving bogus texture index %d\n", index);
            exit(-1);
#endif
        }
        return texmap64[index];
    }

#else
    #define texmap64_store( A ) ( (int) A)
    #define texmap64_retrieve( A ) ( (char*) A)
#endif
#endif

// =====================================================================================
//  CleanupName
// =====================================================================================
static void     CleanupName(const char* const in, char* out)
{
    int             i;

    for (i = 0; i < MAXWADNAME; i++)
    {
        if (!in[i])
        {
            break;
        }

        out[i] = toupper(in[i]);
    }

    for (; i < MAXWADNAME; i++)
    {
        out[i] = 0;
    }
}

// =====================================================================================
//  FindMiptex
//      Find and allocate a texture into the lump data
// =====================================================================================
static int FindMiptex(const char* const name)
{
    int32_t index = TextureDirectoryListing::INVALID_TEXTURE_INDEX;

    ThreadLock();
    index = g_TexDirListing.assignNextTextureIndex(name);
    ThreadUnlock();

    return index;
}

#ifdef ZHLT_PARANOIA_BSP
// =====================================================================================
//  FaceinfoForTexinfo
// =====================================================================================
short FaceinfoForTexinfo( const char *landname, const int in_texture_step, const int in_max_extent, const int groupid )
{
	byte	texture_step = bound( MIN_CUSTOM_TEXTURE_STEP, in_texture_step, MAX_CUSTOM_TEXTURE_STEP );
	byte	max_extent = bound( MIN_CUSTOM_SURFACE_EXTENT, in_max_extent, MAX_CUSTOM_SURFACE_EXTENT );
	short	i;

	ThreadLock();

	dfaceinfo_t *fi = g_dfaceinfo;

	for( i = 0; i < g_numfaceinfo; i++, fi++ )
	{
		if( stricmp( landname, fi->landname) != 0 )
		{
			continue;
		}

		if( fi->texture_step != texture_step )
		{
			continue;
		}

		if( fi->max_extent != max_extent )
		{
			continue;
		}

		if( fi->groupid != groupid )
		{
			continue;
		}

		ThreadUnlock();
		return i;
	}

	hlassume( g_numfaceinfo < MAX_MAP_FACEINFO, assume_MAX_FACEINFO );

	// allocate tne entry
	CleanupName( landname, fi->landname );
	fi->texture_step = texture_step;
	fi->max_extent = max_extent;
	fi->groupid = groupid;

	g_numfaceinfo++;
	ThreadUnlock();

	return i;
}
#endif

// =====================================================================================
//  WriteMiptex
// =====================================================================================
void WriteMiptex()
{
   for ( auto iterator = g_TexDirListing.mapBegin(); iterator != g_TexDirListing.mapEnd(); ++iterator )
   {
        if ( iterator->second == TextureDirectoryListing::INVALID_TEXTURE_INDEX )
        {
            continue;
        }

        const uint32_t newIndex = g_TextureCollection.count();
        g_TextureCollection.allocateAndAppend(1, TextureCollection::ItemType::PngOnDisk);

        PNGTexture* const texture = g_TextureCollection.pngTextureAt(newIndex);
        texture->setPath(iterator->first);
   }
}

//==========================================================================

// =====================================================================================
//  TexinfoForBrushTexture
// =====================================================================================
int             TexinfoForBrushTexture(const plane_t* const plane, brush_texture_t* bt, const vec3_t origin
#ifdef ZHLT_HIDDENSOUNDTEXTURE
					, bool shouldhide
#endif
					)
{
    vec3_t          vecs[2];
    int             sv, tv;
    vec_t           ang, sinv, cosv;
    vec_t           ns, nt;
    texinfo_t       tx;
    texinfo_t*      tc;
    int             i, j, k;

#ifdef HLCSG_HLBSP_VOIDTEXINFO
	if (!strncasecmp(bt->name, BRUSHKEY_NULL, sizeof(BRUSHKEY_NULL) - 1))
	{
		return -1;
	}
#endif
    memset(&tx, 0, sizeof(tx));
#ifndef HLCSG_CUSTOMHULL
#ifdef HLCSG_PRECISIONCLIP
	if(!strncmp(bt->name,BRUSHKEY_BEVEL,sizeof(BRUSHKEY_BEVEL) - 1))
	{
		tx.flags |= TEX_BEVEL;
		safe_strncpy(bt->name,BRUSHKEY_NULL,sizeof(BRUSHKEY_NULL));
	}
#endif
#endif

#ifdef HLCSG_TEXMAP64_FIX
	FindMiptex (bt->name);
#else
    tx.miptex = FindMiptex(bt->name);

    // Note: FindMiptex() still needs to be called here to add it to the global miptex array

    // Very Sleazy Hack 104 - since the tx.miptex index will be bogus after we sort the miptex array later
    // Put the string name of the miptex in this "index" until after we are done sorting it in WriteMiptex().
    tx.miptex = texmap64_store(strdup(bt->name));
#endif

#ifdef ZHLT_PARANOIA_BSP
    tx.faceinfo = bt->faceinfo;
#endif

    // set the special flag
    if (bt->name[0] == '*'
        || !strncasecmp(bt->name, SPECIALTEX_SKY, sizeof(SPECIALTEX_SKY) - 1)

// =====================================================================================
//Cpt_Andrew - Env_Sky Check
// =====================================================================================
        || !strncasecmp(bt->name, SPECIALTEX_ENV_SKY, sizeof(SPECIALTEX_ENV_SKY) - 1)
// =====================================================================================

#ifndef HLCSG_CUSTOMHULL
        || !strncasecmp(bt->name, SPECIALTEX_CLIP, sizeof(SPECIALTEX_CLIP) - 1)
#endif
        || !strncasecmp(bt->name, SPECIALTEX_ORIGIN, sizeof(SPECIALTEX_ORIGIN) - 1)
#ifdef ZHLT_NULLTEX // AJM
        || !strncasecmp(bt->name, SPECIALTEX_NULL, sizeof(SPECIALTEX_NULL) - 1)
#endif
        || !strncasecmp(bt->name, SPECIALTEX_AAATRIGGER, sizeof(SPECIALTEX_AAATRIGGER) - 1)
       )
    {
		// actually only 'sky' and 'aaatrigger' needs this. --vluzacn
        tx.flags |= TEX_SPECIAL;
    }
#ifdef ZHLT_HIDDENSOUNDTEXTURE
	if (shouldhide)
	{
		tx.flags |= TEX_SHOULDHIDE;
	}
#endif

    if (bt->txcommand)
    {
        memcpy(tx.vecs, bt->vects.quark.vects, sizeof(tx.vecs));
        if (origin[0] || origin[1] || origin[2])
        {
            tx.vecs[0][3] += DotProduct(origin, tx.vecs[0]);
            tx.vecs[1][3] += DotProduct(origin, tx.vecs[1]);
        }
    }
    else
    {
        if (g_nMapFileVersion < 220)
        {
            TextureAxisFromPlane(plane, vecs[0], vecs[1]);
        }

        if (!bt->vects.valve.scale[0])
        {
            bt->vects.valve.scale[0] = 1;
        }
        if (!bt->vects.valve.scale[1])
        {
            bt->vects.valve.scale[1] = 1;
        }

        if (g_nMapFileVersion < 220)
        {
            // rotate axis
            if (bt->vects.valve.rotate == 0)
            {
                sinv = 0;
                cosv = 1;
            }
            else if (bt->vects.valve.rotate == 90)
            {
                sinv = 1;
                cosv = 0;
            }
            else if (bt->vects.valve.rotate == 180)
            {
                sinv = 0;
                cosv = -1;
            }
            else if (bt->vects.valve.rotate == 270)
            {
                sinv = -1;
                cosv = 0;
            }
            else
            {
                ang = bt->vects.valve.rotate / 180 * Q_PI;
                sinv = sin(ang);
                cosv = cos(ang);
            }

            if (vecs[0][0])
            {
                sv = 0;
            }
            else if (vecs[0][1])
            {
                sv = 1;
            }
            else
            {
                sv = 2;
            }

            if (vecs[1][0])
            {
                tv = 0;
            }
            else if (vecs[1][1])
            {
                tv = 1;
            }
            else
            {
                tv = 2;
            }

            for (i = 0; i < 2; i++)
            {
                ns = cosv * vecs[i][sv] - sinv * vecs[i][tv];
                nt = sinv * vecs[i][sv] + cosv * vecs[i][tv];
                vecs[i][sv] = ns;
                vecs[i][tv] = nt;
            }

            for (i = 0; i < 2; i++)
            {
                for (j = 0; j < 3; j++)
                {
                    tx.vecs[i][j] = vecs[i][j] / bt->vects.valve.scale[i];
                }
            }
        }
        else
        {
            vec_t           scale;

            scale = 1 / bt->vects.valve.scale[0];
            VectorScale(bt->vects.valve.UAxis, scale, tx.vecs[0]);

            scale = 1 / bt->vects.valve.scale[1];
            VectorScale(bt->vects.valve.VAxis, scale, tx.vecs[1]);
        }

        tx.vecs[0][3] = bt->vects.valve.shift[0] + DotProduct(origin, tx.vecs[0]);
        tx.vecs[1][3] = bt->vects.valve.shift[1] + DotProduct(origin, tx.vecs[1]);
    }

    //
    // find the g_texinfo
    //
    ThreadLock();
    tc = g_texinfo;
    for (i = 0; i < g_numtexinfo; i++, tc++)
    {
        // Sleazy hack 104, Pt 3 - Use strcmp on names to avoid dups
#ifdef HLCSG_TEXMAP64_FIX
		if (strcmp (texmap_retrieve (tc->miptex), bt->name) != 0)
#else
        if (strcmp(texmap64_retrieve((tc->miptex)), texmap64_retrieve((tx.miptex))) != 0)
#endif
        {
            continue;
        }

#ifdef ZHLT_PARANOIA_BSP
        if (tc->faceinfo != tx.faceinfo)
        {
            continue;
        }
#endif

        if (tc->flags != tx.flags)
        {
            continue;
        }
        for (j = 0; j < 2; j++)
        {
            for (k = 0; k < 4; k++)
            {
                if (tc->vecs[j][k] != tx.vecs[j][k])
                {
                    goto skip;
                }
            }
        }
        ThreadUnlock();
        return i;
skip:;
    }

#ifdef HLCSG_HLBSP_REDUCETEXTURE
    hlassume(g_numtexinfo < MAX_INTERNAL_MAP_TEXINFO, assume_MAX_MAP_TEXINFO);
#else
    hlassume(g_numtexinfo < MAX_MAP_TEXINFO, assume_MAX_MAP_TEXINFO);
#endif

    *tc = tx;
#ifdef HLCSG_TEXMAP64_FIX
	tc->miptex = texmap_store (bt->name, false);
#endif
    g_numtexinfo++;
    ThreadUnlock();
    return i;
}

#ifdef HLCSG_HLBSP_VOIDTEXINFO
// Before WriteMiptex(), for each texinfo in g_texinfo, .miptex is a string rather than texture index, so this function should be used instead of GetTextureByNumber.
const char *GetTextureByNumber_CSG(int texturenumber)
{
	if (texturenumber == -1)
		return "";
#ifdef HLCSG_TEXMAP64_FIX
	return texmap_retrieve (g_texinfo[texturenumber].miptex);
#else
	return texmap64_retrieve (g_texinfo[texturenumber].miptex);
#endif
}
#endif
