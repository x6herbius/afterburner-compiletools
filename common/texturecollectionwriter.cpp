#include "texturecollectionwriter.h"
#include "texturecollection.h"
#include "miptexwrapper.h"
#include "hlassert.h"

TextureCollectionWriter::TextureCollectionWriter(const TextureCollection& collection) :
	m_Collection(collection)
{
}

bool TextureCollectionWriter::exportAll()
{
	const uint32_t count = m_Collection.exportableTextureCount();
	const uint32_t textureBytesRequired = m_Collection.exportBytesRequired();

	// int for overall count, plus count * int for offsets, plus data.
	const size_t totalBytesRequired = ((count + 1) * sizeof(uint32_t)) + textureBytesRequired;
	m_ExportData.resize(totalBytesRequired);

	uint32_t* headerCount = reinterpret_cast<uint32_t*>(m_ExportData.data());
	uint32_t* headerOffsets = &headerCount[1];
	byte* data = reinterpret_cast<byte*>(&headerOffsets[count]);

	*headerCount = count;

	uint32_t nextTextureIndex = 0;
	for ( uint32_t outputIndex = 0; outputIndex < count; ++outputIndex )
	{
		const int32_t validTextureIndex = getNextValidTextureIndex(nextTextureIndex);

		// We should never get to the point where there is no valid texture to write.
		// If this happens, it means exportableTextureCount() was returned incorrectly.
		hlassert(validTextureIndex >= 0);

		const uint32_t offset = data - m_ExportData.data();
		headerOffsets[outputIndex] = offset;

		miptex_t* miptex = reinterpret_cast<miptex_t*>(data);
		const MiptexWrapper* wrapper = m_Collection.miptexAt(validTextureIndex);
		hlassert(wrapper);

		const uint32_t exportBytes = wrapper->exportDataSize();
		hlassert(exportBytes > 0);
		hlassert(offset + exportBytes <= m_ExportData.size());

		wrapper->exportToMiptex(miptex);

		data += exportBytes;
		nextTextureIndex = validTextureIndex + 1;
	}

	return true;
}

const std::vector<byte>& TextureCollectionWriter::exportedData() const
{
	return m_ExportData;
}

int32_t TextureCollectionWriter::getNextValidTextureIndex(uint32_t begin) const
{
	for ( uint32_t index = begin; index < m_Collection.count(); ++index )
	{
		const MiptexWrapper* wrapper = m_Collection.miptexAt(index);
		hlassert(wrapper);

		if ( wrapper->canExport() )
		{
			return index;
		}
	}

	return -1;
}
