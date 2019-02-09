#include "texturecollection.h"

struct TextureCollection::Entry
{
	enum class Type
	{
		Undefined = 0,
		Miptex,
		PngOnDisk
	};

	Entry() :
		type(Type::Undefined)
	{
	}

	Type type;
};

TextureCollection::TextureCollection() :
	m_Items()
{
}

TextureCollection::~TextureCollection()
{
}
