#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_trace(void)
{
  int mask;
  
  argint(0, &mask);
  myproc()->mask = mask;
  return 0;
}

uint64
sys_sigalarm(void)
{
  int cputicks;
  uint64 handler;
  argint(0, &cputicks);
  argaddr(1,&handler);

  if (cputicks < 0)
    return -1;

  struct proc *p = myproc();
  p->cputicks = cputicks;
  p->handler = handler;

  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc();

  memmove(p->trapframe, p->initial_trapframe, sizeof(struct trapframe));
  p->cputicks = p->passed_cputicks;
  p->passed_cputicks = 0;

  return p->trapframe->a0;
}

int
sys_settickets(void)
{
  int number;
  argint(0,&number);
  if(number <= 0)
    return -1;
  struct proc *p = myproc();
  p->tickets = number;
  return 0;
}

int
sys_set_priority(void)
{
  int priority,pid,oldSP = -1,oldDP=-1,scheduleAgain = 0;
  argint(0,&priority);
  argint(1,&pid);
  if(pid < 0 || priority < 0 || priority > 100)
    return -1;
  struct proc* p;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    if(p->pid == pid){
      oldSP = p->SP;
      oldDP = p->DP;
      p->SP = priority;
      p->sleepTime = 0;
      p->runTime = 0;
      p->DP = p->SP;
      if(p->DP < oldDP)
        scheduleAgain = 1;
    }
    release(&p->lock);
  }
  if(scheduleAgain)
    yield();
  return oldSP;
}

uint64
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc* p = myproc();
  if (copyout(p->pagetable, addr1,(char*)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2,(char*)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}