#include "texturedirectorylisting.h"
#include <vector>
#include "log.h"
#include "filelib.h"
#include "cmdlib.h"

#define WARNING(...) Warning("TextureDirectoryListing: " __VA_ARGS__)

TextureDirectoryListing::TextureDirectoryListing() :
	m_TextureDirPath(),
	m_TextureRefCount()
{
}

std::string TextureDirectoryListing::textureDirPath()
{
	return m_TextureDirPath;
}

void TextureDirectoryListing::setTextureDirPath(const std::string& path)
{
	m_TextureDirPath = path;
}

void TextureDirectoryListing::textureList(std::vector<std::string>& list) const
{
	list.clear();

	for ( auto iterator = m_TextureRefCount.begin(); iterator != m_TextureRefCount.end(); ++iterator )
	{
		list.push_back(iterator->first);
	}
}

void TextureDirectoryListing::clearTextures()
{
	m_TextureRefCount.clear();
}

size_t TextureDirectoryListing::textureCount() const
{
	return m_TextureRefCount.size();
}

bool TextureDirectoryListing::containsTexture(const std::string& textureRelPath) const
{
	return m_TextureRefCount.find(textureRelPath) != m_TextureRefCount.end();
}

bool TextureDirectoryListing::textureIsReferenced(const std::string& textureRelPath) const
{
	std::map<std::string, uint32_t>::const_iterator iterator = m_TextureRefCount.find(textureRelPath);
	return iterator != m_TextureRefCount.end() && iterator->second > 0;
}

uint32_t TextureDirectoryListing::textureRefCount(const std::string& textureRelPath) const
{
	std::map<std::string, uint32_t>::const_iterator iterator = m_TextureRefCount.find(textureRelPath);
	return iterator != m_TextureRefCount.end() ? iterator->second : 0;
}

bool TextureDirectoryListing::incrementRefCount(const std::string& textureRelPath)
{
	std::map<std::string, uint32_t>::iterator iterator = m_TextureRefCount.find(textureRelPath);
	if ( iterator == m_TextureRefCount.end() )
	{
		return false;
	}

	// Good luck overflowing this.
	if ( iterator->second + 1 > iterator->second )
	{
		++iterator->second;
	}

	return true;
}

bool TextureDirectoryListing::decrementRefCount(const std::string& textureRelPath)
{
	std::map<std::string, uint32_t>::iterator iterator = m_TextureRefCount.find(textureRelPath);
	if ( iterator == m_TextureRefCount.end() )
	{
		return false;
	}

	if ( iterator->second > 0 )
	{
		--iterator->second;
	}

	return true;
}

bool TextureDirectoryListing::makeListing()
{
	m_TextureRefCount.clear();

	if ( m_TextureDirPath.empty() )
	{
		WARNING("No texture directory path was set.");
		return false;
	}

	DIR* directory = opendir(m_TextureDirPath.c_str());
	if ( !directory )
	{
		WARNING("Could not open texture directory: %s", m_TextureDirPath.c_str());
		return false;
	}

	std::vector<std::string> dirPaths;

	// Add the root path, because Nightfire has a couple of things in there.
	dirPaths.push_back("");

	for ( dirent_t* entry = readdir(directory); entry; entry = readdir(directory) )
	{
		// Note that the current and parent directories show up in this listing, so they should be ignored.
		if ( entry->d_type == DT_DIR && strncmp(entry->d_name, ".", 1) != 0 && strncmp(entry->d_name, "..", 2) != 0 )
		{
			dirPaths.push_back(entry->d_name);
		}
	}

	closedir(directory);

	for ( const std::string& path : dirPaths )
	{
		if ( !readTexturesFromDirectory(path) )
		{
			return false;
		}
	}

	return true;
}

bool TextureDirectoryListing::readTexturesFromDirectory(const std::string& path)
{
	std::string fullPath = m_TextureDirPath;

	if ( path.size() > 0 )
	{
		fullPath += std::string(SYSTEM_SLASH_STR) + path;
	}

	dirent_t** entryList = NULL;
	int texturesFound = scandir(
		fullPath.c_str(),
		&entryList,
		[](const dirent_t* entry) -> int
		{
			return TextureDirectoryListing::fileNameIsPNG(entry->d_name) ? 1 : 0;
		},
		NULL);

	if ( texturesFound < 0 || !entryList )
	{
		WARNING("Could not scan texture subdirectory %s for texture files.", fullPath.c_str());
		return false;
	}

	for ( uint32_t index = 0; index < texturesFound; ++index )
	{
		std::string textureRelPath;

		if ( path.size() > 0 )
		{
			textureRelPath += path + std::string(SYSTEM_SLASH_STR);
		}

		textureRelPath += std::string(entryList[index]->d_name);
		m_TextureRefCount[textureRelPath] = 0;
	}

	free(entryList);
	return true;
}

bool TextureDirectoryListing::fileNameIsPNG(const char* path)
{
	std::string extension(FS_FileExtension(path));

	std::for_each(extension.begin(), extension.end(), [](char & c)
	{
		c = ::tolower(c);
	});

	return extension == std::string("png");
}
