#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"

struct {
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void trapret(void);

int
cpuid() {
  return 0;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  return &cpus[0];
}

// Read proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c = mycpu();
  return c->proc;
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  if((p->offset = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  p->sz = PGSIZE - KSTACKSIZE;

  sp = (char*)(p->offset + PGSIZE);

  // Allocate kernel stack.
  p->kstack = sp - KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)trapret;

  return p;
}

// Set up first process.
void
pinit(int pol)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  p->policy = pol;
  initproc = p;

  memmove(p->offset, _binary_initcode_start, (int)_binary_initcode_size);
  memset(p->tf, 0, sizeof(*p->tf));

  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;

  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE - KSTACKSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  p->state = RUNNABLE;
}

// process scheduler.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void) {
  struct cpu *c = mycpu();
  c->proc = 0;
  int run_counter = 0;
  int wrapper = 0;
  struct proc* fg_index = ptable.proc;
  struct proc* bg_index = ptable.proc;

  for (;;) {
    run_counter = (run_counter + 1) % 10;
    sti(); // Enable interrupts on this processor.
    wrapper = 0;
    if (run_counter % 10 < 9) {
      // Background processes get CPU time for 10% of the time

      while(wrapper != NPROC && (fg_index->state != RUNNABLE || fg_index->policy != 0)){

        fg_index = (fg_index == &ptable.proc[NPROC-1]) ? ptable.proc : (fg_index +1) ;

        wrapper++;
      }


      if(wrapper == NPROC){
        // run_counter = (run_counter + 1 ) % 10;
        fg_index = (fg_index == &ptable.proc[NPROC-1]) ? ptable.proc : (fg_index +1) ; 
        continue;     
      }


        c->proc = fg_index;
        fg_index->state = RUNNING;

        switchuvm(fg_index);
        swtch(&(c->scheduler), fg_index->context);

        // Update process state after switching.
        c->proc->state = RUNNABLE;

        fg_index = (fg_index == &ptable.proc[NPROC-1]) ? ptable.proc : (fg_index +1) ;


      // }
    } else {
      // Foreground processes get CPU time for 90% of the time
      while(wrapper != NPROC && (bg_index->state != RUNNABLE || bg_index->policy != 1)){
bg_index = (bg_index == &ptable.proc[NPROC-1]) ? ptable.proc : (bg_index +1) ;
        wrapper++;
      }


      if(wrapper == NPROC){
        // run_counter = (run_counter + 1 ) % 10;
bg_index = (bg_index == &ptable.proc[NPROC-1]) ? ptable.proc : (bg_index +1) ;  
        continue;     
      }


        c->proc = bg_index;
        bg_index->state = RUNNING;

        switchuvm(bg_index);
        swtch(&(c->scheduler), bg_index->context);

        // Update process state after switching.
        c->proc->state = RUNNABLE;

        bg_index = (bg_index == &ptable.proc[NPROC-1]) ? ptable.proc : (bg_index +1) ;
      // }
    }

    
  }
}
// void
// scheduler(void)
// {
//   struct proc *p;
//   struct cpu *c = mycpu();
//   c->proc = 0;
//   int bg_cnt = 1;
//   int fg_cnt = 9;
//   int fg_p =0;
//   int bg_p =0 ;
  
//   for(;;){
//     // Enable interrupts on this processor.
//     sti();
    
//     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//       if (p->state != RUNNABLE){
//         continue;
//       }
//       if (p->policy==0) {

//         fg_p ++;
        
//       } 
//       else if (p->policy==1){

//         bg_p ++;
//       }

//     }

//     // Loop over process table looking for process to run.
//     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

//       if(p->state != RUNNABLE)
//         continue;
//       if (fg_p ==0 || bg_p ==0){
//         c->proc =p;
//         p->state =RUNNING ; 
//         switchuvm(p);
//         swtch(&(c->scheduler), p->context);     

//       }
      
//       else if (p->policy == 0 && fg_cnt>0){
//         // Switch to chosen process.
//         c->proc = p;
//       p->state = RUNNING;
//       fg_cnt --;
//       switchuvm(p);
//       swtch(&(c->scheduler), p->context);

//       }
      
//       else if (p-> policy ==1 && bg_cnt>0){
//         c->proc =p ;
//         p->state =RUNNING ;
//         bg_cnt --;
//         switchuvm(p);
//       swtch(&(c->scheduler), p->context);
//       }
       
//       else if (bg_cnt + fg_cnt ==0){
//         fg_cnt =9;
//         bg_cnt = 1 ;
//       }
      
//       else if (p->policy ==1 && bg_cnt==0) {
//         continue;
//       }
//       else if (p->policy == 0 && fg_cnt == 0){
//         continue ;
//       }

      
//     }
//   }
// }
// void
// scheduler(void)
// {
//   struct proc *p;
//   struct cpu *c = mycpu();
//   c->proc = 0;
//   int fg_p =0;
//   int bg_p =0;
//   int cnt =0 ;
//   struct proc *bl = ptable.proc;
//   struct proc *fl =ptable.proc;
//   struct proc *end ;
//   for (;;){
//     sti();
//     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//       if (p->state != RUNNABLE){
//         continue;
//       }
//       if (p->policy==0) {

//         fg_p ++;
        
//       } 
//       else if (p->policy==1){

//         bg_p ++;
//       }

//     }
//     if (bg_p==0 || fg_p == 0) {
//       for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//       if (p->state != RUNNABLE){
//         continue;
//       }
//       else {
//         c->proc = p;
//         p->state = RUNNING;
//         switchuvm(p);
//         swtch(&(c->scheduler), p->context);


//       }}
     

//     }
//     else {p =bl ;
//     if (bl == ptable.proc ){
//       end = &ptable.proc[NPROC-1];
//     }
//     else {
//       end = bl-1;
//     }
//     while (cnt ==0 && p!= end){
//       if (p->state != RUNNABLE){
//         p++;
//         continue;
//         }
//       else if(p->policy == 1){
//         c->proc =p;
//         p->state =RUNNING ; 
//         cnt = (cnt+1)%10;
//         if (p== &ptable.proc[NPROC -1]){
//           bl= ptable.proc;
//         }
//         else{bl = p+1;}
        
//         p++;
//         switchuvm(p);
//         swtch(&(c->scheduler), p->context);


//       }
//       else {
//         p++;

//       }  

//   }
//   p =fl ;
//     if (fl == ptable.proc ){
//       end = &ptable.proc[NPROC-1];
//     }
//     else {
//       end = fl-1;
//     }

//   while (cnt!=0 && p!= end){
//       if (p->state != RUNNABLE){
//         p++;
//         continue;}
//       else if(p->policy == 0){
//         c->proc =p;
//         p->state =RUNNING ; 
//         cnt = (cnt+1)%10;
//         if (p== &ptable.proc[NPROC -1]){
//           bl= ptable.proc;
//         }
//         else{bl = p+1;}
//         p++;
//         switchuvm(p);
//         swtch(&(c->scheduler), p->context);


//       }
//       else {
//         p++;

//       }  

//   }}
    

// }}
//   int bg_cnt = 1;
//   int fg_cnt = 9;
//   int fg_p =0;
//   int bg_p =0 ;
//   // struct proc *lp , *fp ;
 

//   for(;;){
//     // Enable interrupts on this processor.
//     sti();
  
//     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
//       if (p->state != RUNNABLE){
//         continue;
//       }
//       if (p->policy==0) {

//         fg_p ++;
        
//       } 
//       else if (p->policy==1){

//         bg_p ++;
//       }

//     }
   
//     // Loop over process table looking for process to run.
//     if (fg_p ==0 || bg_p ==0){
//         c->proc =p;
//         p->state =RUNNING ; 
//         switchuvm(p);
//         swtch(&(c->scheduler), p->context);     

//       }
      
    
//     for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){

//       if(p->state != RUNNABLE)
//         continue;
      
      
//       else if (p->policy == 0 && fg_cnt>0){
//         // Switch to chosen process.
//         c->proc = p;
//       p->state = RUNNING;
//       fg_cnt --;
//       switchuvm(p);
//       swtch(&(c->scheduler), p->context);

//       }
      
//       else if (p-> policy ==1 && bg_cnt>0){
//         c->proc =p ;
//         p->state =RUNNING ;
//         bg_cnt --;
//         switchuvm(p);
//       swtch(&(c->scheduler), p->context);
//       }
       
//       else if (bg_cnt + fg_cnt ==0){
//         fg_cnt =9;
//         bg_cnt = 1 ;
//       }
      
//       // else if (p->policy ==1 && bg_cnt==0) {
//       //   continue;
//       // }
//       // else if (p->policy == 0 && fg_cnt == 0){
//       //   continue ;
//       // }

      
//     }
//   }
// }

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct cpu* c = mycpu();
  struct proc *p = myproc();

  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = c->intena;
  swtch(&p->context, c->scheduler);
  c->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  myproc()->state = RUNNABLE;
  sched();
}

void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  };
  struct proc *p;
  char *state;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    cprintf("\n");
  }
}