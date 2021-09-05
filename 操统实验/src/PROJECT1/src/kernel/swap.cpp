#include "swap.h"
#include "memory.h"
#include "os_constant.h"
#include "stdlib.h"
#include "asm_utils.h"
#include "stdio.h"
#include "os_modules.h"
#include "disk.h"
SwapManager::SwapManager(){

}

void SwapManager::initialize(){
    // 前2MB用于存储硬盘，后8MB用于存储换出的页
    StartSector = 2 * 1024 * 2;
    bool flag = createPool(8 * 1024 * 2);
    if(!flag)
        return ;
    
}

int SwapManager::swapOut(uint32 virtualAddress){
    // 关中断
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    // LBA地址为起始扇区地址+资源的下标*8
    int lba = StartSector + (hda.allocate(1) * 8);
    
    char *pageSector = (char *)virtualAddress;

    // 每次写入一个扇区，即512字节，共写8个扇区
    for(int i = 0;i < 8; i++){
        Disk::write(lba + i, (void *)pageSector);
        pageSector += 512;
    }

    // 将虚拟地址对应的物理页释放
    int pAddr = memoryManager.vaddr2paddr(virtualAddress);
    memoryManager.releasePhysicalPages(AddressPoolType::USER, pAddr, 1);

    // 修改页表项，将LBA地址存到高20位，并将P位置0
    int *pte = (int *)memoryManager.toPTE(virtualAddress);
    *pte = *pte & 0xffe;        // 将页表项的高20位和P位都清零
    *pte = *pte |(lba << 12);   // 将lba地址放到高20位

    // 恢复中断状态
    interruptManager.setInterruptStatus(status);

    return pAddr;
}

int SwapManager::swapIn(uint32 virtualAddress, uint32 physicalAddress){
    // 关中断
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();    

    int *pte = (int *)memoryManager.toPTE(virtualAddress);
    int lba = *pte >> 12;
    // 将物理页P位置1，只有这样才能MMU才能寻址到对应的物理页框，否则仍认为该物理页不存在内存中
    *pte = (physicalAddress & 0xfffff000) | 0x7;    // 将物理页P位置位
    
    char *pageSector = (char *)virtualAddress;

    // 每次读入一个扇区，即512字节，共读8个扇区
    for(int i = 0;i < 8; i++){
        Disk::read(lba + i, (void *)pageSector);
        pageSector += 512;
    }

    // 将对应的扇区资源释放
    hda.release((lba - StartSector) / 8, 1);
    // 恢复中断状态
    interruptManager.setInterruptStatus(status);
}

bool SwapManager::createPool(int SectorNum)
{
    int sourcesCount = SectorNum / 8;
    int bitmapLength = ceil(sourcesCount, 8);

    // 计算位图所占的页数
    int pagesCount = ceil(bitmapLength, PAGE_SIZE);

    int start = memoryManager.allocatePages(AddressPoolType::KERNEL, pagesCount);
    //printf("%x %d\n", start, pagesCount);

    if (!start)
    {
        return false;
    }

    memset((char *)start, 0, PAGE_SIZE * pagesCount);
    hda.initialize((char *)start, sourcesCount, 0);

    return true;
}

void SwapManager::doPageFault(){
    PCB *process = programManager.running;
    uint32 virtualAddress = asm_get_cr2();
    printf("Miss %x!\n", virtualAddress);

    // 若发生缺页时驻留集未满,则说明是读写权限的问题
    if(process->user_resident_set_size < process->user_resident_set_maxsize){
        printf("Access Error!\n");
        return ;
    }
    
}