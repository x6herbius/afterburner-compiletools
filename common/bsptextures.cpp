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
	hlassume( g_max_map_miptex > length, assume_MAX_MAP_MIPTEX );

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

void BSPTextures_Reduce(byte* const texData, int& size, const unsigned int newCount)
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
	BSPTextures_SetLumpSizeViaPointerComparison(g_dtexdata, g_texdatasize, newDataBase + dataSize);
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
