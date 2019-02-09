#ifndef TEXTURECOLLECTION_H
#define TEXTURECOLLECTION_H

#include <vector>
#include <memory>
#include "cppmemory.h"

// Need functionality for:
// - Exporting lump data, with optional byte swapping
// - Importing lump data, with optional byte swapping
// - Getting number of textures in list
// - Getting a texture by index
// - Returning the type of the texture at the given index
// - Creating textures with arbitrary user data
// - Filtering the list and outputting a map of old -> new
//   texture indices

// Don't bother about checking for offsets into WAD - remove ZHLT_CHART_WADFILES

class TextureCollection
{
public:
	class Entry;
	enum class EntryType
	{
		Undefined = -1,
		Miptex = 0,
		PngOnDisk
	};

	TextureCollection();
	~TextureCollection();

private:
	typedef std::unique_ptr<Entry> EntryPtr;
	typedef Mem::vector<EntryPtr> EntryList;

	EntryList m_Items;
};

#endif // TEXTURECOLLECTION_H
