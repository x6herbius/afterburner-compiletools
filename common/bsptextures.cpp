#include "bsptextures.h"
#include "messages.h"
#include "log.h"
#include "blockmem.h"

void BSPTextures_Init(byte*& texData, int& size, const unsigned int maxSize)
{
	if ( maxSize > 0 )
	{
		texData = (byte*)AllocBlock(maxSize);
		hlassume(texData != NULL, assume_NoMemory);
	}
	else
	{
		texData = NULL;
	}

	size = 0;
}

void BSPTextures_Free(byte*& texData, int& size)
{
	if ( texData )
	{
		FreeBlock(texData);
		texData = NULL;
	}

	size = 0;
}

unsigned int BSPTextures_TextureCount(const byte* const texData, const int size)
{
	return (texData && size > 0) ? ((const dmiptexlump_t *)texData)->nummiptex : 0;
}

int BSPTextures_GetTextureOffset(const byte* const texData, const int size, const unsigned int index)
{
	const unsigned int count = BSPTextures_TextureCount(texData, size);

	if ( !texData || index >= count )
	{
		return -1;
	}

	const dmiptexlump_t* const header = (const dmiptexlump_t*)texData;
	return header->dataofs[index];
}

miptex_t* BSPTextures_GetTexture(byte* const texData, const int size, const unsigned int index)
{
	const unsigned int count = BSPTextures_TextureCount(texData, size);
	const int offset = BSPTextures_GetTextureOffset(texData, size, index);

	if ( !texData || offset < 0 )
	{
		return NULL;
	}

	// Protect as much as we can against invalid offsets.
	// Min offset is the first byte after the header.
	byte* const firstByteAfterHeader = BSPTextures_RawDataBase(texData, size);

	if ( !firstByteAfterHeader || texData + offset < firstByteAfterHeader )
	{
		return NULL;
	}

	return (miptex_t*)(texData[offset]);
}

byte* BSPTextures_RawDataBase(byte* const texData, const int size)
{
	if ( !texData || size < 1 )
	{
		return NULL;
	}

	const unsigned int count = BSPTextures_TextureCount(texData, size);
	if ( count < 1 )
	{
		return NULL;
	}

	dmiptexlump_t* const header = (dmiptexlump_t*)texData;
	return (byte*)(&header->dataofs[count - 1]) + sizeof(header->dataofs[count - 1]);
}

void BSPTextures_SetLumpSize(const byte* const texData, int& size, const int newSize)
{
	size = newSize;
}

void BSPTextures_IncrementLumpSize(const byte* const texData, int& size, int increment)
{
	size += increment;
}

void BSPTextures_SetLumpSizeViaPointerComparison(const byte* const texData, int& size, const byte* const offsetPointer)
{
	if ( !texData || offsetPointer < texData )
	{
		BSPTextures_SetLumpSize(texData, size, 0);
		return;
	}

	BSPTextures_SetLumpSize(texData, size, offsetPointer - texData);
}

void BSPTextures_SetLumpData(byte* const texData, int& size, const int lumpId, const dheader_t* const header)
{
	if ( !texData )
	{
		return;
	}

	if ( !header )
	{
		BSPTextures_SetLumpSize(texData, size, 0);
		return;
	}

	const int length = header->lumps[lumpId].filelen;
	const int offset = header->lumps[lumpId].fileofs;

	if( length < 1 )
	{
		BSPTextures_SetLumpSize(texData, size, 0);
		return;
	}

	// special handling for tex and lightdata to keep things from exploding - KGP
	hlassume(g_max_map_miptex > length, assume_MAX_MAP_MIPTEX);

	memcpy(texData, (const byte*)header + offset, length);
	BSPTextures_SetLumpSize(texData, size, length);
}

void BSPTextures_ByteSwapAll(byte* const texData, const int size, bool miptexCountIsSwapped)
{
	if ( !texData || size < 1 )
	{
		return;
	}

	dmiptexlump_t* const header = (dmiptexlump_t*)texData;
	const int count = miptexCountIsSwapped ? LittleLong(header->nummiptex) : header->nummiptex;

	header->nummiptex = LittleLong(header->nummiptex);

	for(int index = 0; index < count; index++ )
	{
		header->dataofs[index] = LittleLong(header->dataofs[index]);
	}
}

void BSPTextures_Trim(byte* const texData, int& size, const unsigned int newCount)
{
	byte* const oldDataBase = BSPTextures_RawDataBase(texData, size);
	const unsigned int oldCount = BSPTextures_TextureCount(texData, size);

	if ( !oldDataBase || oldCount < 1 || oldCount >= newCount )
	{
		return;
	}

	const int offsetOfFirstNonIncludedTexture = BSPTextures_GetTextureOffset(texData, size, oldCount);

	if ( offsetOfFirstNonIncludedTexture < 0 )
	{
		return;
	}

	// This isn't a great way to determine the size - what if the offsets aren't in
	// ascending order? Should fix at some point, but using because this is what was
	// done before. New method could be to find the last valid miptex whose index is
	// < oldCount, calculate the size of it and add it to its offset.
	const unsigned int dataSize = (texData + offsetOfFirstNonIncludedTexture) - oldDataBase;
	dmiptexlump_t* const header = (dmiptexlump_t*)texData;
	byte* const newDataBase = (byte*)&header->dataofs[oldCount];

	memmove(newDataBase, oldDataBase, dataSize);
	BSPTextures_SetLumpSizeViaPointerComparison(texData, size, newDataBase + dataSize);
	header->nummiptex = newCount;

	// New base is lower. Adjust all offsets to account for this.
	const unsigned int offsetDiff = oldDataBase - newDataBase;

	for (int textureIndex = 0; textureIndex < newCount; ++textureIndex)
	{
		if (header->dataofs[textureIndex] < 0)
		{
			continue;
		}

		header->dataofs[textureIndex] -= offsetDiff;
	}
}

// This is a copy-and-paste job from FinishBSPFile().
void BSPTextures_Reduce(byte* const texData, int& size, texinfo_t* const texInfo, const unsigned int texInfoCount)
{
	dmiptexlump_t* const header = (dmiptexlump_t*)texData;

	// If texture n is referenced somehow, Used[n] will be true.
	bool* Used = (bool *)calloc(header->nummiptex, sizeof(bool));
	hlassume(Used != NULL, assume_NoMemory);

	// Once unused textures have been pruned, the new index of texture n will be Map[n].
	int* Map = (int *)malloc(header->nummiptex * sizeof(int));
	hlassume(Map != NULL, assume_NoMemory);

	// For a texture n, lumpsizes[n] is how much actual data it takes up after its offset.
	int* lumpsizes = (int *)malloc(header->nummiptex * sizeof (int));
	hlassume(lumpsizes != NULL, assume_NoMemory);

	// This is the amount of data taken up by miptex items, excluding the header at the beginning.
	const int newdatasizemax = size - ((byte *)&header->dataofs[header->nummiptex] - (byte *)header);

	// This is an intermediate buffer within which to consolidate the reduced textures.
	byte* newdata = (byte *)malloc(newdatasizemax);
	hlassume(newdata != NULL, assume_NoMemory);

	int Num = 0;
	int Size = 0;
	int newdatasize = 0;
	int total = 0;

	// For each texture, calculate the amount of data it uses via a long and tedious method.
	for (int i = 0; i < header->nummiptex; i++)
	{
		// If invalid, mark lump size as invalid as well.
		if (header->dataofs[i] == -1)
		{
			lumpsizes[i] = -1;
			continue;
		}

		// lumpsizes initially holds the amount of data from the beginning of this texture to the end of the block.
		// This assumes that the current texture spans all of that data.
		lumpsizes[i] = size - header->dataofs[i];

		// Check all other textures against this one.
		for (int j = 0; j < header->nummiptex; j++)
		{
			// Calculate the distance of the beginning of the other texture from the beginning of this one.
			int offsetDelta = header->dataofs[j] - header->dataofs[i];

			// Ignore if the other texture offset isn't after this one.
			if (header->dataofs[j] == -1 || offsetDelta < 0 || offsetDelta == 0 && j <= i)
			{
				continue;
			}

			// The other texture's offset sits after this one.
			// If it's closer than the closest offset we've found so far, this means that
			// the amount of data used by this texture is smaller than our previous estimate.
			if (offsetDelta < lumpsizes[i])
			{
				lumpsizes[i] = offsetDelta;
			}
		}

		// By this point, lumpsizes[i] will correspond to the number of bytes used by this texture.

		// Keep a count of the sizes of each texture.
		total += lumpsizes[i];
	}

	// We expect the sizes of all the textures to add up to the pre-calculated size.
	// If they don't, something is wrong.
	if (total != newdatasizemax)
	{
		Warning("Bad texdata structure.\n");
		goto skipReduceTexdata;
	}

	// Check each texinfo and mark the texture it uses.
	// The Used array will then show which textures are
	// not referenced by any surface.
	for (int i = 0; i < texInfoCount; i++)
	{
		texinfo_t* t = &texInfo[i];

		if (t->miptex < 0 || t->miptex >= header->nummiptex)
		{
			Warning("Bad miptex number %d.\n", t->miptex);
			goto skipReduceTexdata;
		}

		Used[t->miptex] = true;
	}

	// Then iterate over each texture again.
	// I'm assuming that this checks for textures that are part of an
	// animating/tiling set, and marks them as used too.
	for (int i = 0; i < header->nummiptex; i++)
	{
		const int MAXWADNAME = 16;
		char name[MAXWADNAME];

		// Skip invalid textures.
		if (header->dataofs[i] < 0)
		{
			continue;
		}

		// Skip unreferenced textures
		if (!Used[i])
		{
			continue;
		}

		miptex_t *m = (miptex_t *)((byte *)header + header->dataofs[i]);

		// Skip textures that don't begin with + or -.
		if (m->name[0] != '+' && m->name[0] != '-')
		{
			continue;
		}

		safe_strncpy (name, m->name, MAXWADNAME);

		// Skip textures that don't have a second character.
		if (name[1] == '\0')
		{
			continue;
		}

		// Loop over all characters 0-9 and A-J.
		for (int j = 0; j < 20; j++)
		{
			if (j < 10)
			{
				name[1] = '0' + j;
			}
			else
			{
				name[1] = 'A' + j - 10;
			}

			// For this name (eg. "+4..."), check against all other textures.
			// If the texture matches this name, it's used.
			for (int k = 0; k < header->nummiptex; k++)
			{
				if (header->dataofs[k] < 0)
				{
					continue;
				}

				miptex_t *m2 = (miptex_t *)((byte *)header + header->dataofs[k]);

				if (!strcasecmp(name, m2->name))
				{
					Used[k] = true;
				}
			}
		}
	}

	// Now go over everything again and map each texture to a new index.
	// This is done by ignoring unused textures.
	for (int i = 0; i < header->nummiptex; i++)
	{
		if (Used[i])
		{
			Map[i] = Num;
			Num++;
		}
		else
		{
			Map[i] = -1;
		}
	}

	// Update the texinfo with the new texture indices.
	for (int i = 0; i < texInfoCount; i++)
	{
		texinfo_t *t = &texInfo[i];
		t->miptex = Map[t->miptex];
	}

	// Size will become the new size of the entire texture block.
	// It begins as big as the header itself.
	Size += (byte *)&header->dataofs[Num] - (byte *)header;

	// Now go over every texture AGAIN and copy its data into the newdata buffer to make one contiguous block.
	for (int i = 0; i < header->nummiptex; i++)
	{
		// If the texture is unused, skip it.
		if (!Used[i])
		{
			continue;
		}

		// If the texture had no data before anyway, give it an invalid offset.
		if (lumpsizes[i] == -1)
		{
			header->dataofs[Map[i]] = -1;
			continue;
		}

		// Copy the texture's data into the new array.
		// newdata + newdatasize points to the next byte after the last copied texture.
		memcpy((byte *)newdata + newdatasize, (byte *)header + header->dataofs[i], lumpsizes[i]);

		// Update the offset for the texture we just copied.
		header->dataofs[Map[i]] = Size;

		// Keep track of the size we've copied in so far.
		newdatasize += lumpsizes[i];
		Size += lumpsizes[i];
	}

	// Finally, we can copy the intermediate buffer back into the main texture buffer.
	memcpy(&header->dataofs[Num], newdata, newdatasize);

	Log("Reduced %d texdatas to %d (%d bytes to %d)\n", header->nummiptex, Num, size, Size);

	header->nummiptex = Num;
	BSPTextures_SetLumpSize(texData, size, Size);

	skipReduceTexdata:
	free(lumpsizes);
	free(newdata);
	free(Used);
	free(Map);
}
