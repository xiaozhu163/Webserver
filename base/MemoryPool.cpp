#include "MemoryPool.h"

MemoryPool::MemoryPool() {}
MemoryPool::~MemoryPool()
{
    Slot *cur = currentBlock_;
    while (cur)
    {
        Slot *next = cur->next;
        // free(reinterpret_cast<void *>(cur));
        // 转化为 void 指针，是因为 void 类型不需要调⽤析构函数,只释放空间
        operator delete(reinterpret_cast<void *>(cur));
        cur = next;
    }
}
void MemoryPool::init(int size)
{
    assert(size > 0);
    slotSize_ = size;
    currentBlock_ = NULL;
    currentSlot_ = NULL;
    lastSlot_ = NULL;
    freeSlot_ = NULL;
}
// 计算对⻬所需补的空间
inline size_t MemoryPool::padPointer(char *p, size_t align)
{
    if (align == 0)
    {
        std::cerr << "Error: Alignment must be non-zero." << std::endl;
        throw std::invalid_argument("Alignment must be non-zero.");
    }
    size_t result = reinterpret_cast<size_t>(p);
    size_t padding = (align - (result % align)) % align;
    return padding;
}

int i=0;

Slot *MemoryPool::allocateBlock()
{
    char *newBlock = reinterpret_cast<char *>(operator new(BlockSize));
    char *body = newBlock + sizeof(Slot *);
    // Debug output
    std::cerr << i++ << "  allocateBlock() called." << std::endl;
    std::cerr << "slotSize_: " << slotSize_ << std::endl;
    std::cerr << "body address: " << static_cast<void*>(body) << std::endl;

    size_t bodyPadding = padPointer(body, static_cast<size_t>(slotSize_));
    
    // Debug output for padding
    std::cerr << "bodyPadding: " << bodyPadding << std::endl;

    Slot *useSlot;
    {
        MutexLockGuard lock(mutex_other_);
        reinterpret_cast<Slot *>(newBlock)->next = currentBlock_;
        currentBlock_ = reinterpret_cast<Slot *>(newBlock);
        currentSlot_ = reinterpret_cast<Slot *>(body + bodyPadding);
        lastSlot_ = reinterpret_cast<Slot *>(newBlock + BlockSize - slotSize_);
        
        // Ensure currentSlot_ does not exceed lastSlot_
        if (currentSlot_ > lastSlot_)
        {
            std::cerr << "Error: Block size too small for alignment requirements." << std::endl;
            throw std::runtime_error("Block size too small for alignment requirements.");
        }

        useSlot = currentSlot_;
        currentSlot_ += (slotSize_ >> 3);
    }
    return useSlot;
}

Slot *MemoryPool::nofree_solve()
{
    if (currentSlot_ >= lastSlot_)
        return allocateBlock();
    Slot *useSlot;
    {
        MutexLockGuard lock(mutex_other_);
        useSlot = currentSlot_;
        currentSlot_ += (slotSize_ >> 3);
    }
    return useSlot;
}

Slot *MemoryPool::allocate()
{
    if (freeSlot_)
    {
        {
            MutexLockGuard lock(mutex_freeSlot_);
            if (freeSlot_)
            {
                Slot *result = freeSlot_;
                freeSlot_ = freeSlot_->next;
                return result;
            }
        }
    }
    return nofree_solve();
}
inline void MemoryPool::deAllocate(Slot *p)
{
    if (p)
    {
        // 将slot加⼊释放队列
        MutexLockGuard lock(mutex_freeSlot_);
        p->next = freeSlot_;
        freeSlot_ = p;
    }
}
MemoryPool &get_MemoryPool(int id)
{
    static MemoryPool memorypool_[64];
    return memorypool_[id];
}
// 数组中分别存放Slot⼤⼩为8，16，...，512字节的BLock链表
void init_MemoryPool()
{
    for (int i = 0; i < 64; ++i)
    {
        MemoryPool &pool = get_MemoryPool(i);
        pool.init((i + 1) << 3);
        // std::cerr << "Initialized MemoryPool for slot size " << ((i + 1) << 3) << std::endl;
    }
}

// 超过512字节就直接new
void *use_Memory(size_t size)
{
    if (size == 0)
        return nullptr;
    if (size > 512)
        //提示
        std::cout << "size > 512" <<std::endl;
        return operator new(size);

    int index = ((size + 7) >> 3) - 1;
    if (index < 0 || index >= 64)
    {
        std::cerr << "Error: Size out of range." << std::endl;
        throw std::out_of_range("Size out of range.");
    }
    return reinterpret_cast<void *>(get_MemoryPool(index).allocate());
}


void free_Memory(size_t size, void *p)
{
    if (!p)
        return;
    if (size > 512)
    {
        operator delete(p);
        return;
    }
    get_MemoryPool(((size + 7) >> 3) - 1).deAllocate(reinterpret_cast<Slot *>(p));
}
