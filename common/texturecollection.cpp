#include "texturecollection.h"
#include "miptexwrapper.h"

class TextureCollection::Entry
{
public:
	Entry(EntryType type) :
		m_Type(type)
	{
	}

	virtual ~Entry()
	{
	}

	EntryType type() const
	{
		return m_Type;
	}

private:
	EntryType m_Type;
};

class MiptexEntry : public TextureCollection::Entry
{
public:
	MiptexEntry() : TextureCollection::Entry(TextureCollection::EntryType::Miptex)
	{
	}

	inline MiptexWrapper& miptex()
	{
		return m_Miptex;
	}

	inline const MiptexWrapper& miptex() const
	{
		return m_Miptex;
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
