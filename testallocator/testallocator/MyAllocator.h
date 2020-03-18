#pragma once

#include <stdio.h>
#include <tchar.h>

#include <iostream>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <vector>
#include <memory>
#include <map>
#include <list>
#include <cassert>
#include <utility>

#define POOL_THRESHOLD (131072LL)
#define RESORT_THRESHOLD (131072LL)

#define MIN_POOL_SIZE (4)
#define POOL_BLOCKS (512)

const double grow_base = pow(~0ull, 1.0 / POOL_BLOCKS);

namespace MY_ALLOCATOR
{
	class Pool
	{
	private:
		void *BasePool_Head[64], *List[POOL_BLOCKS];
		size_t Last, t, last_resort;
		size_t blocks_remain;
	public:
		Pool()
		{
			memset(BasePool_Head, 0, sizeof(BasePool_Head));
			memset(List, 0, sizeof(List));
			Last = 0;
			t = MIN_POOL_SIZE;
			BasePool_Head[t] = malloc(1LL<<t);
		}
		virtual ~Pool()
		{
			for (size_t i = 0; i <= t; ++i) free(BasePool_Head[i]);
		}
		void Sort_Out()
		{
			std::cout << "Resorted!" << std::endl;
			int cnt_recycled_blocks=0;
			using PVpI = std::pair<void*, size_t>;
			PVpI *resort_vec = new PVpI[blocks_remain];
			for (int i = 0; i < POOL_BLOCKS; ++i)
			{
				void* ptr = List[i];
				while (ptr)
				{
					resort_vec[cnt_recycled_blocks++] =
						std::make_pair(ptr, *(size_t*)((void**)ptr + 1));
					ptr = *(void**)ptr;
				}
			}
			std::sort(resort_vec, resort_vec+cnt_recycled_blocks);
			memset(List, 0, sizeof(List)); blocks_remain = 0;
			for (int i = 0; i < cnt_recycled_blocks; ++i)
			{
				resort_vec[i].second = ((resort_vec[i].second - 1) / (sizeof(size_t) << 1) + 1)
					*(sizeof(size_t) << 1);
				if ((size_t*)resort_vec[i].first + resort_vec[i].second / sizeof(size_t) ==
					(size_t*)resort_vec[i + 1].first)
				{
					resort_vec[i + 1].first = resort_vec[i].first;
					resort_vec[i + 1].second += resort_vec[i].second;
				}
				else RECYCLE(resort_vec[i].first, resort_vec[i].second, false);
			}
			delete[] resort_vec;
			last_resort = blocks_remain; return;
		}
		void* ALLOC(size_t n)
		{
			n = ((n - 1) / (sizeof(size_t) << 1) + 1) *(sizeof(size_t) << 1);
			if (n > POOL_THRESHOLD) return malloc(n);
			if(blocks_remain-last_resort>RESORT_THRESHOLD) Sort_Out();
			void* tempp = Try_list(n);
			if (tempp) return tempp;
			while (((1LL << t) - Last) < n)
			{
				RECYCLE((size_t*)BasePool_Head[t] + Last / sizeof(size_t), (1LL << t) - Last, false);
				BasePool_Head[++t] = malloc(1LL << t); Last = 0;
			}
			Last += n;
			return (size_t*)BasePool_Head[t] + ((Last-n) / sizeof(size_t));
		};

		void* Try_list(size_t n)
		{
			double temp;
			size_t k;
			for (temp = 1, k = 0; temp < n; temp *= grow_base)k++;
			for ( ; k <= t;++k) if(List[k])
			{
				size_t tempn = *(size_t*)((void**)List[k] + 1);
				if (tempn < n) continue;
				void* ptr = List[k];
				List[k] = *(void**)List[k];
				RECYCLE((size_t*)ptr + n / sizeof(size_t), tempn - n, false);
				blocks_remain--; return ptr;
			}
			return NULL;
		}

		void RECYCLE(void* ptr, size_t n, bool flag)
		{
			if (n == 0) return;
			if (n > POOL_THRESHOLD && flag) { free(ptr); return; }
			double temp;
			size_t k;
			for (temp = 1, k = 0; temp < n; temp *= grow_base)k++;
			*(void**)ptr = List[k];
			*(size_t*)((void**)ptr + 1) = n;
			List[k] = ptr;
			blocks_remain++; return;
		}
	}GlobalPool;

	template<class T> class MyAllocator;

	template <> class MyAllocator<void>
	{
	public:
		typedef void* pointer;
		typedef const void* const_pointer;
		typedef void value_type;
		template <class U> struct rebind { typedef MyAllocator<U> other; };
	};

	template<class T>
	class MyAllocator
	{
	public:
		typedef T                   value_type;
		typedef value_type*         pointer;
		typedef value_type&         reference;
		typedef value_type const*   const_pointer;
		typedef value_type const&   const_reference;
		typedef size_t              size_type;
		typedef ptrdiff_t           difference_type;

		template <typename U> struct rebind { typedef MyAllocator<U> other; };

		MyAllocator() noexcept {};
		MyAllocator(const MyAllocator& alloc) noexcept {};

		template <class U>
		MyAllocator(const MyAllocator<U>& alloc) noexcept {};

		virtual ~MyAllocator() throw() {};

		pointer address(reference x) const noexcept { return &x; };

		pointer allocate(size_type n, MyAllocator<void>::const_pointer Hint = 0)
		{
			return (pointer)GlobalPool.ALLOC(n * sizeof(value_type));
		}

		void deallocate(pointer p, size_type n)
		{
			GlobalPool.RECYCLE(p, n, true);
		}

		size_type max_size() const noexcept
		{
			return (~(size_type)(0)) >> 1;
		}

		template <class U, class... Args>
		void construct(U* p, Args&&... args)
		{
			::new((void*)p) U(std::forward<Args>(args)...);
		}

		template <class U>
		void destroy(U* p)
		{
			p->~U();
		}
	};
}
