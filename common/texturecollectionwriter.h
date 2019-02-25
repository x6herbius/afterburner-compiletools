#ifndef TEXTURECOLLECTIONWRITER_H
#define TEXTURECOLLECTIONWRITER_H

#include <vector>
#include "mathtypes.h"

class TextureCollection;

class TextureCollectionWriter
{
public:
	TextureCollectionWriter(const TextureCollection& collection);

	bool exportAll();
	const std::vector<byte>& exportedData() const;

private:
	int32_t getNextValidTextureIndex(uint32_t begin) const;

	const TextureCollection& m_Collection;
	std::vector<byte> m_ExportData;
};

#endif // TEXTURECOLLECTIONWRITER_H
