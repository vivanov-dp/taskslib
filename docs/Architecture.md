# TasksLib Architecture & Tutorial #

This is a short overview and explanation of the core concepts of the library and how to use them.


## 1. Threads Scheduling: The Queue ##

The library consists of a scheduler class - **TasksQueue**, and an executable wrapper class - **Task**. To use them we instantiate the queue first:

```
#include "TasksQueue.h"

TasksQueue queue({5,1,1});
```

The above code will create a tasks queue with 5 regular threads, 1 non-blocking thread and 1 thread for time management. Alternatively we can split the creation and initialization phases like this:

```
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
  There is an option in the *Task* object, that marks it as *blocking* - to specify that it's executable code may take a long time to finish - for example this can be a task that makes an http:// request to some URL and takes a second or two to return the data, or it might be a path finding job that has to traverse a lot of terrain. We maintain a certain number of worker threads, called *NonBlocking threads* that ignore such tasks in order to always have threads available to do other jobs - this helps avoid a situation where all threads are blocked waiting for long lasting tasks and quicker tasks pile up in the queue. The number of these threads is usually set lower than the blocking threads, but this depends on the particular workload.
- **Time Management Threads**
  The *Task* allows to be scheduled with a delay - we can tell a task to wait for 5 seconds and then execute. When we do that, the task goes on a special queue for suspended tasks and the *Time Management threads* are responsible to move it out of there and onto the normal scheduling queue when the time comes. There is currently no reason to have more than one such thread, but the effectiveness of the code is the same with or without this option, so we implemented it anyway just for the sake of consistency.
- **The Main Thread**
  The thread in which the core application loop is performed, is considered the *main thread*. *Tasks* have an option to execute either in a worker thread, or on the main thread and this can be used as a mechanism to transfer execution and data from one to the other. For example, the tasks can be used to outsource CPU-heavy execution to worker threads, so that the main loop is not delayed (and, if it's a video game - the frame rate is not dropped), and when the work is done, the tasks are rescheduled on the main thread and can call callbacks and apply results to the global objects, without requiring locks on them.
  
  In order for this to work, the *TasksQueue* must regularly receive a call to its `Update()` method, performed on the main thread - it is designed to be simply called from the main loop. The `Update()` will execute any tasks waiting to execute on the main thread, so we need to be cautious of what we put there, as it might slow down the whole application.
  
  ***Note:*** *From the *queue's* point of view, the thread on which it receives the `Update()` call is considered the main thread, but technically it could be any other thread too.*


## 2. Executable Code: Tasks ##

The code we execute using the library is wrapped in *Tasks*. We can use any kind of a callable - a function, a lambda, a bind to a class method - as long as it's signature looks like this: `void Callable(TasksQueue* queue, std::shared_ptr<Task> task)`. 

The ***queue*** parameter points to the queue that executes the task, it's guaranteed only during the execution and it's not intended to be passed and stored in data structures outside that piece of code. 

The ***task*** parameter on the other hand is a shared pointer to the task itself and can be freely passed around and stored. The queue maintains an internal reference to it at all times, until the execution completes - that happens when the executable code returns without calling `task->Reschedule()` beforehand. At that point the task instance will be destroyed, unless we stored a reference to the shared task pointer somewhere else.

The executable code - we can call it simply a callback - is passed as a parameter to the construcotr of the *Task*. The task itself is then scheduled on the queue via it's `AddTask()` method. Here is an example:

```
#include <iostream>
#include "Task.h"
#include "TasksQueue.h"

TasksQueue queue({5,1,1});
auto lambda = [](TasksQueue* queue, TaskPtr task)->void {
  std::cout << "Hello World!";
};

Task task(lambda);
queue->AddTask(task);
```

The code above will create a scheduler queue, then it will create a task that prints "Hello World!" and run it in one of the queue's worker threads. As soon as the text is printed to cout, the task will complete and it will be removed from the queue.