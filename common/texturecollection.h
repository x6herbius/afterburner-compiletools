#ifndef TEXTURECOLLECTION_H
#define TEXTURECOLLECTION_H

#include <vector>
#include <memory>
#include "cppmemory.h"
#include "hlassert.h"

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
	TextureCollection();
	~TextureCollection();

private:
	struct Entry;
	typedef Mem::vector<std::unique_ptr<Entry>> EntryList;

	EntryList m_Items;
};

#endif // TEXTURECOLLECTION_H
