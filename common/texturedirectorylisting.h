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
	typedef std::map<std::string, int32_t> TextureIndexMap;
	static constexpr int32_t INVALID_TEXTURE_INDEX = -1;

	TextureDirectoryListing();

	std::string textureDirPath();
	void setTextureDirPath(const std::string& path);

	bool makeListing();
	uint32_t texturePathsSearched() const;

	bool containsTexture(const std::string& textureRelPath) const;
	bool textureIsReferenced(const std::string& textureRelPath) const;

	// Returns INVALID_TEXTURE_INDEX if the texture doesn't exist or is not referenced.
	int32_t textureIndex(const std::string& textureRelPath) const;

	// Returns the assigned index, or INVALID_TEXTURE_INDEX if the texture did not exist.
	int32_t assignTextureIndex(const std::string& textureRelPath);

	const char* texturePath(const uint32_t index) const;

	void textureList(std::vector<std::string>& list) const;
	size_t count() const;
	void clear();

	TextureIndexMap::const_iterator mapBegin() const;
	TextureIndexMap::const_iterator mapEnd() const;

	std::string makeFullTexturePath(const std::string textureRelPath) const;

private:
	typedef struct dirent dirent_t;

	static std::string makeSystemCanonicalTexturePath(const std::string& origPath);
	static bool fileNameIsPNG(const char* path);
	static std::string fileNameWithoutExtension(const char* origName);

	bool readTexturesFromDirectory(const std::string& path);

	std::string m_TextureDirPath;
	TextureIndexMap m_TextureToIndex;
	int32_t m_NextTextureIndex;
	uint32_t m_NumTexturePathsSearched;
	std::vector<const std::string*> m_IndexToTexturePath;
};

#endif // TEXTUREDIRECTORYLISTING_H
