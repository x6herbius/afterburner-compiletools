#ifndef TEXTURECOLLECTIONLOADER_H
#define TEXTURECOLLECTIONLOADER_H

#include <cstring>
#include <vector>
#include "mathtypes.h"

class TextureCollection;

class TextureCollectionLoader
{
public:
	TextureCollectionLoader(TextureCollection& collection);

	bool load(const void* data, uint32_t length);

private:
	typedef std::pair<uint32_t, uint32_t> IndexOffsetPair;

	struct TextureIndexRecord
	{
		uint32_t index;
		uint32_t offset;
		uint32_t size;
	};

	struct ReadData
	{
		inline ReadData() = default;
		inline ReadData& operator =(const ReadData& other)
		{
			memcpy(this, &other, sizeof(*this));
		}

		const byte* data = NULL;
		uint32_t dataLength = 0;
		uint32_t count = 0;
		uint32_t validCount = 0;
	};

	void clearInternalData();
	bool populateTextureIndexReorderingMap();
	void invalidateTexturesWithInsufficientData(std::vector<IndexOffsetPair>& offsetForIndex);
	void removeInvalidTextures(std::vector<IndexOffsetPair>& offsetsForIndex);

	TextureCollection& m_Collection;
	ReadData m_ReadData;
	std::vector<int32_t> m_TextureIndexReordering;
};

#endif // TEXTURECOLLECTIONSERIALISER_H
