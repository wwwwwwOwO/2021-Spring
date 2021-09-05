#include "memory.h"
#include "os_constant.h"
#include "stdlib.h"
#include "asm_utils.h"
#include "stdio.h"
#include "program.h"
#include "os_modules.h"
MemoryManager::MemoryManager()
{
    initialize();
}

void MemoryManager::initialize()
{
    /* 初始化地址池 */ 
    this->totalMemory = 0;
    this->totalMemory = getTotalMemory();

    // 预留的内存
    int usedMemory = 256 * PAGE_SIZE + 0x100000;
    if (this->totalMemory < usedMemory)
    {
        printf("memory is too small, halt.\n");
        asm_halt();
    }
    // 剩余的空闲的内存
    int freeMemory = this->totalMemory - usedMemory;

    int freePages = freeMemory / PAGE_SIZE;
    int kernelPages = freePages / 2;
    int userPages = freePages - kernelPages;

    int kernelPhysicalStartAddress = usedMemory;
    int userPhysicalStartAddress = usedMemory + kernelPages * PAGE_SIZE;

    int kernelPhysicalBitMapStart = BITMAP_START_ADDRESS;
    int userPhysicalBitMapStart = kernelPhysicalBitMapStart + ceil(kernelPages, 8);
    int kernelVirtualBitMapStart = userPhysicalBitMapStart + ceil(userPages, 8);

    kernelPhysical.initialize(
        (char *)kernelPhysicalBitMapStart,
        kernelPages,
        kernelPhysicalStartAddress);

    userPhysical.initialize(
        (char *)userPhysicalBitMapStart,
        userPages,
        userPhysicalStartAddress);

    kernelVirtual.initialize(
        (char *)kernelVirtualBitMapStart,
        kernelPages,
        KERNEL_VIRTUAL_START);

    printf("total memory: %d bytes ( %d MB )\n",
           this->totalMemory,
           this->totalMemory / 1024 / 1024);

    printf("kernel pool\n"
           "    start address: 0x%x\n"
           "    total pages: %d ( %d MB )\n"
           "    bitmap start address: 0x%x\n",
           kernelPhysicalStartAddress,
           kernelPages, kernelPages * PAGE_SIZE / 1024 / 1024,
           kernelPhysicalBitMapStart);

    printf("user pool\n"
           "    start address: 0x%x\n"
           "    total pages: %d ( %d MB )\n"
           "    bit map start address: 0x%x\n",
           userPhysicalStartAddress,
           userPages, userPages * PAGE_SIZE / 1024 / 1024,
           userPhysicalBitMapStart);

    printf("kernel virtual pool\n"
           "    start address: 0x%x\n"
           "    total pages: %d  ( %d MB ) \n"
           "    bit map start address: 0x%x\n",
           KERNEL_VIRTUAL_START,
           userPages, kernelPages * PAGE_SIZE / 1024 / 1024,
           kernelVirtualBitMapStart);
    
    /* 初始化kernel_block_descs */ 
    block_desc_init(kernel_block_descs);
}

int MemoryManager::allocatePhysicalPages(enum AddressPoolType type, const int count)
{
    int start = -1;

    if (type == AddressPoolType::KERNEL)
    {
        start = kernelPhysical.allocate(count);
    }
    else if (type == AddressPoolType::USER)
    {
        start = userPhysical.allocate(count);
    }

    return (start == -1) ? 0 : start;
}

void MemoryManager::releasePhysicalPages(enum AddressPoolType type, const int paddr, const int count)
{
    if (type == AddressPoolType::KERNEL)
    {
        kernelPhysical.release(paddr, count);
    }
    else if (type == AddressPoolType::USER)
    {

        userPhysical.release(paddr, count);
    }
}

int MemoryManager::getTotalMemory()
{

    if (!this->totalMemory)
    {
        int memory = *((int *)MEMORY_SIZE_ADDRESS);
        // ax寄存器保存的内容
        int low = memory & 0xffff;
        // bx寄存器保存的内容
        int high = (memory >> 16) & 0xffff;

        this->totalMemory = low * 1024 + high * 64 * 1024;
    }

    return this->totalMemory;
}

int MemoryManager::allocatePages(enum AddressPoolType type, const int count)
{
    // 第一步：从虚拟地址池中分配若干虚拟页
    int virtualAddress = allocateVirtualPages(type, count);
    if (!virtualAddress)
    {
        return 0;
    }

    bool flag;
    int physicalPageAddress;
    int vaddress = virtualAddress;

    // 依次为每一个虚拟页指定物理页
    for (int i = 0; i < count; ++i, vaddress += PAGE_SIZE)
    {
        flag = false;
        // 第二步：从物理地址池中分配一个物理页
        physicalPageAddress = allocatePhysicalPages(type, 1);
        if (physicalPageAddress)
        {
            //printf("allocate physical page 0x%x\n", physicalPageAddress);

            // 第三步：为虚拟页建立页目录项和页表项，使虚拟页内的地址经过分页机制变换到物理页内。
            flag = connectPhysicalVirtualPage(vaddress, physicalPageAddress);
        }
        else
        {
            flag = false;
        }

        // 分配失败，释放前面已经分配的虚拟页和物理页表
        if (!flag)
        {
            // 前i个页表已经指定了物理页
            releasePages(type, virtualAddress, i);
            // 剩余的页表未指定物理页
            releaseVirtualPages(type, virtualAddress + i * PAGE_SIZE, count - i);
            return 0;
        }
    }

    return virtualAddress;
}

int MemoryManager::allocateVirtualPages(enum AddressPoolType type, const int count)
{
    int start = -1;

    if (type == AddressPoolType::KERNEL)
    {
        start = kernelVirtual.allocate(count);
    }
    else if (type == AddressPoolType::USER)
    {
        start = programManager.running->userVirtual.allocate(count);
    }

    return (start == -1) ? 0 : start;
}

bool MemoryManager::connectPhysicalVirtualPage(const int virtualAddress, const int physicalPageAddress)
{
    // 计算虚拟地址对应的页目录项和页表项
    int *pde = (int *)toPDE(virtualAddress);
    int *pte = (int *)toPTE(virtualAddress);

    // 页目录项无对应的页表，先分配一个页表
    if (!(*pde & 0x00000001))
    {
        // 从内核物理地址空间中分配一个页表
        int page = allocatePhysicalPages(AddressPoolType::KERNEL, 1);
        if (!page)
            return false;

        // 使页目录项指向页表
        *pde = page | 0x7;
        // 初始化页表
        char *pagePtr = (char *)(((int)pte) & 0xfffff000);
        memset(pagePtr, 0, PAGE_SIZE);
    }

    // 使页表项指向物理页
    *pte = physicalPageAddress | 0x7;

    return true;
}

int MemoryManager::toPDE(const int virtualAddress)
{
    return (0xfffff000 + (((virtualAddress & 0xffc00000) >> 22) * 4));
}

int MemoryManager::toPTE(const int virtualAddress)
{
    return (0xffc00000 + ((virtualAddress & 0xffc00000) >> 10) + (((virtualAddress & 0x003ff000) >> 12) * 4));
}

void MemoryManager::releasePages(enum AddressPoolType type, const int virtualAddress, const int count)
{
    int vaddr = virtualAddress;
    int *pte;
    for (int i = 0; i < count; ++i, vaddr += PAGE_SIZE)
    {
        // 第一步，对每一个虚拟页，释放为其分配的物理页
        releasePhysicalPages(type, vaddr2paddr(vaddr), 1);

        // 设置页表项为不存在，防止释放后被再次使用
        pte = (int *)toPTE(vaddr);
        *pte = 0;
    }

    // 第二步，释放虚拟页
    releaseVirtualPages(type, virtualAddress, count);
}

int MemoryManager::vaddr2paddr(int vaddr)
{
    int *pte = (int *)toPTE(vaddr);
    int page = (*pte) & 0xfffff000;
    int offset = vaddr & 0xfff;
    return (page + offset);
}

void MemoryManager::releaseVirtualPages(enum AddressPoolType type, const int vaddr, const int count)
{
    if (type == AddressPoolType::KERNEL)
    {
        kernelVirtual.release(vaddr, count);
    }
    else if (type == AddressPoolType::USER)
    {
        programManager.running->userVirtual.release(vaddr, count);
    }
}


void MemoryManager::block_desc_init(mem_block_desc* desc_array) {				   
    int block_size = 16;

   /* 初始化每个mem_block_desc描述符 */
   for (int desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
      desc_array[desc_idx].block_size = block_size;

      /* 初始化arena中的内存块数量 */
      desc_array[desc_idx].blocks_per_arena = (PAGE_SIZE - sizeof(arena)) / block_size;	  

      desc_array[desc_idx].free_list.initialize();

      block_size *= 2;         // 更新为下一个规格内存块
   }
}


mem_block* MemoryManager::arena2block(arena* a, int idx) {
  return (mem_block*)((int)a + sizeof(arena) + idx * (a->desc->block_size)); /* 返回arena中第idx个内存块的地址 */
}

arena* MemoryManager::block2arena(mem_block* b) {
   return (arena*)((int)b & 0xfffff000);     /* 返回内存块b所在的arena地址 */
}

int MemoryManager::malloc(const int size) {
    // 关中断，防止malloc的过程被打断
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    enum AddressPoolType  type;
    int pool_size;
    mem_block_desc* descs;
    PCB* cur = programManager.running;

/* 判断用哪个内存池*/
    if (!(cur->pageDirectoryAddress)) {     // 若为内核线程
        type = AddressPoolType::KERNEL; 
        pool_size = this->kernelPhysical.resources.length;
        descs = kernel_block_descs;
    } 
    else {				      // 用户进程pcb中的pgdir会在为其分配页表时创建
        type = AddressPoolType::USER; 
        pool_size = this->userPhysical.resources.length;
        descs = cur->user_block_descs;
    }


   /* 若申请的内存不在内存池容量范围内则直接返回0 */
    if (!(size > 0 && size < pool_size)) {
            // 恢复中断
            interruptManager.setInterruptStatus(status);
            return 0;
    }
   
   arena* a;
   mem_block* b;	
/* 超过最大内存块1024, 就分配页框 */
   if (size > 1024) {
        int page_cnt = ceil(size + sizeof(arena), PAGE_SIZE);    // 向上取整需要的页框数
        a = (arena*)allocatePages(type, page_cnt);

        if (!a) {
            interruptManager.setInterruptStatus(status);
            return 0; 
        } 
        else {
	        memset(a, 0, page_cnt * PAGE_SIZE);	 // 将分配的内存清0  
            /* 对于分配的大块页框,将desc置为0, cnt置为页框数,large置为true */
            a->desc = 0;
            a->cnt = page_cnt;
            a->large = true;

            // printf("%x\n", a);
            interruptManager.setInterruptStatus(status);
            return (int)(a + 1);		 // 跨过arena大小，把剩下的内存返回
        }

   } 
   else 
   {    // 若申请的内存小于等于1024,可在各种规格的mem_block_desc中去适配
        int desc_idx;
      /* 从内存块描述符中匹配合适的内存块规格 */
        for (desc_idx = 0; desc_idx < DESC_CNT; desc_idx++) {
            if (size <= descs[desc_idx].block_size) {  // 从小往大后,找到后退出
                break;
            }
        }

   	    // printf(" %d\n", desc_idx);
   /* 若mem_block_desc的free_list中已经没有可用的mem_block,
    * 就创建新的arena提供mem_block */
        if(descs[desc_idx].free_list.empty()){
            a = (arena*)allocatePages(type, 1);       // 分配1页框做为arena
            if(!a){
                interruptManager.setInterruptStatus(status);
                return 0;
            }
            // printf("a: %x\n", a);

            memset(a, 0, PAGE_SIZE);

            /* 对于分配的小块内存,将desc置为相应内存块描述符, 
            * cnt置为此arena可用的内存块数,large置为false */
            a->desc = &descs[desc_idx];
            a->large = false;
            a->cnt = descs[desc_idx].blocks_per_arena;

            // printf("a->desc: %x\n a->cnt: %d\n", a->desc, a->cnt);
            /* 开始将arena拆分成内存块,并添加到内存块描述符的free_list中 */
            for (int block_idx = 0; block_idx < descs[desc_idx].blocks_per_arena; block_idx++) {
                b = arena2block(a, block_idx);
                // 确保未被分配过
                if(descs[desc_idx].free_list.find(&(b->free_elem)) != -1){
                    releasePages(type, (int)a, 1);
                    interruptManager.setInterruptStatus(status);
                    return 0;
                }
                descs[desc_idx].free_list.push_back(&(b->free_elem));
            }
        }

        /* 开始分配内存块 */
        ListItem* item = descs[desc_idx].free_list.front();
        b = ListItem2Block(item, free_elem);
        descs[desc_idx].free_list.pop_front();

        memset(b, 0, descs[desc_idx].block_size);

        a = block2arena(b);  // 获取内存块b所在的arena
        a->cnt--;		   // 将此arena中的空闲内存块数减1

        interruptManager.setInterruptStatus(status);
        return (int)b;
   }
}

void MemoryManager::free(const int ptr) {
    if (!ptr)
        return ;

    // 关中断，防止free的过程被打断
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    enum AddressPoolType  type;
    PCB* cur = programManager.running;

    /* 判断是线程还是进程 */
    if (!(cur->pageDirectoryAddress)) {     // 若为内核线程
        type = AddressPoolType::KERNEL; 
    } 
    else {				      // 用户进程pcb中的pgdir会在为其分配页表时创建
        type = AddressPoolType::USER; 
    }

    mem_block* b = (mem_block*)ptr;
    arena* a = block2arena(b);	     // 把mem_block转换成arena,获取元信息
    
    if (a->desc == 0 && a->large == true) { // 大于1024的内存
        releasePages(type, (int)a, a->cnt);
    } 
    else 
    {				 // 小于等于1024的内存块
	    /* 先将内存块回收到free_list */
        a->desc->free_list.push_back(&(b->free_elem));

        /* 再判断此arena中的内存块是否都是空闲,如果是就释放arena */
        if (++a->cnt == a->desc->blocks_per_arena) {    // 如果该arena空闲
            for (int block_idx = 0; block_idx < a->desc->blocks_per_arena; block_idx++) {
                b = arena2block(a, block_idx);
                if(a->desc->free_list.find(&(b->free_elem))==-1){
                    interruptManager.setInterruptStatus(status);
                    return;
                }
                a->desc->free_list.erase(&(b->free_elem));
            }
            releasePages(type, (int)a, 1);
        } 
    }   

    interruptManager.setInterruptStatus(status);
}