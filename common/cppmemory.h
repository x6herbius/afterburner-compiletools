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

namespace Mem
{
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

	template<typename T>
	using vector = std::vector<T, BlockAllocator<T>>;

	template<typename T>
	using DeletedUniquePtr = std::unique_ptr<T, std::function<void(T*)>>;

	template<typename T>
	class HLAllocUniquePtr : public DeletedUniquePtr<T>
	{
	public:
		HLAllocUniquePtr(T* obj = NULL) : DeletedUniquePtr<T>(obj, &Free) {}
	};
}

#endif // BLOCKALLOCATOR_H
