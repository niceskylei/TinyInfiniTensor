#include "core/allocator.h"
#include <utility>

namespace infini
{
    Allocator::Allocator(Runtime runtime) : runtime(runtime)
    {
        used = 0;
        peak = 0;
        ptr = nullptr;

        // 'alignment' defaults to sizeof(uint64_t), because it is the length of
        // the longest data type currently supported by the DataType field of
        // the tensor
        alignment = sizeof(uint64_t);
        // Add a free block for the first 1MB
        freeBlocks[0] = reinterpret_cast<size_t>(malloc(1024 * 1024));
    }

    Allocator::~Allocator()
    {
        if (this->ptr != nullptr)
        {
            runtime->dealloc(this->ptr);
        }
    }

    size_t Allocator::alloc(size_t size)
    {
        IT_ASSERT(this->ptr == nullptr);
        // pad the size to the multiple of alignment
        size = this->getAlignedSize(size);

        // =================================== 作业 ===================================
        // TODO: 设计一个算法来分配内存，返回起始地址偏移量
        // =================================== 作业 ===================================

        for (auto it = freeBlocks.begin(); it != freeBlocks.end(); ++it)
        {
            if (it->second >= size)
            {
                // We found a free block large enough
                size_t blockStart = it->first;
                size_t blockSize = it->second;

                // Remove this block from free list
                freeBlocks.erase(it);

                // If the block is larger than requested, split it and return the address
                if (blockSize > size)
                {
                    freeBlocks[blockStart + size] = blockSize - size;
                }

                // Update used memory
                used += size;
                peak = std::max(peak, used);

                // Return the start address of the allocated block
                return blockStart;
            }
        }
        // If no suitable block was found, use system's default allocator (malloc)
        void *newBlock = malloc(size);
        if (!newBlock)
        {
            throw std::bad_alloc(); // Or some custom error handling
        }

        // If malloc succeeds, we simulate adding the new block to the memory pool
        // We return the address as a size_t offset
        size_t blockStart = reinterpret_cast<size_t>(newBlock);

        // Update used memory
        used += size;
        peak = std::max(peak, used);

        // Return the start address of the newly allocated block
        return blockStart;
    }
    // Merges adjacent free blocks in the freeBlocks map
    void Allocator::mergeFreeBlocks()
    {
        if (freeBlocks.empty())
            return;

        auto it = freeBlocks.begin();
        while (it != freeBlocks.end())
        {
            auto nextIt = std::next(it);

            if (nextIt != freeBlocks.end() && it->first + it->second == nextIt->first)
            {
                // Merge the two adjacent blocks
                it->second += nextIt->second;
                freeBlocks.erase(nextIt); // Remove the merged block
            }
            else
            {
                ++it;
            }
        }
    }
    void Allocator::free(size_t addr, size_t size)
    {
        IT_ASSERT(this->ptr == nullptr);
        size = getAlignedSize(size);

        // =================================== 作业 ===================================
        // TODO: 设计一个算法来回收内存
        // =================================== 作业 ===================================
        // Insert the freed block back into the free blocks map
        freeBlocks[addr] = size;

        // Try to merge adjacent free blocks
        mergeFreeBlocks();

        // Update memory usage
        used -= size;
    }

    void *Allocator::getPtr()
    {
        if (this->ptr == nullptr)
        {
            this->ptr = runtime->alloc(this->peak);
            printf("Allocator really alloc: %p %lu bytes\n", this->ptr, peak);
        }
        return this->ptr;
    }

    size_t Allocator::getAlignedSize(size_t size)
    {
        return ((size - 1) / this->alignment + 1) * this->alignment;
    }

    void Allocator::info()
    {
        std::cout << "Used memory: " << this->used
                  << ", peak memory: " << this->peak << std::endl;
    }
}
