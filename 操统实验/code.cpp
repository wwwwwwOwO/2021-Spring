/********************************** Assignment 1 ************************************/
/* 编写系统调用 */
#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"
#include "syscall.h"
// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
// 内存管理器
MemoryManager memoryManager;
// 系统调用
SystemService systemService;
int syscall_0(int first, int second, int third, int forth, int fifth)
{
    printf("systerm call 0: %d, %d, %d, %d, %d\n",
           first, second, third, forth, fifth);
    return first + second + third + forth + fifth;
}
int syscall_7(int a, int b)
{
    if(a + b == 7)
    printf("systerm call 7: 19335074 wenny\n");
    return a + b;
}
void first_thread(void *arg)
{
    asm_halt();
}
extern "C" void setup_kernel()
{
    // 中断管理器
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);
    // 输出管理器
    stdio.initialize();
    // 进程/线程管理器
    programManager.initialize();
    // 内存管理器
    memoryManager.openPageMechanism();
    memoryManager.initialize();
    // 初始化系统调用
    systemService.initialize();
    // 设置0号系统调用
    systemService.setSystemCall(0, (int)syscall_0);
    // 设置7号系统调用
    systemService.setSystemCall(7, (int)syscall_7);
    int ret;
    ret = asm_system_call(7, 7);
    printf("return value: %d\n", ret);
    ret = asm_system_call(7, 3, 4);
    printf("return value: %d\n", ret);
    ret = asm_system_call(7, 3, 6);
    printf("return value: %d\n", ret);
    ret = asm_system_call(7, 7, -1);
    printf("return value: %d\n", ret);
    // 创建第一个线程
    int pid = programManager.executeThread(first_thread, nullptr, "first thread", 1);
    if (pid == -1)
    {
        printf("can not execute thread\n");
        asm_halt();
    }
    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);
    firstThread->status = RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);
    asm_halt();
}







/********************************** Assignment 3 ************************************/
/* 回收僵尸进程的方法 */
#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"
#include "syscall.h"
#include "tss.h"
// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
// 内存管理器
MemoryManager memoryManager;
// 系统调用
SystemService systemService;
// Task State Segment
TSS tss;
int syscall_0(int first, int second, int third, int forth, int fifth)
{
    printf("systerm call 0: %d, %d, %d, %d, %d\n",
           first, second, third, forth, fifth);
    return first + second + third + forth + fifth;
}
void Init()
{
    int pid;
    int retval;
    while (true)
    {
        uint32 tmp = 0x1fffffff;
        while (tmp)
            --tmp;       
        while((pid = wait(&retval)) != -1){
            printf("wait for a child process, pid: %d, return value: %d\n", pid, retval);
        }
        printf("all child process of Init exit, programs: %d\n", programManager.allPrograms.size());
    }
}
void first_process()
{
    int pid = fork(); // 第一个子进程
    int retval;
    if (pid) // 主进程
    {
        while ((pid = wait(&retval)) != -1)
        {
            printf("wait for a child process, pid: %d, return value: %d\n", pid, retval);
        }
        printf("all child process of first process exit, programs: %d\n", programManager.allPrograms.size());
        asm_halt();
    }
    else // 进入第一个子进程
    { 
        pid = fork();  // 第一个子进程的子进程
        // 释放第一个子进程
        if(pid){
            exit(6);
        }
        uint32 tmp = 0xffffff;
        while (tmp)
            --tmp;
        printf("child's child, pid: %d\n", programManager.running->pid);
        exit(666);
    }
}
void second_thread(void *arg)
{
    printf("thread exit\n");
    exit(0);
}
void first_thread(void *arg)
{
    printf("start process\n");
    programManager.executeProcess((const char *)Init, 1);
    programManager.executeProcess((const char *)first_process, 1);
    programManager.executeThread(second_thread, nullptr, "second", 1);
    asm_halt();
}
extern "C" void setup_kernel()
{
    // 中断管理器
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);
    // 输出管理器
    stdio.initialize();
    // 进程/线程管理器
    programManager.initialize();
    // 初始化系统调用
    systemService.initialize();
    // 设置0号系统调用
    systemService.setSystemCall(0, (int)syscall_0);
    // 设置1号系统调用
    systemService.setSystemCall(1, (int)syscall_write);
    // 设置2号系统调用
    systemService.setSystemCall(2, (int)syscall_fork);
    // 设置3号系统调用
    systemService.setSystemCall(3, (int)syscall_exit);
    // 设置4号系统调用
    systemService.setSystemCall(4, (int)syscall_wait);
    // 内存管理器
    memoryManager.initialize();
    // 创建第一个线程
    int pid = programManager.executeThread(first_thread, nullptr, "first thread", 1);
    if (pid == -1)
    {
        printf("can not execute thread\n");
        asm_halt();
    }
    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);
    firstThread->status = ProgramStatus::RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);
    asm_halt();
}
void ProgramManager::exit(int ret)
{
    interruptManager.disableInterrupt();
    PCB *program = this->running;
    program->retValue = ret;
    program->status = ProgramStatus::DEAD;
    int *pageDir, *page;
    int paddr;
    if (program->pageDirectoryAddress)
    {
        pageDir = (int *)program->pageDirectoryAddress;
        for (int i = 0; i < 768; ++i)
        {
            if (!(pageDir[i] & 0x1))
            {
                continue;
            }
            page = (int *)(0xffc00000 + (i << 12));
            for (int j = 0; j < 1024; ++j)
            {
                if (!(page[j] & 0x1))
                {
                    continue;
                }
                paddr = memoryManager.vaddr2paddr((i << 22) + (j << 12));
                memoryManager.releasePhysicalPages(AddressPoolType::USER, paddr, 1);
            }
            paddr = memoryManager.vaddr2paddr((int)page);
            memoryManager.releasePhysicalPages(AddressPoolType::USER, paddr, 1);
        }
        memoryManager.releasePages(AddressPoolType::KERNEL, (int)pageDir, 1);
        int bitmapBytes = ceil(program->userVirtual.resources.length, 8);
        int bitmapPages = ceil(bitmapBytes, PAGE_SIZE);
        memoryManager.releasePages(AddressPoolType::KERNEL, (int)program->userVirtual.resources.bitmap, bitmapPages);
    }
    int parentPid = program->parentPid;
    if(PCB_SET_STATUS[parentPid]){
        PCB *Parent = (PCB *)((int)PCB_SET + parentPid * PCB_SIZE);
        if(Parent->status == ProgramStatus::DEAD){
            program->parentPid = 1;
        }
    }
    else
        program->parentPid = 1;
    schedule();
}
