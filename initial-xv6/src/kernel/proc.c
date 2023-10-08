#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include <stddef.h>

// Declaration of the queues for the processes for FCFS
queue Queue[4];            // 4 queues for MLFQ
Node processesList[NPROC]; // List of processes

// #ifdef MLFQ
// // Initialize queues
// void initializeQueues(Queue *queues, int numQueues)
// {
//   for (int i = 0; i < numQueues; i++)
//   {
//     queues[i].head = NULL;
//     queues[i].curr_size = 0;
//   }
// }

// // Initialize process list
// void initializeProcessList(ProcessList *processList, int numProcesses)
// {
//   for (int i = 0; i < numProcesses; i++)
//   {
//     processList[i].curr_proc = NULL;
//     processList[i].next = NULL;
//   }
// }

// initializeQueues(Queue, 4);
// initializeProcessList(processesList, NPROC);
// #endif

struct cpu cpus[NCPU];

struct proc proc[NPROC];

struct proc *initproc;

int nextpid = 1;
struct spinlock pid_lock;

extern void forkret(void);
static void freeproc(struct proc *p);

extern char trampoline[]; // trampoline.S

// helps ensure that wakeups of wait()ing
// parents are not lost. helps obey the
// memory model when using p->parent.
// must be acquired before any p->lock.
struct spinlock wait_lock;

// Allocate a page for each process's kernel stack.
// Map it high in memory, followed by an invalid
// guard page.
void proc_mapstacks(pagetable_t kpgtbl)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    char *pa = kalloc();
    if (pa == 0)
      panic("kalloc");
    uint64 va = KSTACK((int)(p - proc));
    kvmmap(kpgtbl, va, (uint64)pa, PGSIZE, PTE_R | PTE_W);
  }
}

// initialize the proc table.
void procinit(void)
{
  struct proc *p;

  initlock(&pid_lock, "nextpid");
  initlock(&wait_lock, "wait_lock");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    initlock(&p->lock, "proc");
    p->state = UNUSED;
    p->kstack = KSTACK((int)(p - proc));
  }
  // Setting up QUEUES for scheduler

#ifdef MLFQ
  for (int i = 0; i < 4; i++)
  {
    Queue[i].head = 0;
    Queue[i].curr_size = 0;
  }
  for (int i = 0; i < NPROC; i++)
  {
    processesList[i].curr_proc = 0;
    processesList[i].next = 0;
  }
#endif
}

/*
  MLFQ HELPER FUNCTIONS START
*/

// Enqueue a process to the queue
void enqueue(Node **head, struct proc *p)
{
  Node *newNode = findAvailableNode(); // Find an available node
  newNode->curr_proc = p;
  newNode->next = NULL;

  if (*head == NULL) // If the queue is empty, new node is the head
  {
    *head = newNode;
  }
  else // Otherwise, append to the end of the queue
  {
    appendToQueue(*head, newNode);
  }
}

// Remove the front process from the queue
void dequeue(Node **head)
{
  if (*head == NULL)
  {
    return;
  }
  Node *temp = *head;
  *head = (*head)->next;
  temp->curr_proc = NULL;
  temp->next = NULL;
}

// Find an available node in the process list
Node *findAvailableNode()
{
  for (int i = 0; i < NPROC; i++)
  {
    if (processesList[i].curr_proc == NULL)
    {
      return &(processesList[i]);
    }
  }
  return NULL; // No available node found
}

// Append a node to the end of the queue
void appendToQueue(Node *head, Node *newNode)
{
  Node *current = head;
  while (current->next != NULL)
  {
    current = current->next;
  }
  current->next = newNode;
}

// Get the front process of the queue without removing it
struct proc *front(Node **head)
{
  if (*head == NULL)
  {
    return NULL;
  }
  return (*head)->curr_proc; // Return the front process
}

// Delete a process with a specific PID from the queue
void delete(Node **head, uint pid)
{
  if (*head == NULL)
  {
    return;
  }
  if ((*head)->curr_proc->pid == pid)
  {
    Node *temp = *head;
    *head = (*head)->next;
    temp->curr_proc = NULL;
    temp->next = NULL;
    return;
  }

  Node *current = *head;
  while (current->next != NULL && current->next->curr_proc->pid != pid)
  {
    current = current->next;
  }

  if (current->next != NULL) // PID found
  {
    Node *temp = current->next;
    current->next = current->next->next;
    temp->curr_proc = NULL;
    temp->next = NULL;
  }
}

/*
  END OF HELPER FUNCTIONS FOR MLFQ
*/

// Must be called with interrupts disabled,
// to prevent race with process being moved
// to a different CPU.
int cpuid()
{
  int id = r_tp();
  return id;
}

// Return this CPU's cpu struct.
// Interrupts must be disabled.
struct cpu *
mycpu(void)
{
  int id = cpuid();
  struct cpu *c = &cpus[id];
  return c;
}

// Return the current struct proc *, or zero if none.
struct proc *
myproc(void)
{
  push_off();
  struct cpu *c = mycpu();
  struct proc *p = c->proc;
  pop_off();
  return p;
}

int allocpid()
{
  int pid;

  acquire(&pid_lock);
  pid = nextpid;
  nextpid = nextpid + 1;
  release(&pid_lock);

  return pid;
}

// Look in the process table for an UNUSED proc.
// If found, initialize state required to run in the kernel,
// and return with p->lock held.
// If there are no free procs, or a memory allocation fails, return 0.
static struct proc *
allocproc(void)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == UNUSED)
    {
      goto found;
    }
    else
    {
      release(&p->lock);
    }
  }
  return 0;

found:
  p->pid = allocpid();
  p->state = USED;

  // for getreadcount
  p->readcount = 0; // added // to keep track of the number of read system calls

  // for sigalarm and sigreturn
  p->signal_state = 0;   // added   //  to keep track of the state of the signal
  p->signal_handler = 0; // added   //  to keep track of the signal handler
  p->interval = 0;       // added   // to keep track of the interval
  p->num_ticks = 0;      // added   // to keep track of the number of ticks
  p->alarm_trapf = 0;    // added   // to keep track of the trapframe

  // for scheduling
  p->level = 0;          // added // to keep track of the level of the process
  p->timeslice = 0;      // added // to keep track of the timeslice of the process
  p->entry_time = ticks; // added // to keep track of the entry time of the process
  p->check_interval = 0; // added // to keep track of the interval of the process

  for (int i = 0; i <= 3; i++)
  {
    p->run_time[i] = 0;
  }

  // Allocate a trapframe page.
  if ((p->trapframe = (struct trapframe *)kalloc()) == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // An empty user page table.
  p->pagetable = proc_pagetable(p);
  if (p->pagetable == 0)
  {
    freeproc(p);
    release(&p->lock);
    return 0;
  }

  // Set up new context to start executing at forkret,
  // which returns to user space.
  memset(&p->context, 0, sizeof(p->context));
  p->context.ra = (uint64)forkret;
  p->context.sp = p->kstack + PGSIZE;
  p->rtime = 0;
  p->etime = 0;
  p->ctime = ticks;
  return p;
}

// free a proc structure and the data hanging from it,
// including user pages.
// p->lock must be held.
static void
freeproc(struct proc *p)
{
  if (p->trapframe)
    kfree((void *)p->trapframe);
  p->trapframe = 0;
  if (p->pagetable)
    proc_freepagetable(p->pagetable, p->sz);
  p->pagetable = 0;
  p->sz = 0;
  p->pid = 0;
  p->parent = 0;
  p->name[0] = 0;
  p->chan = 0;
  p->killed = 0;
  p->xstate = 0;
  p->state = UNUSED;
}

// Create a user page table for a given process, with no user memory,
// but with trampoline and trapframe pages.
pagetable_t
proc_pagetable(struct proc *p)
{
  pagetable_t pagetable;

  // An empty page table.
  pagetable = uvmcreate();
  if (pagetable == 0)
    return 0;

  // map the trampoline code (for system call return)
  // at the highest user virtual address.
  // only the supervisor uses it, on the way
  // to/from user space, so not PTE_U.
  if (mappages(pagetable, TRAMPOLINE, PGSIZE,
               (uint64)trampoline, PTE_R | PTE_X) < 0)
  {
    uvmfree(pagetable, 0);
    return 0;
  }

  // map the trapframe page just below the trampoline page, for
  // trampoline.S.
  if (mappages(pagetable, TRAPFRAME, PGSIZE,
               (uint64)(p->trapframe), PTE_R | PTE_W) < 0)
  {
    uvmunmap(pagetable, TRAMPOLINE, 1, 0);
    uvmfree(pagetable, 0);
    return 0;
  }

  return pagetable;
}

// Free a process's page table, and free the
// physical memory it refers to.
void proc_freepagetable(pagetable_t pagetable, uint64 sz)
{
  uvmunmap(pagetable, TRAMPOLINE, 1, 0);
  uvmunmap(pagetable, TRAPFRAME, 1, 0);
  uvmfree(pagetable, sz);
}

// a user program that calls exec("/init")
// assembled from ../user/initcode.S
// od -t xC ../user/initcode
uchar initcode[] = {
    0x17, 0x05, 0x00, 0x00, 0x13, 0x05, 0x45, 0x02,
    0x97, 0x05, 0x00, 0x00, 0x93, 0x85, 0x35, 0x02,
    0x93, 0x08, 0x70, 0x00, 0x73, 0x00, 0x00, 0x00,
    0x93, 0x08, 0x20, 0x00, 0x73, 0x00, 0x00, 0x00,
    0xef, 0xf0, 0x9f, 0xff, 0x2f, 0x69, 0x6e, 0x69,
    0x74, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00};

// Set up first user process.
void userinit(void)
{
  struct proc *p;

  p = allocproc();
  initproc = p;

  // allocate one user page and copy initcode's instructions
  // and data into it.
  uvmfirst(p->pagetable, initcode, sizeof(initcode));
  p->sz = PGSIZE;

  // prepare for the very first "return" from kernel to user.
  p->trapframe->epc = 0;     // user program counter
  p->trapframe->sp = PGSIZE; // user stack pointer

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;

  release(&p->lock);
}

// Grow or shrink user memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n)
{
  uint64 sz;
  struct proc *p = myproc();

  sz = p->sz;
  if (n > 0)
  {
    if ((sz = uvmalloc(p->pagetable, sz, sz + n, PTE_W)) == 0)
    {
      return -1;
    }
  }
  else if (n < 0)
  {
    sz = uvmdealloc(p->pagetable, sz, sz + n);
  }
  p->sz = sz;
  return 0;
}

// Create a new process, copying the parent.
// Sets up child kernel stack to return as if from fork() system call.
int fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *p = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
  {
    return -1;
  }

  // Copy user memory from parent to child.
  if (uvmcopy(p->pagetable, np->pagetable, p->sz) < 0)
  {
    freeproc(np);
    release(&np->lock);
    return -1;
  }
  np->sz = p->sz;

  // copy saved user registers.
  *(np->trapframe) = *(p->trapframe);

  // Cause fork to return 0 in the child.
  np->trapframe->a0 = 0;

  // increment reference counts on open file descriptors.
  for (i = 0; i < NOFILE; i++)
    if (p->ofile[i])
      np->ofile[i] = filedup(p->ofile[i]);
  np->cwd = idup(p->cwd);

  safestrcpy(np->name, p->name, sizeof(p->name));

  pid = np->pid;

  release(&np->lock);

  acquire(&wait_lock);
  np->parent = p;
  release(&wait_lock);

  acquire(&np->lock);
  np->state = RUNNABLE;
  release(&np->lock);

  return pid;
}

// Pass p's abandoned children to init.
// Caller must hold wait_lock.
void reparent(struct proc *p)
{
  struct proc *pp;

  for (pp = proc; pp < &proc[NPROC]; pp++)
  {
    if (pp->parent == p)
    {
      pp->parent = initproc;
      wakeup(initproc);
    }
  }
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait().
void exit(int status)
{
  struct proc *p = myproc();

  if (p == initproc)
    panic("init exiting");

  // Close all open files.
  for (int fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd])
    {
      struct file *f = p->ofile[fd];
      fileclose(f);
      p->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(p->cwd);
  end_op();
  p->cwd = 0;

  acquire(&wait_lock);

  // Give any children to init.
  reparent(p);

  // Parent might be sleeping in wait().
  wakeup(p->parent);

  acquire(&p->lock);

  p->xstate = status;
  p->state = ZOMBIE;
  p->etime = ticks;

  release(&wait_lock);

  // Jump into the scheduler, never to return.
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(uint64 addr)
{
  struct proc *pp;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (pp = proc; pp < &proc[NPROC]; pp++)
    {
      if (pp->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&pp->lock);

        havekids = 1;
        if (pp->state == ZOMBIE)
        {
          // Found one.
          pid = pp->pid;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&pp->xstate,
                                   sizeof(pp->xstate)) < 0)
          {
            release(&pp->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(pp);
          release(&pp->lock);
          release(&wait_lock);
          return pid;
        }
        release(&pp->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || killed(p))
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run.
//  - swtch to start running that process.
//  - eventually that process transfers control
//    via swtch back to the scheduler.
void scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();

  c->proc = 0;
#ifdef FCFS
  printf("FCFS has been chosen as the scheduler for the OS\n");
  for (;;)
  {
    struct proc *min_time_proc = proc;
    intr_on(); // Enable interrupts to avoid deadlock

    int min_creationTime = __INT32_MAX__;
    // Find the process with the minimum creation time that is runnable
    for (p = proc; p < &proc[NPROC]; p++)
    {
      if ((p->state == RUNNABLE) && (p->ctime < min_creationTime)) // If the process is runnable and has a lower creation time
      {
        min_time_proc = p;
        min_creationTime = p->ctime;
      }
    }
    // min_time_proc contains the process with the minimum creation time that is runnable
    // min contains the minimum creation time

    acquire(&min_time_proc->lock);
    p = min_time_proc;
    if (p->state == RUNNABLE)
    {
      p->state = RUNNING;              // Change the state of the process to running
      c->proc = p;                     // Set the current process to the process with the minimum creation time
      swtch(&c->context, &p->context); // Switch to the process with the minimum creation time

      // Set the current process to 0 since the process has finished running therefore it is not the current process anymore
      c->proc = 0;
    }
    release(&p->lock);
  }
#elif defined(MLFQ)
  printf("MLFQ has been chosen as the scheduler for the OS\n");
  for (;;)
  {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on(); // Enable interrupts to avoid deadlock

    // Aging of the process
    for (p = proc; p < &proc[NPROC]; p++) // Iterate through all processes
    {
      if (p->state == RUNNABLE && (ticks - (p->entry_time)) >= 128) // If the process is runnable and has been in the queue for more than 128 ticks
      {
        if (p->check_interval == 1) // If the process is in the queue
        {
          delete (&(Queue[p->level].head), p->pid); // Delete the process from the queue
          Queue[p->level].curr_size--;              // Decrement the size of the queue
          p->check_interval = 0;                    // Set the check interval to 0
        }
        if (p->level) // If the process is not in the highest queue
        {
          p->level--; // Move the process to the higher queue
        }
        p->entry_time = ticks; // Update the entry time of the process
      }
    }

    // Enqueue runnable processes
    for (p = proc; p < &proc[NPROC]; p++) //  Iterate through all processes again to find runnable processes
    {
      if (p->state == RUNNABLE && p->check_interval == 0)
      {
        enqueue(&(Queue[p->level].head), p); // Enqueue the process to the queue
        Queue[p->level].curr_size++;         // Increment the size of the queue
        p->check_interval = 1;               // Set the check interval to 1
      }
    }

    // Find the next process to run
    int flag = 0;                           // Flag to check if a process has been found
    for (int q_lev = 0; q_lev < 4; q_lev++) // Iterate through all queues
    {
      while (Queue[q_lev].curr_size)
      {
        p = front(&(Queue[q_lev].head)); // Get the front process in the queue
        dequeue(&(Queue[q_lev].head));   // Dequeue the front process in the queue
        Queue[q_lev].curr_size--;        // Decrement the size of the queue
        p->check_interval = 0;           // Set the check interval to 0
        if (p->state == RUNNABLE)
        {
          p->entry_time = ticks; // Update the entry time of the process
          flag = 1;              // Set the flag to 1
          break;
        }
      }
      // if a process has been found, break out of the loop
      if (flag == 1)
      {
        break;
      }
    }

    if (p == 0) //  If no process has been found
    {
      continue;
    }

    if (p->state == RUNNABLE)
    {
      acquire(&p->lock);
      // Assign time slice based on process level
      int timeslices[] = {1, 3, 9, 15};
      p->timeslice = timeslices[p->level];

      p->state = RUNNING;
      c->proc = p;
      p->entry_time = ticks;
      swtch(&c->context, &p->context);
      c->proc = 0;
      release(&p->lock);
    }
  }

#elif defined(RR)
  // was already given
  printf("RR has been chosen as the scheduler for the process\n");

  for (;;)
  {
    // Avoid deadlock by ensuring that devices can interrupt.
    intr_on();
    for (p = proc; p < &proc[NPROC]; p++)
    {
      acquire(&p->lock);
      if (p->state == RUNNABLE)
      {
        // Switch to chosen process.  It is the process's job
        // to release its lock and then reacquire it
        // before jumping back to us.
        p->state = RUNNING;
        c->proc = p;
        swtch(&c->context, &p->context);

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
      }
      release(&p->lock);
    }
  }
#endif
}

// Switch to scheduler.  Must hold only p->lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->noff, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void)
{
  int intena;
  struct proc *p = myproc();

  if (!holding(&p->lock))
    panic("sched p->lock");
  if (mycpu()->noff != 1)
    panic("sched locks");
  if (p->state == RUNNING)
    panic("sched running");
  if (intr_get())
    panic("sched interruptible");

  intena = mycpu()->intena;
  swtch(&p->context, &mycpu()->context);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void)
{
  struct proc *p = myproc();
  acquire(&p->lock);
  p->state = RUNNABLE;
  sched();
  release(&p->lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch to forkret.
void forkret(void)
{
  static int first = 1;

  // Still holding p->lock from scheduler.
  release(&myproc()->lock);

  if (first)
  {
    // File system initialization must be run in the context of a
    // regular process (e.g., because it calls sleep), and thus cannot
    // be run from main().
    first = 0;
    fsinit(ROOTDEV);
  }

  usertrapret();
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  // Must acquire p->lock in order to
  // change p->state and then call sched.
  // Once we hold p->lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup locks p->lock),
  // so it's okay to release lk.

  acquire(&p->lock); // DOC: sleeplock1
  release(lk);

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  release(&p->lock);
  acquire(lk);
}

// Wake up all processes sleeping on chan.
// Must be called without any p->lock.
void wakeup(void *chan)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p != myproc())
    {
      acquire(&p->lock);
      if (p->state == SLEEPING && p->chan == chan)
      {
        p->state = RUNNABLE;
      }
      release(&p->lock);
    }
  }
}

// Kill the process with the given pid.
// The victim won't exit until it tries to return
// to user space (see usertrap() in trap.c).
int kill(int pid)
{
  struct proc *p;

  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->pid == pid)
    {
      p->killed = 1;
      if (p->state == SLEEPING)
      {
        // Wake process from sleep().
        p->state = RUNNABLE;
      }
      release(&p->lock);
      return 0;
    }
    release(&p->lock);
  }
  return -1;
}

void setkilled(struct proc *p)
{
  acquire(&p->lock);
  p->killed = 1;
  release(&p->lock);
}

int killed(struct proc *p)
{
  int k;

  acquire(&p->lock);
  k = p->killed;
  release(&p->lock);
  return k;
}

// Copy to either a user address, or kernel address,
// depending on usr_dst.
// Returns 0 on success, -1 on error.
int either_copyout(int user_dst, uint64 dst, void *src, uint64 len)
{
  struct proc *p = myproc();
  if (user_dst)
  {
    return copyout(p->pagetable, dst, src, len);
  }
  else
  {
    memmove((char *)dst, src, len);
    return 0;
  }
}

// Copy from either a user address, or kernel address,
// depending on usr_src.
// Returns 0 on success, -1 on error.
int either_copyin(void *dst, int user_src, uint64 src, uint64 len)
{
  struct proc *p = myproc();
  if (user_src)
  {
    return copyin(p->pagetable, dst, src, len);
  }
  else
  {
    memmove(dst, (char *)src, len);
    return 0;
  }
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void)
{
  static char *states[] = {
      [UNUSED] "unused",
      [USED] "used",
      [SLEEPING] "sleep ",
      [RUNNABLE] "runble",
      [RUNNING] "run   ",
      [ZOMBIE] "zombie"};
  struct proc *p;
  char *state;

  printf("\n");
  for (p = proc; p < &proc[NPROC]; p++)
  {
    if (p->state == UNUSED)
      continue;
    if (p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    printf("%d %s %s %d", p->pid, state, p->name, p->level); // printing the level which the process is running in MLFQ
    printf("\n");
  }
}

// waitx
int waitx(uint64 addr, uint *wtime, uint *rtime)
{
  struct proc *np;
  int havekids, pid;
  struct proc *p = myproc();

  acquire(&wait_lock);

  for (;;)
  {
    // Scan through table looking for exited children.
    havekids = 0;
    for (np = proc; np < &proc[NPROC]; np++)
    {
      if (np->parent == p)
      {
        // make sure the child isn't still in exit() or swtch().
        acquire(&np->lock);

        havekids = 1;
        if (np->state == ZOMBIE)
        {
          // Found one.
          pid = np->pid;
          *rtime = np->rtime;
          *wtime = np->etime - np->ctime - np->rtime;
          if (addr != 0 && copyout(p->pagetable, addr, (char *)&np->xstate,
                                   sizeof(np->xstate)) < 0)
          {
            release(&np->lock);
            release(&wait_lock);
            return -1;
          }
          freeproc(np);
          release(&np->lock);
          release(&wait_lock);
          return pid;
        }
        release(&np->lock);
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || p->killed)
    {
      release(&wait_lock);
      return -1;
    }

    // Wait for a child to exit.
    sleep(p, &wait_lock); // DOC: wait-sleep
  }
}

void update_time()
{
  struct proc *p;
  for (p = proc; p < &proc[NPROC]; p++)
  {
    acquire(&p->lock);
    if (p->state == RUNNING)
    {
      p->rtime++;
/*
ADDED CODE FOR UPDATING TIME FOR MLFQ
*/
#ifdef MLFQ
      p->run_time[p->level]++;
      p->timeslice--;
#endif
    }
    release(&p->lock);
  }
}