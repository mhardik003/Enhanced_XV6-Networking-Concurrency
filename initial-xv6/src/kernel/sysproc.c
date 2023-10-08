#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

/*
  MANUAL CODE WRITTEN FOR ASSIGNMENT
*/

uint64
sys_getreadcount(void)
{
  return myproc()->readcount;
}

uint64
sys_sigalarm(void)
{
  int interval;
  uint64 handler;

  // Get the interval and handler address from the user space.
  argint(0, &interval);
  argaddr(1, &handler);

  struct proc *p = myproc(); // Get the current process

  // Ensure atomic operations to prevent race conditions.
  acquire(&p->lock);

  p->interval = interval;      // Set the interval for the current process
  p->signal_handler = handler; // Set the signal handler for the current process

  release(&p->lock);

  return 0;
}

uint64
sys_sigreturn(void)
{
  struct proc *p = myproc(); // Get the current process

  // Ensure atomic operations to prevent race conditions.
  acquire(&p->lock);

  // Check if alarm_trapf is not NULL before performing operations.
  if (p->alarm_trapf != 0)
  {
    memmove(p->trapframe, p->alarm_trapf, PGSIZE); // Restore the trapframe of the current process
    kfree(p->alarm_trapf);                         // Free the memory allocated for the alarm_trapf
    p->alarm_trapf = 0;                            // Reset the alarm_trapf
  }

  p->num_ticks = 0;    // Reset the number of ticks
  p->signal_state = 0; // Reset the signal state

  release(&p->lock);

  usertrapret(); // Return to user space

  return 0;
}

/*
  MANUAL CODE ENDS
*/

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
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
  if (growproc(n) < 0)
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
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
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
sys_waitx(void)
{
  uint64 addr, addr1, addr2;
  uint wtime, rtime;
  argaddr(0, &addr);
  argaddr(1, &addr1); // user virtual memory
  argaddr(2, &addr2);
  int ret = waitx(addr, &wtime, &rtime);
  struct proc *p = myproc();
  if (copyout(p->pagetable, addr1, (char *)&wtime, sizeof(int)) < 0)
    return -1;
  if (copyout(p->pagetable, addr2, (char *)&rtime, sizeof(int)) < 0)
    return -1;
  return ret;
}
