#ifndef BSPTEXTURES_H
#define BSPTEXTURES_H

#include "mathlib.h"
#include "bspfile.h"

unsigned int BSPTextures_TextureCount(const byte* const texData, const int size);
int BSPTextures_GetTextureOffset(const byte* const texData, const int size, const unsigned int index);
miptex_t* BSPTextures_GetTexture(byte* const texData, const int size, const unsigned int index);

// This is a little redundant, but it makes sure we funnel things through one function.
void BSPTextures_SetLumpSize(const byte* const texData, int& size, const int newSize);
void BSPTextures_SetLumpSizeViaPointerComparison(const byte* const texData, int& size, const byte* const offsetPointer);
void BSPTextures_IncrementLumpSize(const byte* const texData, int& size, int increment);

// Also sets size.
void BSPTextures_SetLumpData(byte* const texData, int& size, const int lumpId, const dheader_t* const header);

#endif // BSPTEXTURES_H
