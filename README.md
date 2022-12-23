# Enhancing XV 6 RISC V
---
## K. Lakshmi Nirmala (2021101126)
## V. Siva Koti Reddy (2021101135)
---
> Note : The following commands are with respect to the write in vscode and execute it in vscode  and i run it on ubuntu

> This XV 6 supports schedulers like MLFQ,PBS,FCFS,RR

# Commands to exectute file

open the file ../2021101126_Assignment-4 directory
```
    $ make
```
For Default round robin scheduling
 ```
     $ make qemu                      
 ```
 For scheduler schedname MLFQ/PBS/FCFS/RANDOM(with equal ticket number assignment)
 ```
    $ make qemu SCHEDULER=schedname  
```
For Lottery scheduler and random ticket number assignment

    $ make qemu SCHEDULER=RANDOM RANDOMTICKETNUMBER=TRUE 

# Specification - 1

## System call 1 - strace

> changes in kernel folder

1. Added `np->mask = p->mask;` in fork function in proc.c file
2. Added `int mask` in proc.h file
3. Added `#define SYS_strace 22` in syscall.h
4. Added `extern uint64 sys_strace(void);` prototype in syscall.c
5. Added `[SYS_strace] sys_strace,` mapping in syscall.c
6. Created `syscall_list` and `syscall_num` arrays in syscall.c
7. Modidfied `syscall` function in syscall.c to print strace command output
8. Implemented `sys_strace` in sysproc.c
   ```
   uint64
   sys_strace(void)
   {
      int mask;
      argint(0, &mask);
      myproc()->mask = mask;
      return 0;
   }
   ```
> changes in user folder
1. Added `strace.c` file in user folder with following implementation

```
#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
  int i;
  char *nargv[MAXARG];

  if(argc < 3 || (argv[1][0] < '0' || argv[1][0] > '9')){
    fprintf(2, "Usage: %s mask command\n", argv[0]);
    exit(1);
  }

  if (strace(atoi(argv[1])) < 0) {
    fprintf(2, "%s: trace failed\n", argv[0]);
    exit(1);
  }
  
  for(i = 2; i < argc && i < MAXARG; i++){
    nargv[i-2] = argv[i];
  }
  exec(nargv[0], nargv);
  exit(0);
}
```

2. Added `int strace(int)` in defs.h
3. Added `entry("strace")` in usyls.pl
4. Included  `$U/_strace\`  in UPROGS in makefile

## system call 2: sigalarm and sigreturn
> In kernel folder
1.  Added variables in proc.h file

 ```
  int duration;                     
  int alarm;                         
  uint64 handler;                    
  struct trapframe *alarm_trapframe; 
 ```
3. Added `#define SYS_sigalarm 23` and ` #define SYS_sigreturn 24` in syscall.h
4. Added `extern uint64 sys_sigalarm(void);` and `extern uint64 sys_sigreturn(void);`  prototype in syscall.c
5. Added `[SYS_sigalarm] sys_sigalarm,`  and `[SYS_sigreturn] sys_sigreturn,` mapping in syscall.c
6. Added following in proc.c
```
 p->alarm = 0;
 p->duration = 0;
 p->handler = 0;
 if (p->alarm_trapframe)
    kfree((void *)p->alarm_trapframe);
 p->alarm_trapframe = 0;
```
7. Implemented prototypes in syscall.c
```
uint64
sys_sigalarm(void)
{
  int ticks;
  uint64 handler;
  argint(0, &ticks);
  argaddr(1, &handler) ;
    
  
  struct proc* p = myproc();
  p->alarm = ticks;
  p->handler = handler;
  p->duration = 0;
  p->alarm_trapframe = 0;
  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc* p = myproc();
  if(p->alarm_trapframe != 0){
    memmove(p->trapframe, p->alarm_trapframe, 512);
    kfree(p->alarm_trapframe);
    p->alarm_trapframe = 0;
  }
  return 0;
}
```

7. Added following commands to handle interrupts in `usertrap` function in  trap.c file

```
if (which_dev == 2)
  {
    if (p->alarm != 0)
    {
      p->duration++;
      if (p->duration == p->alarm)
      {
        p->duration = 0;
        if (p->alarm_trapframe == 0)
        {
          p->alarm_trapframe = kalloc();
          memmove(p->alarm_trapframe, p->trapframe, 512);
          p->trapframe->epc = p->handler;
        }
        else
        {
          yield();
        }
      }
      else
      {
        yield();
      }
    }
    else
    {
      yield();
    }
  }
 ```
 > In user folder
 
 1. Defined two functions in user.h file
 ```
int sigalarm(int ticks, void (*handler)());
int sigreturn(void);
 ```
 2. To test these commands i added alarmtest.c file in user folder
 ```

#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/riscv.h"
#include "user/user.h"

void test0();
void test1();
void test2();
void periodic();
void slow_handler();

int
main(int argc, char *argv[])
{
  test0();
  test1();
  test2();
  exit(0);
}

volatile static int count;

void
periodic()
{
  count = count + 1;
  printf("alarm!\n");
  sigreturn();
}


void
test0()
{
  int i;
  printf("test0 start\n");
  count = 0;
  sigalarm(2, periodic);
  for(i = 0; i < 1000*500000; i++){
    if((i % 1000000) == 0)
      write(2, ".", 1);
    if(count > 0)
      break;
  }
  sigalarm(0, 0);
  if(count > 0){
    printf("test0 passed\n");
  } else {
    printf("\ntest0 failed: the kernel never called the alarm handler\n");
  }
}

void __attribute__ ((noinline)) foo(int i, int *j) {
  if((i % 2500000) == 0) {
    write(2, ".", 1);
  }
  *j += 1;
}
void
test1()
{
  int i;
  int j;

  printf("test1 start\n");
  count = 0;
  j = 0;
  sigalarm(2, periodic);
  for(i = 0; i < 500000000; i++){
    if(count >= 10)
      break;
    foo(i, &j);
  }
  if(count < 10){
    printf("\ntest1 failed: too few calls to the handler\n");
  } else if(i != j){
   
    printf("\ntest1 failed: foo() executed fewer times than it was called\n");
  } else {
    printf("test1 passed\n");
  }
}


void
test2()
{
  int i;
  int pid;
  int status;

  printf("test2 start\n");
  if ((pid = fork()) < 0) {
    printf("test2: fork failed\n");
  }
  if (pid == 0) {
    count = 0;
    sigalarm(2, slow_handler);
    for(i = 0; i < 1000*500000; i++){
      if((i % 1000000) == 0)
        write(2, ".", 1);
      if(count > 0)
        break;
    }
    if (count == 0) {
      printf("\ntest2 failed: alarm not called\n");
      exit(1);
    }
    exit(0);
  }
  wait(&status);
  if (status == 0) {
    printf("test2 passed\n");
  }
}

void
slow_handler()
{
  count++;
  printf("alarm!\n");
  if (count > 1) {
    printf("test2 failed: alarm handler called more than once\n");
    exit(1);
  }
  for (int i = 0; i < 1000*500000; i++) {
    asm volatile("nop"); // avoid compiler optimizing away loop
  }
  sigalarm(0, 0);
  sigreturn();
}
```
3. Added `entry("sigalarm");` and `entry("sigreturn");` in usyls.pl
4. Included  `$U/_alarmtest\`  in UPROGS in makefile

## Specification-2 : SCHEDULING

1. FCFS

- implemented in kernel/proc.c
- to run 
```
    $make qemu SCHEDULER=FCFS
```

2. PBS

- implemented in kernel/proc.c
- to run
```
    $make qemu SCHEDULER=PBS
```

3. Lottery Based

- For Lottery scheduler and random ticket number assignment
```
    $ make qemu SCHEDULER=RANDOM RANDOMTICKETNUMBER=TRUE 
```
4. MLFQ

- to run
```
    $make qemu SCHEDULER=MLFQ
```

changes CPU to 1 to getrid of kerneltrap() panic;

## Specification-3 : COW
> changes in kernel folder
1. Defined two functions in defs.h
```
int             uvmcheckcowpage(uint64 va);
int             uvmcowcopy(uint64 va);
```

2. Implemented defines functions in vm.c
```
int uvmcheckcowpage(uint64 va)
{
  pte_t *pte;
  struct proc *p = myproc();

  return va < p->sz                                                    // within size of memory for the process
         && ((pte = walk(p->pagetable, va, 0)) != 0) && (*pte & PTE_V) // page table entry exists
         && (*pte & PTE_COW);                                          // page is a cow page
}

// Copy the cow page, then map it as writable
int uvmcowcopy(uint64 va)
{
  pte_t *pte;
  struct proc *p = myproc();

  if ((pte = walk(p->pagetable, va, 0)) == 0)
    panic(" uvmcowcopy: walk ");

  // copy the cow page
  // (no copying will take place if reference count is already 1)
  uint64 pa = PTE2PA(*pte);
  uint64 new = (uint64)kcopy_n_deref((void *)pa);
  if (new == 0)
    return -1;

  // map as writable, remove the cow flag
  uint64 flags = (PTE_FLAGS(*pte) | PTE_W) & ~PTE_COW;
  uvmunmap(p->pagetable, PGROUNDDOWN(va), 1, 0);
  if (mappages(p->pagetable, va, 1, new, flags) == -1)
  {
    panic(" uvmcowcopy: mappages ");
  }
  return 0;
}
```
 3. Added few lines in `usertrap` function in  trap.c file
 ```
 else if ((r_scause() == 13 || r_scause() == 15) && uvmcheckcowpage(r_stval()))
  { 
    if (uvmcowcopy(r_stval()) == -1)
    {
      setkilled(p);
    }
  }
 ```
 
 4. Defined three variables in kalloc.c
 ```
#define PA2PGREF_ID(p) (((p)-KERNBASE) / PGSIZE)
 #define PGREF_MAX_ENTRIES PA2PGREF_ID(PHYSTOP)
 #define PA2PGREF(p) pageref[PA2PGREF_ID((uint64)(p))]
 ```
 5. Added two functions in kalloc.c
 ```
 void *kcopy_n_deref(void *pa)
{
  acquire(&pgreflock);

  if (PA2PGREF(pa) <= 1)
  {
    release(&pgreflock);
    return pa;
  }

  uint64 newpa = (uint64)kalloc();
  if (newpa == 0)
  {
    release(&pgreflock);
    return 0; // out of memory
  }
  memmove((void *)newpa, (void *)pa, PGSIZE);
  PA2PGREF(pa)
  --;

  release(&pgreflock);
  return (void *)newpa;
}

// increase reference count of the page by one
void krefpage(void *pa)
{
  acquire(&pgreflock);
  PA2PGREF(pa)
  ++;
  release(&pgreflock);
}
```
Summary:
> changed files
 - defs.h
 - kalloc.c
 - proc.c
 - proc.h
 - syscall.c
 - syscall.h
 - sysfile.c
 - sysfile.h
 - sysproc.c
 - vm.c
 - user.h
 - usyls.pl
 - makefile
>Created files
- strace.c
- alarmtest.c
- schedulertest.c

To get rid of errors:
Do make clean and again run

Thankyou...


