#ifndef BSPTEXTURES_H
#define BSPTEXTURES_H

#include "mathlib.h"
#include "bspfile.h"

unsigned int BSPTextures_TextureCount(byte* const texData, const int size);
int BSPTextures_GetTextureOffset(byte* const texData, const int size, const unsigned int index);
miptex_t* BSPTextures_GetTexture(byte* const texData, const int size, const unsigned int index);

#endif // BSPTEXTURES_H
