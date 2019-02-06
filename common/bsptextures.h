#ifndef BSPTEXTURES_H
#define BSPTEXTURES_H

#include "mathlib.h"
#include "bspfile.h"

void BSPTextures_Init(byte*& texData, int& size, const unsigned int maxSize);
void BSPTextures_Free(byte*& texData, int& size);

unsigned int BSPTextures_TextureCount(const byte* const texData, const int size);
int BSPTextures_GetTextureOffset(const byte* const texData, const int size, const unsigned int index);
miptex_t* BSPTextures_GetTexture(byte* const texData, const int size, const unsigned int index);
byte* BSPTextures_RawDataBase(byte* const texData, const int size);

// This is a little redundant, but it makes sure we funnel things through one function.
void BSPTextures_SetLumpSize(const byte* const texData, int& size, const int newSize);
void BSPTextures_SetLumpSizeViaPointerComparison(const byte* const texData, int& size, const byte* const offsetPointer);
void BSPTextures_IncrementLumpSize(const byte* const texData, int& size, int increment);

// Also sets size.
void BSPTextures_SetLumpData(byte* const texData, int& size, const int lumpId, const dheader_t* const header);

void BSPTextures_ByteSwapAll(byte* const texData, const int size, bool miptexCountNeedsSwap);

// If newSize >= size, nothing happens.
// The texture currently existing at newCount must also have a valid offset.
void BSPTextures_Trim(byte* const texData, int& size, const unsigned int newCount);

void BSPTextures_Reduce(byte* const texData, int& size, texinfo_t* const texInfo, const unsigned int texInfoCount);

#endif // BSPTEXTURES_H
