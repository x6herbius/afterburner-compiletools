#include "texturecollectionloader.h"

#include <utility>
#include <algorithm>
#include "texturecollection.h"
#include "bspfile.h"
#include "log.h"
#include "miptexwrapper.h"

#define WARNING(...) Warning("TextureCollectionLoader: " __VA_ARGS__)

TextureCollectionLoader::TextureCollectionLoader(TextureCollection& collection) :
	m_Collection(collection),
	m_ReadData()
{
}

bool TextureCollectionLoader::load(const void* data, uint32_t length)
{
	clearInternalData();

	m_ReadData.data = reinterpret_cast<const byte*>(data);
	m_ReadData.dataLength = length;

	if ( !m_ReadData.data || m_ReadData.dataLength < sizeof(uint32_t) )
	{
		WARNING("Data to load from was not valid.");
		return false;
	}

	// g_max_map_miptex is the size in bytes.
	hlassume(m_ReadData.dataLength < g_max_map_miptex, assume_MAX_MAP_MIPTEX);
	m_ReadData.count = *reinterpret_cast<const uint32_t*>(m_ReadData.data);

	if ( !populateTextureIndexReorderingMap() )
	{
		return false;
	}

	// TODO
	return false;
}

bool TextureCollectionLoader::populateTextureIndexReorderingMap()
{
	m_TextureIndexReordering.clear();

	// First int for count, plus count * offsets
	const uint32_t headerLength = (m_ReadData.count + 1) * sizeof(uint32_t);
	if ( headerLength > m_ReadData.dataLength )
	{
		WARNING("Data was not long enough to contain %u texture offsets.", m_ReadData.count);
		return false;
	}

	const uint32_t* offsets = reinterpret_cast<const uint32_t*>(m_ReadData.data + sizeof(uint32_t));
	std::vector<IndexOffsetPair> offsetForIndex;

	// 1. Sanity check offsets.
	for ( uint32_t index = 0; index < m_ReadData.count; ++index )
	{
		// Offset begins at 0 - this is invalid, since the offset should at least be past the header.
		// If the actual offset it valid, this 0 is replaced with the actual offset.
		IndexOffsetPair pair(index, 0);

		if ( offsets[index] >= m_ReadData.dataLength )
		{
			WARNING("Offset value of %u for texture index %u exceeds total data size of %u.",
					offsets[index],
					index,
					m_ReadData.dataLength);
		}
		else if ( offsets[index] < headerLength )
		{
			WARNING("Offset value of %u for texture index %u is invalid (expected at least %u).",
					offsets[index],
					index,
					headerLength);
		}
		else
		{
			pair.second = offsets[index];
		}

		offsetForIndex.push_back(pair);
	}

	// 2. Order pairs by offset.
	std::sort(offsetForIndex.begin(), offsetForIndex.end(),
	[](const IndexOffsetPair& a, const IndexOffsetPair& b)
	{
		return a.second < b.second;
	});

	// 3. Flag further invalid entries.
	invalidateTexturesWithInsufficientData(offsetForIndex);

	// 4. Remove any entries with invalid offsets.
	removeInvalidTextures(offsetForIndex);

	// 5. Create the vector to map original index to new index.
	m_TextureIndexReordering.resize(m_ReadData.count, -1);
	m_ReadData.validCount = offsetForIndex.size();

	for ( uint32_t targetIndex = 0; targetIndex < offsetForIndex.size(); ++targetIndex )
	{
		const IndexOffsetPair& pair = offsetForIndex[targetIndex];
		m_TextureIndexReordering[pair.first] = targetIndex;
	}

	return true;
}

void TextureCollectionLoader::removeInvalidTextures(std::vector<IndexOffsetPair>& offsetsForIndex)
{
	std::vector<IndexOffsetPair>::iterator iter;

	for ( iter = offsetsForIndex.begin(); iter != offsetsForIndex.end(); )
	{
		if ( iter->second == 0 )
		{
			iter = offsetsForIndex.erase(iter);
		}
		else
		{
			++iter;
		}
	}
}

// Assumes that the incoming list has been ordered by data offset.
void TextureCollectionLoader::invalidateTexturesWithInsufficientData(std::vector<IndexOffsetPair>& offsetForIndex)
{
	for ( uint32_t index = 0; index < offsetForIndex.size(); ++index )
	{
		IndexOffsetPair& pair = offsetForIndex[index];
		const uint32_t textureIndex = pair.first;
		const uint32_t textureOffset = pair.second;

		if ( textureOffset == 0 )
		{
			// Skip - already invalidated.
			continue;
		}

		const uint32_t nextOffset = index < offsetForIndex.size() - 1
			? offsetForIndex[index + 1].second
			: m_ReadData.dataLength;
		const uint32_t availableSize = nextOffset - textureOffset;

		const miptex_t* miptex = reinterpret_cast<const miptex_t*>(m_ReadData.data + textureOffset);

		if ( miptex->width < 1 || miptex->width % 16 != 0 || miptex->height < 1 || miptex->height % 16 != 0 )
		{
			WARNING("Texture %u has invalid dimensions %ux%u.", index, miptex->width, miptex->height);
			pair.second = 0;
			continue;
		}

		const uint32_t sizeRequired = MiptexWrapper::totalIdealBytesRequired(miptex->width, miptex->height);

		if ( availableSize < sizeRequired )
		{
			WARNING("Texture %u invalid - %u bytes required, but data layout only had space for %u bytes.",
					textureIndex,
					sizeRequired,
					availableSize);

			pair.second = 0;
			continue;
		}

		if ( availableSize > sizeRequired )
		{
			WARNING("Data layout provided texture %u with %u bytes of data, when only %u bytes were required. "
					"Extra bytes are ignored.",
					textureIndex,
					availableSize,
					sizeRequired);
		}
	}
}

void TextureCollectionLoader::clearInternalData()
{
	m_Collection.clear();
	m_TextureIndexReordering.clear();
	m_ReadData = ReadData();
}
