# Interview prep, Phases 2 to 6

Phase 1 questions are at the end of [DESIGN.md](DESIGN.md).

## Phase 2: core engine (Task, loader, clock, simulator, FCFS)

1. **Why does the Task constructor throw instead of letting bad values through?**
   It enforces the class invariant: any `Task` that exists is valid, so no
   other class has to re-check burst > 0 and similar rules. A bad CSV row fails
   early, with a line number.
2. **Who owns a Job, and why can schedulers keep raw `Job*`?**
   `Simulator::run` owns every job in a `vector<unique_ptr<Job>>`. The heap
   allocation never moves, so pointers in the ready queues stay valid for the
   whole run. The schedulers only observe the jobs, so a non-owning pointer is right.
3. **Why an event queue instead of checking every task on every tick?**
   Releases are kept in a min-heap ordered by time, so finding the next one is
   O(log T) instead of O(T) per tick. A real RTOS does the same with its
   delayed-task list (FreeRTOS `pxDelayedTaskList`, sorted by wake time).
4. **Is FCFS suitable for an RTOS?** No. It is non-preemptive, so a long job
   blocks an urgent one (the convoy effect). Response time is unbounded.

## Phase 3: Round Robin, Priority and the data structures

1. **Walk me through push and pop on your heap.** Push appends at the end and
   sifts up while the node beats its parent. Pop moves the last element to the
   root and sifts down, swapping it with its better child. Both are O(log n)
   because the tree is complete, so its height is log n.
2. **Why is a ring buffer good for RR?** O(1) push and pop, and no shifting or
   allocation per operation. The head index wraps with `% capacity`. FreeRTOS
   queues are ring buffers too.
3. **How does choosing the quantum trade off?** A small quantum gives better
   response time but more context switches (see the comparison table). A huge
   quantum degenerates into FCFS.
4. **One heap class serves three schedulers. How?** `HeapScheduler<Compare>` is
   a template. Priority, RM and EDF just pass different comparators. That is
   compile-time polymorphism inside runtime polymorphism.
5. **What is starvation, and does your Priority scheduler have it?** Yes: a
   steady stream of urgent jobs can block a low-priority job forever. Fixes
   include aging (slowly raising the priority of waiting jobs).

## Phase 4: RM, EDF and schedulability

1. **State the Liu & Layland bound.** U ≤ n(2^(1/n) − 1), which tends to ln 2 ≈ 0.69.
   It is sufficient, not necessary: our 0.971 set fails it but could in
   principle still pass (here it doesn't, as the simulation shows).
2. **Why can EDF reach 100% utilization but RM can't?** EDF re-ranks jobs by
   their actual deadlines at runtime. RM fixes priorities by period ahead of
   time, so it can run a less urgent job first and waste capacity.
3. **Then why do most RTOSes use fixed priorities?** They are predictable under
   overload (low-priority tasks fail first, as designed), O(1) to implement with
   a bitmap, and easy to certify. EDF overload can cascade misses into every task.
4. **What is the hyperperiod and why simulate it?** The LCM of all periods.
   After it the release pattern repeats, so one hyperperiod covers every case.

## Phase 5: statistics, Gantt chart and comparison

1. **Define waiting, turnaround and response time.** Turnaround = finish −
   release. Waiting = turnaround − burst. Response = first run − release.
   Response time matters most for interactive and real-time tasks.
2. **Why are Statistics and Report separate classes?** Single responsibility:
   one computes numbers (pure, testable), the other formats them. Changing the
   table layout can't break the maths.
3. **How do you count a context switch?** Each time the CPU goes from one job
   to a different job. Idle gaps are not counted. Real switches cost time
   (saving and restoring registers), which we model as zero.

## Phase 6: testing

1. **How did you get the expected values in your scheduler tests?** I worked
   each timeline out by hand for tiny task sets, then asserted the exact tick
   sequence. That makes the tests document the algorithms too.
2. **Why link the tests against a library rather than the executable?** The
   tests call the same code directly. There is no stdout parsing and no
   duplicated source files.
