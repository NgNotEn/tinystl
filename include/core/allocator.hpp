/*
    该 allocator 参考G2.9 alloc 实现了 内存池管理
    本库不实现并发版本，固可能与实际G2.9的源码有所出入，
    且在内存malloc时，不是增量malloc，每次仅申请 40 倍的块大小
    此文件未实现过于繁杂的边界处理，以及内存错误类的try-catch
    未实现底层的内存回收 ————因为G2.9也没设计，
    这样会导致多任务系统中内存的过度占用，
    我想这就是后续GNU C舍弃该版本的原因吧
*/
#if !defined(size_t)
#define size_t unsigned int
#endif // size_t，不知道为什么GNU C 没有声明 size_t


#if !defined(_MY_ALLOCATOR_)
#define _MY_ALLOCATOR_
#include <cstdlib>
// template<bool threads, int inst>
class myAllocator {
private:
    // 辅助函数: 用于实现 将size向上对齐到8的倍数 
    static inline size_t RoundUp(size_t size) {
        return (size + 7) & ~7;
        /*原理：
            size+7 不会让size超过向上对齐的8倍数
            ~7 = ~0...011 = 1...100
            按位与后结果等效为(size+7) - (size+7) % 8
            但位运算更快更稳定
        */
    }
    // 辅助函数: 用于实现 找到对应 字节数 所在空闲链表的 下标 size 已为8的倍数
    static inline size_t FREELIST_INDEX(size_t size) {
        return (size >> 3) - 1;
    }

private:
    // emmbeded pointer 嵌入式指针
    struct obj{
        struct obj* next = nullptr; // 用于实现链表
    };
    static obj* free_lists[16]; // 多级链表

    // 内存池指针，两指针之间即已申请却未分配的内存
    static char* pool_start;
    static char* pool_end;

    // 一级分配：空闲链表非空
    static void* freelist_allocate(obj*&);
    // 二级分配：空闲链表不够则向内存池要
    static void* pool_allocate(size_t, obj*&);
    // 三级分配：内存池不够向底层要
    static void* refill(size_t, obj*&);

public:
    // 对外接口、即申请与归还、构造等
    static void* allocate(size_t);
    static void deallocate(void*, size_t);
    myAllocator() = default;
};

// 返回一个块供使用
void* myAllocator::allocate(size_t size) {
    // 处理边界
    if(size == 0) return nullptr;
    if(size > 128) { return malloc(size); }


    // 调整size
    size = RoundUp(size);
    // 定位链表
    obj*& freelist = free_lists[FREELIST_INDEX(size)];

    /* 下面开始链表操作 */

    // 链表不为空
    if(freelist) {
        return freelist_allocate(freelist);
    }
    else if(pool_end - pool_start > size) // 内存池够用
    {
        return pool_allocate(size, freelist);
    }
    else // 内存池不足
    {
        return refill(size, freelist);
    }
}

void myAllocator::deallocate(void* block, size_t size) {
    // 处理边界
    if(size == 0) return;
    if(size > 128) {
        free(block);
    }


    // 调整size
    size = RoundUp(size);
    // 定位链表
    obj*& freelist = free_lists[FREELIST_INDEX(size)];
    // 回收
    static_cast<obj*>(block)->next = freelist;
    freelist = static_cast<obj*>(block);
}

void* myAllocator::freelist_allocate(obj*& freelist) {
    obj* block = freelist;
    freelist = freelist->next;
    return block;
}



void* myAllocator::pool_allocate(size_t size, obj*& freelist) {
    // 查看内存池大小
    size_t toltal_bytes = pool_end - pool_start;
    // 查看能切割多少个块
    size_t block_num = toltal_bytes / size;
    // 至多切20块
    if(block_num > 20) block_num = 20;
    // 切割
    obj* block = (obj*)pool_start;
    obj* head = block;
    
    for (size_t i = 1; i < block_num; ++i) {
        block->next = (obj*)((char*)block + size);
        block = block->next;
    }
    block->next = nullptr;
    
    // 定位链表
    freelist = head;
    // 更新内存池指针
    pool_start += block_num * size;

    return freelist_allocate(freelist);
}

void* myAllocator::refill(size_t size, obj*& freelist) {
    // 先清理内存池碎片
    deallocate(pool_start, pool_end - pool_start);
    // 挖一大块内存，放入内存池
    size_t ttsize = size * 40;
    pool_start = (char*)malloc(ttsize);
    pool_end = pool_start + ttsize; // 40 [)
    
    return pool_allocate(size, freelist);
}

// 静态初始化
myAllocator::obj* myAllocator::free_lists[16] = {nullptr};
char* myAllocator::pool_start = nullptr;
char* myAllocator::pool_end = nullptr;

#endif

