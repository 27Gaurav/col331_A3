#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

/* System Call Definitions */
int 
sys_set_sched_policy(void)
{
    int t ;
 
    if (argint(0, &t) < 0)
        return -22;

    if (t != 0 && t != 1)
        return -22; 

    myproc()->policy= t;
    return 0;
}

int 
sys_get_sched_policy(void)
{
    // Implement your code here 
    return myproc()-> policy;
    
}
