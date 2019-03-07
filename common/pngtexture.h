#ifndef PNGTEXTURE_H
#define PNGTEXTURE_H

#include <cstdint>
#include <vector>
#include <string>
#include "bspfile.h"

class PNGTexture
{
public:
	// -1 to include room for terminator when writing to textures lump.
	static constexpr size_t MAX_PATH_LENGTH = MAX_TEXTURE_NAME_LENGTH - 1;

	PNGTexture();

	inline bool isValid() const
	{
		return m_Width > 0 && m_Height > 0;
	}

	void invalidate(bool clearPath = true);

	uint32_t width() const;
	uint32_t height() const;

	// This is the number of pixels, not bytes.
	uint32_t area() const;

	std::string path() const;
	bool setPath(const std::string& path);
	bool hasValidPath() const;

	// Image is initialised to fully opaque black.
	bool initialise(uint32_t width, uint32_t height);

	// There are width * height * 4 bytes in row-major RGBA order.
	// If the image is not valid, NULL is returned.
	uint8_t* rawData();
	const uint8_t* rawData() const;
	size_t rawDataLength() const;

	// Pixel is RGBA.
	// If the image is not valid, NULL is returned.
	uint8_t* pixelAt(uint32_t x, uint32_t y);
	const uint8_t* pixelAt(uint32_t x, uint32_t y) const;

	// Exporting this texture just exports the path, as it is loaded from disk when required.
	size_t exportDataSize() const;
	bool exportData(void* buffer, size_t length) const;

private:
	inline uint32_t index2DToSequential(uint32_t x, uint32_t y) const
	{
		return (m_Width * y) + x;
	}

	std::string m_Path;
	uint32_t m_Width;
	uint32_t m_Height;
	std::vector<uint8_t> m_Data;
};

#endif // PNGTEXTURE_H
