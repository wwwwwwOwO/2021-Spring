#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"
#include "syscall.h"
#include "tss.h"
#include "shell.h"
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
/*
void first_process()
{
    printf("Start first process.\n");

    int addr[10];
    for(int i = 0; i < 7; i++){
        addr[i] = malloc(513);
        printf("malloc(513), addr is %x\n", addr[i]);
    }

    for(int i = 0; i < 3; i++){
        printf("free %x \n", addr[i]);
        free(addr[i]);
    }

    for(int i = 7; i <10; i++){
        addr[i] = malloc(513);
        printf("malloc(513), addr is %x\n", addr[i]);
    }

    for(int i = 0; i < 10; i++){
        printf("addr[%d]: %x\n", i, addr[i]);
    }

    for(int i = 3; i < 10; i++){
        free(addr[i]);
    }
    asm_halt();

}
void second_process()
{
    printf("Start second process.\n");

    int addr[10];
    int size = 16;
    for(int i = 0; i < 10; i++){
        addr[i] = malloc(size);
        printf("malloc(%d), addr is %x\n", size, addr[i]);
        size *= 2;
    }

    for(int i = 9; i >= 0; i--){
        printf("free %x \n", addr[i]);
        free(addr[i]);
    }
    asm_halt();

}

void second_thread(void *arg){
    printf("start second thread.\n");

    int addr2 = syscall_malloc(63);
    printf("malloc(63), addr is %x\n", addr2);

    addr2 = syscall_malloc(1026);
    printf("malloc(1026), addr is %x\n", addr2);

    // programManager.executeProcess((const char *)first_process, 1);
    asm_halt();
}

void first_thread(void *arg)
{
    printf("start first thread.\n");
    programManager.executeThread(second_thread, nullptr, "second thread", 1);

    int addr1 = syscall_malloc(33);
    printf("malloc(33), addr is %x\n", addr1);

    addr1 = syscall_malloc(1025);
    printf("malloc(1025), addr is %x\n", addr1);
    
    // programManager.executeProcess((const char *)first_process, 1);
    asm_halt();
}


void second_thread(void *arg){
    printf("start second thread.\n");

    int addr[10];
    int size = 16;
    for(int i = 0; i < 10; i++){
        addr[i] = syscall_malloc(size);
        printf("malloc(%d), addr is %x\n", size, addr[i]);
        size *= 2;
    }

    for(int i = 9; i >= 0; i--){
        printf("free %x \n", addr[i]);
        syscall_free(addr[i]);
    }

    asm_halt();
}

void first_thread(void *arg)
{
    printf("Start first thread.\n");
    programManager.executeThread(second_thread, nullptr, "second thread", 1);
    programManager.executeProcess((const char *)first_process, 1);
    programManager.executeProcess((const char *)second_process, 1);

    int addr[10];
    for(int i = 0; i < 7; i++){
        addr[i] = syscall_malloc(513);
        printf("malloc(513), addr is %x\n", addr[i]);
    }

    for(int i = 0; i < 3; i++){
        printf("free %x \n", addr[i]);
        syscall_free(addr[i]);
    }

    for(int i = 7; i <10; i++){
        addr[i] = syscall_malloc(513);
        printf("malloc(513), addr is %x\n", addr[i]);
    }

    for(int i = 0; i < 10; i++){
        printf("addr[%d]: %x\n", i, addr[i]);
    }

    for(int i = 3; i < 10; i++){
        syscall_free(addr[i]);
    }
    
    asm_halt();
}
*/
void second_thread(void *arg){
    printf("Start second thread.\n");
    int add2 = memoryManager.allocateVirtualPages(AddressPoolType::KERNEL, 4);
    printf("Second thread allocate start %x\n", add2);

    asm_halt();
}

void first_thread(void *arg)
{
    printf("Start first thread.\n");
    programManager.executeThread(second_thread, nullptr, "second thread", 1);
    int add1 = memoryManager.allocateVirtualPages(AddressPoolType::KERNEL, 4);
    printf("First thread allocate start %x\n", add1);
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
    // 设置5号系统调用
    systemService.setSystemCall(5, (int)syscall_move_cursor);
    // 设置6号系统调用
    systemService.setSystemCall(6, (int)syscall_malloc);
    // 设置7号系统调用
    systemService.setSystemCall(7, (int)syscall_free);
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
