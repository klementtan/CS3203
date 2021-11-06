// arena.h

#pragma once
#include <memory>
#include <vector>
#include <unordered_map>

namespace util
{
    struct Arena
    {
        struct Chunk
        {
            uint8_t* memory;
            size_t used;
            size_t capacity;

            Chunk* next;
        };

        void* allocate(size_t n, size_t align);
        void clear();

        static Arena& global();

    private:
        Chunk* head = nullptr;
    };

    template <typename T>
    struct arena_allocator
    {
        using value_type = T;

        arena_allocator() noexcept { } // not required, unless used

        template <typename U>
        arena_allocator(const arena_allocator<U>&) noexcept
        {
        }

        inline value_type* allocate(std::size_t n)
        {
            return (value_type*) Arena::global().allocate(n * sizeof(value_type), alignof(value_type));
            // return static_cast<value_type*>(::operator new(n * sizeof(value_type)));
        }

        inline void deallocate(value_type* p, size_t n) noexcept
        {
            // ::operator delete(p);
        }
    };

    template <typename T, typename U>
    bool operator==(const arena_allocator<T>&, const arena_allocator<U>&) noexcept
    {
        return true;
    }

    template <typename T, typename U>
    bool operator!=(const arena_allocator<T>& x, const arena_allocator<U>& y) noexcept
    {
        return !(x == y);
    }


    template <typename T>
    using ArenaVec = std::vector<T, arena_allocator<T>>;

    template <typename K, typename V>
    using ArenaMap = std::unordered_map<K, V, std::hash<K>, std::equal_to<K>, arena_allocator<std::pair<const K, V>>>;
}