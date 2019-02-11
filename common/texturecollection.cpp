#include "texturecollection.h"
#include "miptexwrapper.h"

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
