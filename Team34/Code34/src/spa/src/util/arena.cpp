// arena.cpp

#include <cstdlib>
#include <algorithm>

#include "arena.h"
#include "exceptions.h"

namespace util
{
    // allocate in 256kb blocks.
    static constexpr size_t CHUNK_CAPCITY = 256 * 1024;

    Arena& Arena::global()
    {
        static Arena arena {};
        return arena;
    }

    static Arena::Chunk* make_new_chunk(size_t minimum)
    {
        auto chunk = new Arena::Chunk {};

        chunk->used = 0;
        chunk->next = nullptr;
        chunk->capacity = std::max(minimum, CHUNK_CAPCITY);
        chunk->memory = (uint8_t*) malloc(chunk->capacity);

        return chunk;
    }

    void* Arena::allocate(size_t req, size_t align)
    {
        spa_assert((align & (align - 1)) == 0);

        if(this->head == nullptr)
        {
            // zpr::fprintln(stderr, "no head");
            auto chunk = make_new_chunk(req);
            this->head = chunk;
        }

        Chunk* cur = this->head;
        while(true)
        {
            // clang-format off
            uint8_t* ptr = (uint8_t*) (((uintptr_t) (cur->memory + cur->used + (align - 1))) & ~(align - 1));
            size_t real_size = (size_t) ((ptr + req) - (cur->memory + cur->used));
            // clang-format on

            if(req == 0)
                return cur->memory + cur->used;

            if(cur->used + real_size <= cur->capacity)
            {
                cur->used += real_size;
                return ptr;
            }

            if(cur->next)
            {
                cur = cur->next;
                continue;
            }

            size_t worst_case = req + (align - 1);
            auto chunk = make_new_chunk(worst_case);

            chunk->next = this->head;
            this->head = chunk;
            cur = this->head;
        }
    }

    void Arena::clear()
    {
        // zpr::fprintln(stderr, "clearing head = {}", (void*) this->head);

        auto cur = this->head;
        while(cur)
        {
            auto old = cur;
            cur = cur->next;

            free(old->memory);
            delete old;
        }

        this->head = nullptr;
    }
}
