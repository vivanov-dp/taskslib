# TasksLib Architecture & Tutorial #

## Threads Scheduling: The Queue ##

The library consists of a scheduler class - **TasksQueue**, and an executable wrapper class - **Task**. To use them we instantiate the queue first:

```
#include "Task.h"
#include "TasksQueue.h"

TasksQueue queue({5,1,1});
```

The above code will create a tasks queue with 5 regular threads, 1 non-blocking thread and 1 thread for time management. Alternatively we can split the creation and initialization phases like this:

```
#include "Task.h"
#include "TasksQueue.h"

TasksQueue queue;

...
...

queue.Initialize({5,1,1});
```

In this case the queue is not going to create its threads and is not going to accept tasks before the Initialize method is called. There is also a `Shutdown()` method, which will destroy the threads and put the queue back into the uninitialized state.

The job of the tasks queue is to maintain a list of executable pieces of code  - *tasks*, and run them whenever there are any free threads. If there are more tasks than threads, they will wait in the queue for their turn. The scheduler code goes trough the whole queue on each pass and it doesn't matter in which order the tasks are added to it.

### Configuring the Queue ###

The **TasksQueue** accepts a `TasksQueue::Configuration` struct in it's constructor or `Initialize()` method. This specifies the number of threads to create of each kind.

- **Blocking Threads**
  Standard worker threads, which handle everything. These are the bulk of our threads, we create as many as necessary to handle the load.
- **Non-Blocking Threads**
  There is an option in the **Task** object, that marks it as *blocking* - to specify that it's executable code may take a long time to finish - for example this can be a task that makes an http:// request to some URL and takes a second or two to return the data. We maintain a certain number of worker threads, called *NonBlocking threads* that ignore such tasks in order to always have threads available to do other jobs - this helps avoid a situation where all threads are blocked waiting for long lasting tasks and quicker tasks pile up in the queue. The number of these threads is usually set lower than the blocking threads, but this depends on the particular workload.
- **Time Management Threads**
  The **Task** allows to be scheduled with a delay - we can tell a task to wait for 5 seconds and then execute. When we do that, the task goes on a special queue for suspended tasks and the *Time Management threads* are responsible to move it out of there and onto the normal scheduling queue when the time comes. There is currently no reason to have more than one such thread, but the effectiveness of the code is the same with or without this option, so we implemented it anyway just for the sake of consistency.
- **The Main Thread**
  The thread in which the queue instance is created, is considered the main thread. Usually this would be the thread where the main application loop is performed. **Tasks** have an option to execute either in a worker thread, or on the main thread and this can be used as a mechanism to transfer execution and data from one to the other. For example, the tasks can be used to outsource CPU-heavy execution to worker threads, so that the main loop is not delayed (and the frame rate is not dropped if it is a game), and when the work is done, the tasks are rescheduled on the main thread and can apply the results to the global objects, without requiring any locks on them.
  
  In order for this to work, the **TasksQueue** must regularly receive a call to its `Update()` method, performed on the main thread - we can simply call it from the main loop. The `Update()` will execute any tasks waiting to execute on the main thread, so we need to be cautious of what we put there, as it might slow down the whole application.
