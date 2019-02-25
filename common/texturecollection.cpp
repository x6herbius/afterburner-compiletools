#include "texturecollection.h"
#include "miptexwrapper.h"
#include "hlassert.h"

class TextureCollection::Item
{
public:
	Item(ItemType type) :
		m_Type(type)
	{
	}

	virtual ~Item()
	{
	}

	ItemType type() const
	{
		return m_Type;
	}

private:
	ItemType m_Type;
};

class MiptexItem : public TextureCollection::Item
{
public:
	MiptexItem() : TextureCollection::Item(TextureCollection::ItemType::Miptex)
	{
	}

	virtual ~MiptexItem()
	{
	}

	inline MiptexWrapper* miptex()
	{
		return &m_Miptex;
	}

	inline const MiptexWrapper* miptex() const
	{
		return &m_Miptex;
	}

private:
	MiptexWrapper m_Miptex;
};

TextureCollection::TextureCollection() :
	m_Items()
{
}

TextureCollection::~TextureCollection()
{
}

TextureCollection::ItemType TextureCollection::itemType(uint32_t index) const
{
	if ( index >= m_Items.size() )
	{
		return ItemType::Undefined;
	}

	return m_Items[index]->type();
}

uint32_t TextureCollection::count() const
{
	return m_Items.size();
}

MiptexWrapper* TextureCollection::miptexAt(uint32_t index)
{
	if ( itemType(index) != ItemType::Miptex )
	{
		return NULL;
	}

	return static_cast<MiptexItem*>(m_Items[index].get())->miptex();
}

const MiptexWrapper* TextureCollection::miptexAt(uint32_t index) const
{
	if ( itemType(index) != ItemType::Miptex )
	{
		return NULL;
	}

	return static_cast<const MiptexItem*>(m_Items[index].get())->miptex();
}

void TextureCollection::filter(const std::function<bool(uint32_t, TextureCollection::ItemType)>& callback, std::vector<int32_t>& map)
{
	map.clear();

	const uint32_t originalCount = count();
	if ( originalCount < 1 )
	{
		return;
	}

	map.resize(originalCount, -1);
	uint32_t currentRemappedIndex = 0;

	for ( uint32_t index = 0; index < originalCount; ++index )
	{
		if ( callback(index, itemType(index)) )
		{
			map[index] = currentRemappedIndex++;
		}
	}

	mapItems(map, currentRemappedIndex);
}

// Assumes that there are no holes in the map
// (because this should only be called with a map we've generated ourselves).
void TextureCollection::mapItems(const std::vector<int32_t>& map, uint32_t newCount)
{
	if ( newCount < 1 )
	{
		m_Items.clear();
		return;
	}

	ItemList tempItems;
	tempItems.resize(newCount);

	for ( uint32_t index = 0; index < newCount; ++index )
	{
		const int32_t newIndex = map[index];
		if ( newIndex < 0 || newIndex >= newCount )
		{
			return;
		}

		tempItems[newIndex] = m_Items[index];
	}

	m_Items = tempItems;
}

bool TextureCollection::allocateAndAppend(size_t count, ItemType type)
{
	if ( count < 1 || type == ItemType::Undefined )
	{
		return false;
	}

	const size_t oldSize = m_Items.size();
	m_Items.resize(oldSize + count);

	for ( uint32_t index = oldSize; index < m_Items.size(); ++index )
	{
		m_Items[index] = createItem(type);
		hlassert(m_Items[index]);
	}
}

void TextureCollection::truncate(size_t newCount)
{
	if ( newCount >= m_Items.size() )
	{
		return;
	}

	m_Items.resize(newCount);
}

void TextureCollection::clear()
{
	m_Items.clear();
}

int TextureCollection::calculateChecksum() const
{
	// TODO
	hlassert(false);
	return 0;
}

size_t TextureCollection::totalBytesInUse() const
{
	size_t size = 0;

	for ( const ItemPtr& item : m_Items )
	{
		if ( !item.get() )
		{
			continue;
		}

		switch ( item->type() )
		{
			case ItemType::Miptex:
			{
				const MiptexItem* miptexItem = static_cast<const MiptexItem*>(item.get());
				const MiptexWrapper* wrapper = miptexItem->miptex();
				size += wrapper->dataSize();
				break;
			}

			default:
			{
				break;
			}
		}
	}
}

size_t TextureCollection::exportBytesRequired() const
{
	size_t size = 0;

	for ( const ItemPtr& item : m_Items )
	{
		if ( !item.get() )
		{
			continue;
		}

		switch ( item->type() )
		{
			case ItemType::Miptex:
			{
				const MiptexItem* miptexItem = static_cast<const MiptexItem*>(item.get());
				const MiptexWrapper* wrapper = miptexItem->miptex();
				size += wrapper->exportDataSize();
				break;
			}

			default:
			{
				break;
			}
		}
	}

	return size;
}

size_t TextureCollection::exportableTextureCount() const
{
	size_t count = 0;

	for ( const ItemPtr& item : m_Items )
	{
		if ( !item.get() )
		{
			continue;
		}

		switch ( item->type() )
		{
			case ItemType::Miptex:
			{
				const MiptexItem* miptexItem = static_cast<const MiptexItem*>(item.get());
				const MiptexWrapper* wrapper = miptexItem->miptex();

				if ( wrapper->canExport() )
				{
					++count;
				}

				break;
			}

			default:
			{
				break;
			}
		}
	}

	return count;
}

TextureCollection::ItemPtr TextureCollection::createItem(ItemType type)
{
	switch (type)
	{
		case ItemType::Miptex:
		{
			return ItemPtr(new MiptexItem());
		}

		default:
		{
			return ItemPtr();
		}
	}
}
