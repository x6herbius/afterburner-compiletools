#ifndef BLOCKALLOCATOR_H
#define BLOCKALLOCATOR_H

// Thanks to https://stackoverflow.com/a/11417774/2054335

#include <new>
#include <limits>
#include "blockmem.h"
#include "messages.h"
#include "log.h"

template <class T>
struct BlockAllocator
{
	typedef size_t size_type;
	typedef ptrdiff_t difference_type;
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;

	template <class U> struct rebind { typedef BlockAllocator<U> other; };
	BlockAllocator() throw() {}
	BlockAllocator(const BlockAllocator&) throw() {}

	template <class U> BlockAllocator(const BlockAllocator<U>&) throw(){}

	~BlockAllocator() throw() {}

	pointer address(reference x) const { return &x; }
	const_pointer address(const_reference x) const { return &x; }

	pointer allocate(size_type s, void const * = 0)
	{
		if (0 == s)
		{
			return NULL;
		}

		pointer temp = (pointer)AllocBlock(s * sizeof(T));
		hlassume (temp != NULL, assume_NoMemory);

		return temp;
	}

	void deallocate(pointer p, size_type)
	{
		FreeBlock(p);
	}

	size_type max_size() const throw()
	{
		return std::numeric_limits<size_t>::max() / sizeof(T);
	}

	void construct(pointer p, const T& val)
	{
		new((void*)p) T(val);
	}

	void destroy(pointer p)
	{
		p->~T();
	}
};

#endif // BLOCKALLOCATOR_H
