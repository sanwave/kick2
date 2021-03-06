
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <iostream>

#include "log.h"
#include "socket.h"
#include "mem_pool.h"

MemPool::MemPool()
    :m_maxMemory(0) {}

int MemPool::Initialize(uint64_t maxMemory)
{
    m_maxMemory = maxMemory;
    uint32_t sizes[] = { 1024,10240,102400,1024000,2048000,5120000, };
    for (uint32_t i = 0; i < sizeof(sizes)/sizeof(uint32_t); ++i)
        m_pool2.insert(std::pair<uint32_t, std::list<Block *> >(sizes[i], std::list<Block *>()));
    return 0;
}

Block *MemPool::Allocate(uint32_t size)
{
    uint32_t leftSize = size;
    std::list<Block *> buffer;
    while (leftSize > 0)
    {
        std::map<uint32_t, std::list<Block *> >::iterator item = m_pool2.lower_bound(leftSize);
        if (item == m_pool2.end())
            --item;
        Block *block;
        if (item->second.empty())
        {
            if (m_maxMemory > 0 && zmalloc_used_memory() > m_maxMemory)
            {
                return NULL;
            }
            block = new Block(item->first);
            DEBUG_LOG("MemPool::Allocate(), allocate new block, buf: "<<(void *)block->Data<<", size: "<<item->first);
        }
        else
        {
            block = item->second.front();
            item->second.pop_front();
        }
        uint32_t capacity = block->Capacity;
        if (block->Capacity > leftSize)
            capacity = leftSize;
        leftSize -= capacity;
        if (!buffer.empty())
            (*buffer.rbegin())->Next = block;
        buffer.push_back(block);
    }
    return *buffer.begin();
}


void MemPool::Free(Block *buffer)
{
    Block *block = buffer;
    while (block)
    {
        std::map<uint32_t, std::list<Block *> >::iterator item = m_pool2.lower_bound(block->Capacity);
        if (item == m_pool2.end())
        {
            item = m_pool2.begin();
            block->Capacity = item->first;
        }       
        memset(block->Data, 0, block->Capacity);
        block->Size = 0;
        item->second.push_back(block);
        //DEBUG_LOG("MemPool::Free(), free block, buf: " << (void *)block->Data << ", size: " << item->first);
        block = block->Next;
    }    
}

int ReadToBuffer(int fd, Block &buffer, uint32_t offset, uint32_t len)
{
    Block *block = &buffer;
    int32_t left = len;
    while (left > 0 && Socket::IsReadable(fd))
    {
        while (offset >= block->Capacity)
        {
            offset -= block->Capacity;
            block = block->Next;
            assert(block);
        }
        char *buf = block->Data + offset;
        size_t readOnce = (uint32_t)left <= block->Capacity - offset ? left : block->Capacity - offset;
        int readSize = read(fd, buf, readOnce);
        if (readSize < 0)
            return -1;
        block->Size += readSize;
        offset += readSize;
        left -= readSize;
    }
    return len - left;
}
