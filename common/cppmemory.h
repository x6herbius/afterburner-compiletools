#ifndef BLOCKALLOCATOR_H
#define BLOCKALLOCATOR_H

// Thanks to https://stackoverflow.com/a/11417774/2054335

#include <new>
#include <limits>
#include <vector>
#include <memory.h>
#include <functional>
#include "blockmem.h"
#include "messages.h"
#include "log.h"

void* operator new(size_t size)
{
	void* mem = Alloc(size);
	hlassume(mem != NULL, assume_NoMemory);

	if ( !mem )
	{
		throw std::bad_alloc();
	}

	return mem;
}

void operator delete(void* ptr) noexcept
{
	if ( ptr )
	{
		Free(ptr);
	}
}

#endif // BLOCKALLOCATOR_H
