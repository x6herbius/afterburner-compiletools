#include "pngtexture.h"

PNGTexture::PNGTexture() :
	m_Path(),
	m_Width(0),
	m_Height(0),
	m_Data()
{
}

void PNGTexture::invalidate(bool clearPath)
{
	m_Width = 0;
	m_Height = 0;
	m_Data.clear();

	if ( clearPath )
	{
		m_Path.clear();
	}
}

uint32_t PNGTexture::width() const
{
	return m_Width;
}

uint32_t PNGTexture::height() const
{
	return m_Height;
}

uint32_t PNGTexture::area() const
{
	return m_Width * m_Height;
}

std::string PNGTexture::path() const
{
	return m_Path;
}

bool PNGTexture::setPath(const std::string& path)
{
	if ( path.size() > MAX_PATH_LENGTH )
	{
		return false;
	}

	m_Path = path;
	return true;
}

bool PNGTexture::hasValidPath() const
{
	return m_Path.size() > 0 && m_Path.size() <= MAX_PATH_LENGTH;
}

bool PNGTexture::initialise(uint32_t width, uint32_t height)
{
	invalidate(false);

	m_Width = width;
	m_Height = height;

	m_Data.resize(m_Width * m_Height * 4, 0x00);

	// Set alpha bytes to 0xFF.
	for ( uint32_t index = 3; index < m_Data.size(); index += 4 )
	{
		m_Data[index] = 0xFF;
	}

	return true;
}

uint8_t* PNGTexture::rawData()
{
	if ( !isValid() )
	{
		return NULL;
	}

	return m_Data.data();
}

size_t PNGTexture::rawDataLength() const
{
	return m_Data.size();
}


const uint8_t* PNGTexture::rawData() const
{
	if ( !isValid() )
	{
		return NULL;
	}

	return m_Data.data();
}

uint8_t* PNGTexture::pixelAt(uint32_t x, uint32_t y)
{
	if ( !isValid() )
	{
		return NULL;
	}

	return m_Data.data() + index2DToSequential(x, y);
}

const uint8_t* PNGTexture::pixelAt(uint32_t x, uint32_t y) const
{
	if ( !isValid() )
	{
		return NULL;
	}

	return m_Data.data() + index2DToSequential(x, y);
}

size_t PNGTexture::exportDataSize() const
{
	return hasValidPath() ? MAX_TEXTURE_NAME_LENGTH : 0;
}

bool PNGTexture::exportData(void* buffer, size_t length) const
{
	const size_t dataSize = exportDataSize();

	if ( !buffer || dataSize < 1 || length < dataSize )
	{
		return false;
	}

	memset(buffer, 0, dataSize);
	strncpy(static_cast<char*>(buffer), m_Path.c_str(), m_Path.size());

	return true;
}
