# READCOUNT

- Declared `int readcount` in `src/kernel/proc.h` at line 102 to be used in `proc.c`.

<br>

- Initialized `readcount = 0` in `src/kernel/proc.c` at line 129 which is a global variable that keeps track of the number of times `read` has been called.

  ```
  p->readcount = 0;
  ```

<br>

- Initialized `count_read` to `0` in `src/kernel/sysproc.c` at line 10 which is a global variable that keeps track of the number of times `read` has been called.

  ```
  int count_read = 0;
  ```

<br>

- Defined the systemcall number of `SYS_getreadcount` in `src/kernel/syscall.h` to `23` at line 24

  ```
  #define SYS_getreadcount 23
  ```

<br>

- Added the prototype of `sys_getreadcount` in `src/kernel/syscall.c` at line 103.
  ```
  extern uint64 sys_getreadcount(void);
  ```

<br>

- Added the mapping of the number of `SYS_getreadcount` to `sys_getreadcount` in `src/kernel/syscall.c` at line 132.

  ```
  [SYS_getreadcount] sys_getreadcount,
  ```

<br>

- Checked if the `num` is the same as the number of `SYS_read` and if it is then increment the `count_read` by `1` in `src/kernel/syscall.c` at line 145.

  ```
      if (num > 0 && num == SYS_read)
      {
          count_read++;
      }
  ```

<br>

- Checked if the `num` is the same as the number of `SYS_getreadcount` and if it is then change the value `readcount` of the process to the updated value of `count_read` in `src/kernel/syscall.c` at line 150.
  ```
     if (num > 0 && num == SYS_getreadcount)
     {
         p->readcount = count_read;
     }
  ```

<br>

- Added the function prototype of `getreadcount` in `src/user/user.h` at line 26.

  ```
  int getreadcount(void);
  ```

<br>

- Added the entry for `getreadcount` in `src/user/usys.pl` at line 40.

  ```
  entry("getreadcount");
  ```

<br>
<br>
<br>

---

# SIGALARM AND SIGRETURN

- Declared the following in `src/kernel/proc.h` at line numbers `105` to `109` :
  ```
  int signal_state;
  uint64 signal_handler;
  int interval;
  int num_ticks;
  struct trapframe \*alarm_trapf;
  ```

<br>

- Initialized the following in `src/kernel/proc.c` at line numbers `129` to `134` :

  ````
  p-> signal_state = 0; //This is a global variable that keeps track of the state of the signal handler.
  p-> interval = 0; //This is a global variable that keeps track of the interval of the signal handler.
  p-> num_ticks = 0; //This is a global variable that keeps track of the number of ticks of the signal handler.
  p-> signal_handler = 0; //This is a global variable that keeps track of the signal handler.
  p-> alarm_tf = 0;//This is a global variable that keeps track of the trapframe of the signal handler.

      ```

  <br>

  ````

- Defined the systemcall number of `SYS_sigalarm` and `SYS_sigreturn` to `24` and `25` respectively in `src/kernel/syscall.h` at line numbers `25` and `26` respectively.

  ```
  #define SYS_sigalarm 24
  #define SYS_sigreturn 25
  ```

<br>

- Added the prototypes of the functions `sys_sigalarm` and `sys_sigreturn` in `src/kernel/syscall.c` at line numbers `104` and `105` respectively.

  ```
  extern uint64 sys_sigalarm(void);
  extern uint64 sys_sigreturn(void);
  ```

  <br>

- Added the mappings of the numbers of `SYS_sigalarm` and `SYS_sigreturn` to `sys_sigalarm` and `sys_sigreturn` respectively in `src/kernel/syscall.c` at line numbers `133` and `134` respectively.

  ```
  [SYS_sigalarm] sys_sigalarm,
  [SYS_sigreturn] sys_sigreturn,
  ```

<br>

- Added the main code for `sys_sigalarm(void)` in `src/kernel/sysproc.c` at line numbers `19` to `40`.

  ```
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
  ```

- Added the main code for `sys_sigreturn(void)` in `src/kernel/sysproc.c` at line numbers `42` to `67`.

  ```
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
  ```

<br>

- Added the following code to handle the operatoins in `src/kernel/trap.c` at line numbers `86` to `115`

  ```
    // Ensure atomic operations to prevent race conditions.
      acquire(&p->lock);

      if (p->interval > 0 && p->signal_state == 0)
      {
        p->num_ticks++;
        if (p->num_ticks >= p->interval)
        {
          p->num_ticks = 0;
          p->signal_state = 1;

          // Allocate memory for alarm_trapf and check for allocation failure.
          p->alarm_trapf = kalloc();
          if (p->alarm_trapf == 0)
          {
            // Handle memory allocation failure, e.g., log an error or reset signal_state.
            p->signal_state = 0;
          }
          else
          {
            // Ensure memmove is safe, i.e., trapframe and alarm_trapf point to valid memory.
            memmove(p->alarm_trapf, p->trapframe, PGSIZE);

            // Ensure signal_handler points to a valid function before modifying epc.

            p->trapframe->epc = p->signal_handler;
          }
        }
      }

      release(&p->lock);

  ```

<br>

- Added the function prototypes of `sigalarm` and `sigreturn` in `src/user/user.h` at line numbers `27` and `28` respectively.

  ```
  int sigalarm(int ticks, void (*handler)());
  int sigreturn(void);
  ```

<br>

- Added the entries for `sigalarm` and `sigreturn` in `src/user/usys.pl` at line numbers `41` and `42` respectively.

  ```
  entry("sigalarm");
  entry("sigreturn");
  ```

<br>
<br>
<br>

---

# SCHEDULING

- Added the default chosen scheduler explicitly in the `MakeFile` from line `59` to `62` (due to the changes made in the code, the default scheduler was not being chosen automatically) and added the `CFLAG` corresponding to it.

  ```
  # If the scheduler is not specified, default to RR
  ifndef SCHEDULER
  SCHEDULER := RR
  endif

  ```

  ```
  CFLAGS += -D $(SCHEDULER)
  ```

<br>

- Added new variables to the file `src/kernel/proc.h` from line `111` to `116` for scheduling

  ```
  int level;          // level or queue number
  int check_interval; // check whether proc is in any queue or not
  int timeslice;      // timeslice in the current queue
  int entry_time;     // Entry time in this queue
  int run_time[4];    // run time in 4 queues
  ```

<br>


- Added new structs `Node` and `queue` and wrote the function definition of functions which will be used in `src/kernel/proc.h` from line `156` to `175` for MLFQ

  ```
  typedef struct node // Node for the queue
  {
    struct proc *curr_proc;
    struct node *next;
  } Node;

  typedef struct QUEUE // Queue for MLFQ
  {
    struct node *head;
    int curr_size;
  } queue;

  void enqueue(Node **head, struct proc *p);                              // Enqueue a process in the queue
  struct proc *front(Node **head);                                        // Return the front process in the queue
  void dequeue(Node **head);                                              // Dequeue the front process in the queue
  void delete(Node **head, uint pid);                                     // Delete a process from the queue
  Node *findAvailableNode();                                              // Find an available node in the queue
  void appendToQueue(Node *head, Node *newNode);                          // Append a node to the queue
  void initializeQueues(queue *queues, int numQueues);                    // Initialize the queues
  void initializeProcessList(Node *processList, int numProcesses); // Initialize the process list
  ```

<br>

- Made a `Queue` and list of processes `processList` in `src/kernel/proc.c` from line `10` to `12` for MLFQ

  ```
  queue queues[4]; // Queue for MLFQ
  Node processList[NPROC]; // List of processes for MLFQ
  ```

<br>

- Wrote the functions which were defined in `proc.h` in `src/kernel/proc.c` from line `91` to `202` (not writing them here to avoid cluttering)

<br>

- Initialized the variables which were decleared in `proc.h` in `src/kernel/proc.c` from line `288` to `296` for MLFQ

  ```
  p->level = 0;           // to keep track of the level of the process
  p->timeslice = 0;       // to keep track of the timeslice of the process
  p->entry_time = ticks;  // to keep track of the entry time of the process
  p->check_interval = 0;  // to keep track of the interval of the process

  for (int i = 0; i <= 3; i++)
  {
    p->run_time[i] = 0;
  }
  ```

<br>


- Wrote the codes for how to schedule the processes in `src/kernel/proc.c` for FCFS & MLFQ (RR is the by default method in xv6) from lines `635` to `753`

<br>

- Added the code to change the levels for MLFQ in `src/kernel/trap.c` from lines `120` to `142` and `217` to `227`

<br>

- Added a small change in `src/user/schedulertest.c` for FCFS so that it doesn't quit after intervals


<br>
<br>
<br>

