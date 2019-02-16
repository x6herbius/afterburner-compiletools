#ifndef TEXTURECOLLECTION_H
#define TEXTURECOLLECTION_H

#include <vector>
#include <memory>
#include <functional>
#include "cppmemory.h"

// Need functionality for:
// - Exporting lump data
// - Importing lump data
// - Returning whether a texture at a given index is valid
// - Getting number of textures in list - done
// - Getting a texture by index - done
// - Returning the type of the texture at the given index - done
// - Creating textures with arbitrary user data
// - Filtering the list and outputting a map of old -> new
//   texture indices - done

class MiptexWrapper;

class TextureCollection
{
public:
	class Item;
	enum class ItemType
	{
		Undefined = -1,
		Miptex = 0,
		PngOnDisk
	};

	TextureCollection();
	~TextureCollection();

	ItemType itemType(uint32_t index) const;
	uint32_t count() const;
	void clear();

	// Regardless of whether textures are in an exportable state.
	size_t totalBytesInUse() const;

	// Only counts textures that can currently be exported.
	size_t exportBytesRequired() const;

	// If index is out of range, or type does not match, returns NULL.
	MiptexWrapper* miptexAt(uint32_t index);
	const MiptexWrapper* miptexAt(uint32_t index) const;

	// The callback should take a uint32_t index and an item type.
	// It should return true if the item is to be kept, and false otherwise.
	// Output vector will hold a list of mappings from original index to new index.
	// A mapping to -1 means that the item has been removed.
	void filter(const std::function<bool(uint32_t, ItemType)>& callback, std::vector<int32_t>& map);

	bool allocateAndAppend(size_t count, ItemType type);

	// If newCount is >= current count, nothing happens.
	void truncate(size_t newCount);

	int calculateChecksum() const;

private:
	typedef std::shared_ptr<Item> ItemPtr;
	typedef std::vector<ItemPtr> ItemList;

	static ItemPtr createItem(ItemType type);

	void mapItems(const std::vector<int32_t>& map, uint32_t newCount);

	ItemList m_Items;
};

#endif // TEXTURECOLLECTION_H
