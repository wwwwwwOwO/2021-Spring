#ifndef SWAP_H
#define SWAP_H

#include "os_type.h"
#include "address_pool.h"

#define ListItem2PDB(ADDRESS, LIST_ITEM) ((PDB *)((int)(ADDRESS) - (int)&((PDB *)0)->LIST_ITEM))

class SwapManager
{

    AddressPool hda;
    int StartSector;

public:
    SwapManager();

    void initialize();
    
    bool createPool(int SectorNum);
    // 将虚拟地址对应的页换出到磁盘中
    int swapOut(uint32 virtualAddress);
    // 将虚拟地址对应的页换入到对应的物理地址的页
    int swapIn(uint32 virtualAddress, uint32 physicalAddress);
    // 对缺页异常进行处理
    void doPageFault();
};

#endif