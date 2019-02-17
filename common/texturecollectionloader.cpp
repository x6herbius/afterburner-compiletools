#include "texturecollectionloader.h"

#include <utility>
#include <algorithm>
#include "texturecollection.h"
#include "bspfile.h"
#include "log.h"
#include "miptexwrapper.h"
#include "hlassert.h"

#define WARNING(...) Warning("TextureCollectionLoader: " __VA_ARGS__)

TextureCollectionLoader::TextureCollectionLoader(TextureCollection& collection) :
	m_Collection(collection),
	m_ReadData()
{
}

bool TextureCollectionLoader::appendMiptex(const void* miptex, uint32_t length, bool headerOnly)
{
	if ( !miptex || length < sizeof(miptex_t) )
	{
		WARNING("Data provided to append() was invalid or too short.");
		return false;
	}

	if ( !validateMiptex(reinterpret_cast<const byte*>(miptex), length, std::string(""), false) )
	{
		return false;
	}

	m_Collection.allocateAndAppend(1, TextureCollection::ItemType::Miptex);

	MiptexWrapper* wrapper = m_Collection.miptexAt(m_Collection.count() - 1);
	hlassert(wrapper);

	return wrapper->setFromMiptex(reinterpret_cast<const miptex_t*>(miptex), headerOnly);
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

	// We're loading from disk, so apparently we have to byte-swap.
	const uint32_t preSwapCount = *reinterpret_cast<const uint32_t*>(m_ReadData.data);
	m_ReadData.count = LittleLong(preSwapCount);

	m_ReadData.validCount = populateTextureIndexReorderingMap();
	if ( m_ReadData.validCount < 1 )
	{
		return false;
	}

	m_Collection.allocateAndAppend(m_ReadData.validCount, TextureCollection::ItemType::Miptex);

	const uint32_t* offsets = reinterpret_cast<const uint32_t*>(m_ReadData.data + sizeof(uint32_t));
	for ( uint32_t origIndex = 0; origIndex < m_TextureIndexReordering.size(); ++origIndex )
	{
		const int32_t newIndex = m_TextureIndexReordering[origIndex];
		if ( newIndex < 0 )
		{
			continue;
		}

		MiptexWrapper* miptexWrapper = m_Collection.miptexAt(newIndex);
		hlassert(miptexWrapper);

		const uint32_t offset = LittleLong(offsets[origIndex]);
		miptexWrapper->setFromMiptex(reinterpret_cast<const miptex_t*>(m_ReadData.data + offset));
	}

	return true;
}

// Returns how many textures were valid.
uint32_t TextureCollectionLoader::populateTextureIndexReorderingMap()
{
	m_TextureIndexReordering.clear();

	// First int for count, plus count * offsets
	const uint32_t headerLength = (m_ReadData.count + 1) * sizeof(uint32_t);
	if ( headerLength > m_ReadData.dataLength )
	{
		WARNING("Data was not long enough to contain %u texture offsets.", m_ReadData.count);
		return 0;
	}

	const uint32_t* offsets = reinterpret_cast<const uint32_t*>(m_ReadData.data + sizeof(uint32_t));
	std::vector<IndexOffsetPair> offsetForIndex;

	// 1. Sanity check offsets. We remove textures with offsets of -1 because they're useless to us -
	// there's no way to even establish the texture name. Actual mipmap data offsets of -1 within the
	// texture are fine, but they're handled by the miptex wrapper later.
	for ( uint32_t index = 0; index < m_ReadData.count; ++index )
	{
		const uint32_t offset = LittleLong(offsets[index]);

		// Offset value here begins at 0. This is invalid, since the offset should at least be past the header.
		// If the actual offset it valid, this 0 is replaced with the actual offset.
		IndexOffsetPair pair(index, 0);

		if ( offset >= m_ReadData.dataLength )
		{
			WARNING("Offset value of %u for texture index %u exceeds total data size of %u.",
					offset,
					index,
					m_ReadData.dataLength);
		}
		else if ( offset < headerLength )
		{
			WARNING("Offset value of %u for texture index %u is invalid (expected at least %u).",
					offset,
					index,
					headerLength);
		}
		else
		{
			pair.second = offset;
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

	for ( uint32_t targetIndex = 0; targetIndex < offsetForIndex.size(); ++targetIndex )
	{
		const IndexOffsetPair& pair = offsetForIndex[targetIndex];
		m_TextureIndexReordering[pair.first] = targetIndex;
	}

	return offsetForIndex.size();
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

		if ( textureOffset < 1 )
		{
			// Skip - already invalidated.
			continue;
		}

		const uint32_t nextOffset = index < offsetForIndex.size() - 1
			? offsetForIndex[index + 1].second
			: m_ReadData.dataLength;
		const uint32_t availableSize = nextOffset - textureOffset;

		if ( !validateMiptex(m_ReadData.data + textureOffset, availableSize, std::string(" ") + std::to_string(index)) )
		{
			pair.second = 0;
		}
	}
}

void TextureCollectionLoader::clearInternalData()
{
	m_Collection.clear();
	m_TextureIndexReordering.clear();
	m_ReadData = ReadData();
}

// indexString is blank if an index is not applicable.
bool TextureCollectionLoader::validateMiptex(const byte* proposedMiptex,
											 uint32_t availableSize,
											 const std::string& indexString,
											 bool availableSizeShouldBeExact)
{
	// Sanity:
	if ( !proposedMiptex || availableSize < 1 )
	{
		WARNING("Internal error. Could not validate texture%s: input data was not valid.", indexString.c_str());
		return false;
	}

	if ( availableSize < sizeof(miptex_t) )
	{
		WARNING("Texture%s invalid: not enough space in buffer to read header.", indexString.c_str());
		return false;
	}

	const miptex_t* const miptex = reinterpret_cast<const miptex_t*>(proposedMiptex);

	if ( miptex->width < 1 || miptex->width % 16 != 0 || miptex->height < 1 || miptex->height % 16 != 0 )
	{
		WARNING("Texture%s has invalid dimensions %ux%u.", indexString.c_str(), miptex->width, miptex->height);
		return false;
	}

	const uint32_t sizeRequired = MiptexWrapper::totalIdealBytesRequired(miptex->width, miptex->height);

	if ( availableSize < sizeRequired )
	{
		WARNING("Texture%s invalid - %u bytes required, but data buffer only had space for %u bytes.",
				indexString.c_str(),
				sizeRequired,
				availableSize);

		return false;
	}

	if ( availableSizeShouldBeExact && availableSize > sizeRequired )
	{
		WARNING("Data layout provided texture%s with %u bytes of data, when only %u bytes were required. "
				"Extra bytes are ignored.",
				indexString.c_str(),
				availableSize,
				sizeRequired);
	}

	return true;
}
