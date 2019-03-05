#ifndef TEXTUREDIRECTORYLISTING_H
#define TEXTUREDIRECTORYLISTING_H

#include <string>
#include <map>
#include <cstdint>
#include <vector>
#include "dirent.h"

class TextureDirectoryListing
{
public:
	TextureDirectoryListing();

	std::string textureDirPath();
	void setTextureDirPath(const std::string& path);

	bool makeListing();
	bool containsTexture(const std::string& textureRelPath) const;
	bool textureIsReferenced(const std::string& textureRelPath) const;
	uint32_t textureRefCount(const std::string& textureRelPath) const;

	bool incrementRefCount(const std::string& textureRelPath);
	bool decrementRefCount(const std::string& textureRelPath);

	void textureList(std::vector<std::string>& list) const;
	size_t textureCount() const;
	void clearTextures();

private:
	typedef struct dirent dirent_t;

	static bool fileNameIsPNG(const char* path);

	bool readTexturesFromDirectory(const std::string& path);

	std::string m_TextureDirPath;
	std::map<std::string, uint32_t> m_TextureRefCount;
};

#endif // TEXTUREDIRECTORYLISTING_H
