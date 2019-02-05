#include "bsptextures.h"
#include "messages.h"
#include "log.h"

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
	dmiptexlump_t* const header = (dmiptexlump_t*)texData;
	byte* const firstByteAfterHeader = (byte*)(&header->dataofs[count - 1]) + sizeof(header->dataofs[count - 1]);

	if ( texData + offset < firstByteAfterHeader )
	{
		return NULL;
	}

	return (miptex_t*)(texData[offset]);
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
