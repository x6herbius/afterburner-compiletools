#include "bsptextures.h"

unsigned int BSPTextures_TextureCount(byte* const texData, const int size)
{
	return (texData && size > 0) ? ((dmiptexlump_t *)texData)->nummiptex : 0;
}

int BSPTextures_GetTextureOffset(byte* const texData, const int size, const unsigned int index)
{
	const unsigned int count = BSPTextures_TextureCount(texData, size);

	if ( !texData || index >= count )
	{
		return -1;
	}

	dmiptexlump_t* const header = (dmiptexlump_t*)texData;
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
