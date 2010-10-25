//  lock-free freelist
//
//  Copyright (C) 2008, 2009 Tim Blechmann
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

//  Disclaimer: Not a Boost library.

#ifndef BOOST_LOCKFREE_FREELIST_HPP_INCLUDED
#define BOOST_LOCKFREE_FREELIST_HPP_INCLUDED

#include <boost/lockfree/detail/tagged_ptr.hpp>
#include <boost/atomic_int.hpp>
#include <boost/noncopyable.hpp>

#include <algorithm>            /* for std::min */

namespace boost
{
namespace lockfree
{

namespace detail
{

template <typename T, typename Alloc = std::allocator<T> >
class dummy_freelist:
    private boost::noncopyable,
    private Alloc
{
    T * allocate (void)
    {
        return Alloc::allocate(1);
    }

    void deallocate (T * n)
    {
        Alloc::deallocate(n, 1);
    }
};

/** dummy freelist, specialization fost std::allocator */
template <typename T>
struct dummy_freelist<T, std::allocator<T> >:
    boost::noncopyable
{
    T * allocate (void)
    {
        return static_cast<T*>(operator new(sizeof(T)));
    }

    void deallocate (T * n)
    {
        operator delete(n);
    }
};

} /* namespace detail */

/** simple freelist implementation  */
template <typename T,
          std::size_t max_size = 64,
          typename Alloc = std::allocator<T> >
class freelist:
    private detail::dummy_freelist<T, Alloc>
{
    struct freelist_node
    {
        lockfree::tagged_ptr<freelist_node> next;
    };

    typedef lockfree::tagged_ptr<freelist_node> tagged_ptr;

public:
    freelist(void):
        pool_(NULL)
    {}

    explicit freelist(std::size_t initial_nodes):
        pool_(NULL)
    {
        for (int i = 0; i != std::min(initial_nodes, max_size); ++i)
        {
            T * node = detail::dummy_freelist<T, Alloc>::allocate();
            deallocate(node);
        }
    }

    ~freelist(void)
    {
        free_memory_pool();
    }

    T * allocate (void)
    {
        for(;;)
        {
            tagged_ptr old_pool(pool_);

            if (not old_pool)
                return detail::dummy_freelist<T, Alloc>::allocate();

            freelist_node * new_pool = old_pool->next.get_ptr();

            if (pool_.CAS(old_pool, new_pool))
            {
                --free_list_size;
                return reinterpret_cast<T*>(old_pool.get_ptr());
            }
        }
    }

    void deallocate (T * n)
    {
        if (free_list_size > max_size)
        {
            detail::dummy_freelist<T, Alloc>::deallocate(n);
            return;
        }

        for(;;)
        {
            tagged_ptr old_pool (pool_);

            freelist_node * new_pool = reinterpret_cast<freelist_node*>(n);

            new_pool->next.set_ptr(old_pool.get_ptr());

            if (pool_.CAS(old_pool, new_pool))
            {
                --free_list_size;
                return;
            }
        }
    }

private:
    void free_memory_pool(void)
    {
        tagged_ptr current (pool_);

        while (current)
        {
            freelist_node * n = current.get_ptr();
            current.set(current->next);
            detail::dummy_freelist<T, Alloc>::deallocate(reinterpret_cast<T*>(n));
        }
    }

    tagged_ptr pool_;
    atomic_int<long> free_list_size;
};

template <typename T, typename Alloc = std::allocator<T> >
class caching_freelist:
    private detail::dummy_freelist<T, Alloc>
{
    struct freelist_node
    {
        lockfree::tagged_ptr<freelist_node> next;
    };

    typedef lockfree::tagged_ptr<freelist_node> tagged_ptr;

public:
    caching_freelist(void):
        pool_(NULL)
    {}

    explicit caching_freelist(std::size_t initial_nodes):
        pool_(NULL)
    {
        for (int i = 0; i != initial_nodes; ++i)
        {
            T * node = detail::dummy_freelist<T, Alloc>::allocate();
            deallocate(node);
        }
    }

    ~caching_freelist(void)
    {
        free_memory_pool();
    }

    T * allocate (void)
    {
        for(;;)
        {
            tagged_ptr old_pool(pool_);

            if (!old_pool)
                return detail::dummy_freelist<T, Alloc>::allocate();

            freelist_node * new_pool = old_pool->next.get_ptr();

            if (pool_.CAS(old_pool, new_pool))
                return reinterpret_cast<T*>(old_pool.get_ptr());
        }
    }

    void deallocate (T * n)
    {
        for(;;)
        {
            tagged_ptr old_pool (pool_);

            freelist_node * new_pool = reinterpret_cast<freelist_node*>(n);

            new_pool->next.set_ptr(old_pool.get_ptr());

            if (pool_.CAS(old_pool,new_pool))
                return;
        }
    }

private:
    void free_memory_pool(void)
    {
        tagged_ptr current (pool_);

        while (current)
        {
            freelist_node * n = current.get_ptr();
            current.set(current->next);
            detail::dummy_freelist<T, Alloc>::deallocate(reinterpret_cast<T*>(n));
        }
    }

    tagged_ptr pool_;
};

} /* namespace lockfree */
} /* namespace boost */

#endif /* BOOST_LOCKFREE_FREELIST_HPP_INCLUDED */
