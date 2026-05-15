#include "pstat.h"


#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "vm.h"
#include "sem.h"


extern struct proc proc[NPROC];

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  kexit(n);
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
  return kfork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return kwait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int t;
  int n;

  argint(0, &n);
  argint(1, &t);
  addr = myproc()->sz;

  if(t == SBRK_EAGER || n < 0) {
    if(growproc(n) < 0) {
      return -1;
    }
  } else {
    // Lazily allocate memory for this process: increase its memory
    // size but don't allocate memory. If the processes uses the
    // memory, vmfault() will allocate it.
    if(addr + n < addr)
      return -1;
    if(addr + n > TRAPFRAME)
      return -1;
    myproc()->sz += n;
  }
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if(n < 0)
    n = 0;
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
  return kkill(pid);
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
sys_getprocs(void)
{
  extern struct proc proc[];
  int count = 0;
  struct proc *p;
  for(p = proc; p < &proc[NPROC]; p++) {
    acquire(&p->lock);
    // Change the line below for extra credit:
    if(p->state == RUNNABLE || p->state == RUNNING) {
      count++;
    }
    release(&p->lock);
  }
  return count;
}

uint64 sys_sem_init(void) {
  int id, val;
  argint(0, &id);
  argint(1, &val);
  sem_init(id, val);
  return 0;
}

uint64 sys_sem_wait(void) {
  int id;
  argint(0, &id);
  sem_wait(id);
  return 0;
}

uint64 sys_sem_signal(void) {
  int id;
  argint(0, &id);
  sem_signal(id);
  return 0;
}
uint64
sys_settickets(void) {
  int n;
  argint(0, &n); // Get the number of tickets from the user
  if(n < 1) return -1;
  myproc()->tickets = n;
  return 0;
}

uint64
sys_getpinfo(void) {
  uint64 addr;
  struct pstat ps;
  struct proc *p;
  argaddr(0, &addr); // Get the address of the pstat struct
  if(addr == 0) return -1;

  int i = 0;
  for(p = proc; p < &proc[NPROC]; p++){
    acquire(&p->lock);
    ps.inuse[i] = (p->state != UNUSED);
    ps.pid[i] = p->pid;
    ps.tickets[i] = p->tickets;
    ps.ticks[i] = p->ticks;
    release(&p->lock);
    i++;
  }
  // Copy the data from kernel memory to user memory
  if(copyout(myproc()->pagetable, addr, (char *)&ps, sizeof(ps)) < 0)
    return -1;
  return 0;
}
