#ifndef THREAD_H
#define THREAD_H

#include "list.h"
#include "os_constant.h"
#include "address_pool.h"
#include "memory.h"

typedef void (*ThreadFunction)(void *);

enum ProgramStatus
{
    CREATED,
    RUNNING,
    READY,
    BLOCKED,
    DEAD
};

struct PCB
{
    int *stack;                      // 栈指针，用于调度时保存esp
    char name[MAX_PROGRAM_NAME + 1]; // 线程名
    enum ProgramStatus status;       // 线程的状态
    int priority;                    // 线程优先级
    int pid;                         // 线程pid
    int ticks;                       // 线程时间片总时间
    int ticksPassedBy;               // 线程已执行时间
    ListItem tagInGeneralList;       // 线程队列标识
    ListItem tagInAllList;           // 线程队列标识

    int pageDirectoryAddress; // 页目录表地址
    AddressPool userVirtual;  // 用户程序虚拟地址池
    int parentPid;            // 父进程pid
    int retValue;             // 返回值
    
    mem_block_desc user_block_descs[DESC_CNT];	// 用户内存块描述符数组
};

#endif