#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(write)
SYSCALL(close)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(get_sched_policy)
SYSCALL(set_sched_policy)