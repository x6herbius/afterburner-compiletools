#include "texturecollection.h"

const uint32_t MiptexWrapper::PALETTE_SIZE = 256;

MiptexWrapper::MiptexWrapper(const char* name, uint32_t width, uint32_t height) :
	m_Name(),
	m_Width(0),
	m_Height(0),
	m_Mipmaps(),
	m_Palette()
{
	m_Name[0] = '\0';

	if ( name )
	{
		setName(name);
	}

	if ( width > 0 && height > 0 )
	{
		setDimensions(width, height);
	}
}

void MiptexWrapper::invalidate(bool clearName)
{
	m_Width = 0;
	m_Height = 0;

	if ( clearName )
	{
		m_Name[0] = '\0';
	}

	for ( uint32_t level = 0; level < MIPLEVELS; ++level )
	{
		m_Mipmaps[level].clear();
	}

	m_Palette.clear();
}

void MiptexWrapper::setBlank()
{
	if ( !isValid() )
	{
		return;
	}

	initialiseMipmap();
	initialisePalette();
}

bool MiptexWrapper::initialise(uint32_t width, uint32_t height)
{
	if ( !setDimensions(width, height) )
	{
		return false;
	}

	setBlank();
	return true;
}

bool MiptexWrapper::setDimensions(uint32_t width, uint32_t height)
{
	// Reset everything except the name.
	invalidate(false);

	if ( width < 1 || height < 1 || width % 16 != 0 || height % 16 != 0 )
	{
		return false;
	}

	m_Width = width;
	m_Height = height;

	return true;
}

uint32_t MiptexWrapper::width() const
{
	return m_Width;
}

uint32_t MiptexWrapper::height() const
{
	return m_Height;
}

const char* MiptexWrapper::name() const
{
	return m_Name;
}

bool MiptexWrapper::hasPalette() const
{
	return !m_Palette.empty();
}

void MiptexWrapper::initialisePalette()
{
	if ( !isValid() )
	{
		return;
	}

	m_Palette.clear();
	m_Palette.resize(PALETTE_SIZE * sizeof(rgbpixel_t), 0);
}

bool MiptexWrapper::hasMipmap(uint32_t level) const
{
	return level < MIPLEVELS && !m_Mipmaps[level].empty();
}

void MiptexWrapper::initialiseMipmap(int32_t level)
{
	if ( !isValid() || level >= MIPLEVELS )
	{
		return;
	}

	if ( level >= 0 )
	{
		m_Mipmaps[level].clear();
		m_Mipmaps[level].resize(areaForMipLevel(level), 0);
		return;
	}

	// Level is negative, so init all.
	for ( level = 0; level < MIPLEVELS; ++level )
	{
		initialiseMipmap(level);
	}
}

void MiptexWrapper::setName(const char* name)
{
	if ( !name )
	{
		m_Name[0] = '\0';
		return;
	}

	safe_strncpy(m_Name, name, sizeof(m_Name));
}

int32_t MiptexWrapper::paletteIndexAt(uint32_t x, uint32_t y, uint32_t mipLevel) const
{
	if ( !hasMipmap(mipLevel) || mipLevel >= MIPLEVELS || x >= widthForMipLevel(mipLevel) || y >= heightForMipLevel(mipLevel) )
	{
		return -1;
	}

	const std::vector<byte>& mipmap = m_Mipmaps[mipLevel];
	return static_cast<int32_t>(mipmap[(y * m_Width) + x]);
}

const MiptexWrapper::rgbpixel_t* MiptexWrapper::paletteColour(uint8_t paletteIndex) const
{
	if ( !hasPalette() )
	{
		return NULL;
	}

	return reinterpret_cast<const rgbpixel_t*>(&m_Palette[paletteIndex * sizeof(rgbpixel_t)]);
}

const MiptexWrapper::rgbpixel_t* MiptexWrapper::colourAt(uint32_t x, uint32_t y, uint32_t mipLevel) const
{
	int32_t paletteIndex = paletteIndexAt(x, y, mipLevel);
	if ( paletteIndex < 0 )
	{
		return NULL;
	}

	return paletteColour(paletteIndex);
}

uint32_t MiptexWrapper::widthForMipLevel(uint32_t level) const
{
	return dimensionForMipLevel(m_Width, level);
}

uint32_t MiptexWrapper::heightForMipLevel(uint32_t level) const
{
	return dimensionForMipLevel(m_Height, level);
}

uint32_t MiptexWrapper::areaForMipLevel(uint32_t level) const
{
	return areaForMipLevel(m_Width, m_Height, level);
}

uint32_t MiptexWrapper::dimensionForMipLevel(uint32_t dim, uint32_t level)
{
	if ( level >= MIPLEVELS )
	{
		level = MIPLEVELS - 1;
	}

	return dim >> level;
}

uint32_t MiptexWrapper::areaForMipLevel(uint32_t width, uint32_t height, uint32_t level)
{
	return dimensionForMipLevel(width, level) * dimensionForMipLevel(height, level);
}
