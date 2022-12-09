#include <new>
#include <limits>
#include <memory>
#include <cstddef>
#include <cstdint>

template<typename T>
class heap_allocator
{
public:
    using pointer = T *;
    using size_type = std::size_t;
    using reference = T &;
    using value_type = T;
    using const_pointer = const T *;
    using const_reference = const T &;
    using difference_type = std::ptrdiff_t;

public:
    template<typename U>
    struct rebind{ using other = heap_allocator<U>; };

public:
    template<typename U>
    heap_allocator(const heap_allocator<U>& other) noexcept {}

public:
    pointer allocate(size_type count)
    {
        return static_cast<pointer>(::operator new(count * sizeof(value_type)));
    }

    void deallocate(pointer ptr, size_type count) noexcept
    {
        ::operator delete(ptr);
    }

    size_type max_size() const noexcept
    {
        return std::numeric_limits<size_type>::max() / sizeof(value_type);
    }
};

template <typename T, std::size_t Capacity>
class stack_allocator: public std::allocator<T>
{
public:
    using pointer = T *;
    using size_type = std::size_t;
    using reference = T &;
    using value_type = T;
    using const_pointer = const T *;
    using const_reference = const T &;
    using difference_type = std::ptrdiff_t;

public:
    template <typename U>
    struct rebind { using other = stack_allocator<U, Capacity>; };

public:
    stack_allocator(): buffer_ {}, head_{reinterpret_cast<block_header*> (buffer_)}
    {
        head_->data = reinterpret_cast<std::uint8_t*> (head_ + 1);
        head_->size = Capacity - sizeof(block_header);
        head_->next = nullptr;
    }

    template <typename U>
    stack_allocator(const stack_allocator<U, Capacity> &other) noexcept: stack_allocator() {}

public:
    pointer allocate(size_type count)
    {
        std::size_t size = count* sizeof(value_type);

        // align size to 8 bytes
        size = (size + 7) &~7;

        block_header *block = head_;
        block_header *prev = nullptr;

        // search for the first block with enough size
        while (block != nullptr && block->size < size)
        {
            prev = block;
            block = block->next;
        }

        if (block == nullptr)
        {
            // no block with enough size was found
            return nullptr;
        }

        // split the block if there is enough remaining size
        if (block->size - size >= sizeof(block_header) + 8)
        {
            block_header *next = reinterpret_cast<block_header*> (block->data + size);
            next->data = block->data + size + sizeof(block_header);
            next->size = block->size - size - sizeof(block_header);
            next->next = block->next;

            block->size = size;
            block->next = next;
        }

        return static_cast<pointer> (block->data);
    }

    void deallocate(pointer ptr, size_type count) noexcept
    {
        // no-op, since deallocating individual blocks is not supported
    }

    size_type max_size() const noexcept
    {
        return Capacity / sizeof(value_type);
    }

private:
    struct block_header
    {
        std::uint8_t * data;
        std::size_t size;
        block_header * next;
    };

    block_header * head_;
    alignas(8) std::uint8_t buffer_[Capacity];
};

template <typename T, typename PrimaryAllocator, typename FallbackAllocator>
class fallback_allocator: public std::allocator<T>
{
public:
    using pointer = T *;
    using size_type = std::size_t;
    using reference = T &;
    using value_type = T;
    using const_pointer = const T *;
    using const_reference = const T &;
    using difference_type = std::ptrdiff_t;

public:
    template <typename U>
    struct rebind { using other = fallback_allocator<U, PrimaryAllocator, FallbackAllocator>; };

public:
    fallback_allocator()
    {
    }

    template <typename U>
    fallback_allocator(const fallback_allocator<U, PrimaryAllocator, FallbackAllocator> &other) noexcept: fallback_allocator() {}

public:
    pointer allocate(size_type count)
    {
        return nullptr;
    }

    void deallocate(pointer ptr, size_type count) noexcept
    {
        // not yet implemented
    }

    size_type max_size() const noexcept
    {
        // not yet implemented
        return 0;
    }

private:
    PrimaryAllocator primary_allocator;
    FallbackAllocator fallback_allocator;
};
