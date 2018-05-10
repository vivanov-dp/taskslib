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

TaskPtr task = std::make_shared<Task>(lambda);
queue.AddTask(task);
```

The code above will create a scheduler queue, then it will create a task that prints "Hello World!" and run it in one of the queue's worker threads. As soon as the text is printed to cout, the task will complete and it will be removed from the queue.

### Rescheduling ###

The *Task* has a public method `Reschedule()`, which if called during the task's execution will make it be placed back on the queue for another pass. Elaborating on the previous example:

```
#include <iostream>
#include "Task.h"
#include "TasksQueue.h"

TasksQueue queue({5,1,1});
auto lambda = [](TasksQueue* queue, TaskPtr task)->void {
  static std::string step = "Once!";

  std::cout << "Hello World! " << step;
		
  if (step == "Once!") {
    step = "Twice!";
    task->Reschedule();
  }
};

TaskPtr task = std::make_shared<Task>(lambda);
queue.AddTask(task);
```

This task will output `"Hello World! Once!"`, then it will go on the queue, execute a second time, output `"Hello World! Twice!"` and then it will end.

This functionality allows us to split tasks into steps and execute each step in sequence. As we will see in the next chapter, the `Reschedule()` method allows us to change the task's options between steps - things like executing it in a worker thread or on the main thread, delaying it for a specified time, or even changing the executable callback itself so that we can use separate lambdas for each step instead of creating a state machine within the function we call.

The original use case that we solved with this, was sending an out-of-band HTTP request with CPR/CURL: We create the request's object and populate it with data in the first step, then we send the request on second step and we mark it as blocking, then on step 3 we decode the returned results and finally we switch to the main thread on step 4 and invoke a callback within the game's code, which will go over the results and update the game state as needed. *(NOTE: For those of you who would like to try it, bear in mind that this requires a modification of CPR's code to split the execution of the request in two parts - creation of a Session object and actual execution of a pre-created Session. All this is a subject of another library we have, called HttpLib, which we might or might not find the time to also publish as OpenSource)*


### Task Options ###

Both the *Task*'s constructor and the `Reschedule()` method accept a varying number of parameters that specify task's options. The list of options and their types are found in the `Types.h` header.

- **TaskThreadTarget**
  *enum*, specifying whether the task should execute in main thread or worker thread. Default is *WORKER_THREAD*.
  
- **TaskBlocking**
  *bool*, if true - the task execution is expected to block for a longer time, so non-blocking threads will ignore it. Default is *false*.

- **TaskPriority**
  *uint32_t*, specifies the priority of the task. Currently task prioritization is not well developed, but there is a basic functionality that will make the queue ignore all tasks with lower priorities until higher priority tasks are complete. Lowest priority is 0, highest is as much as unit32_t can hold. Default is *0*.

- **TaskExecutable**
  *std::function*, a pointer to a callable code - this sets the callback that the queue invokes when executing the task. Default is *nullptr*.

- **TaskDelay**
  *std::chrono::milliseconds*, specifies a sleep time that needs to pass before the task is considered for execution. Default is *0*.

We can call with any number of these parameters and in any order. For example:

```
  #include "Task.h"

  using namespace TasksLib;
```
```
  TaskPtr task1 = std::make_shared<Task>(TaskThreadTarget::MAIN_THREAD, TaskPriority{ 12 }, TaskBlocking{ true });
  TaskPtr task2 = std::make_shared<Task>(true, 54, lambda1);
  TaskPtr task3 = std::make_shared<Task>(std::chrono::milliseconds{ 500 }, lambda1);
  TaskPtr task4 = std::make_shared<Task>(TaskExecutable{ lambda1 }, TaskDelay{ 2500 });
  TaskPtr task5 = std::make_shared<Task>(lambda3);
```

**task1** - Blocking, priority is 12, executes in main thread, but since there is no executable it will simply finish right away.

**task2** - Blocking, priority 54, executes lambda1. *Notice that the parameters can be specified with or without their full type, but we prefer full type because it makes for self-documenting code*

**task3** - Sleep 0.5 seconds, then execute lambda1. Worker thread, priority 0, non-blocking.

**task4** - Sleep 2.5 seconds, then execute lambda1. Worker thread, priority 0, non-blocking.

**task5** - Execute lambda3. Worker thread, priority 0, non-blocking, no sleep.

```
  auto lambda1 = [](TasksQueue* queue, TaskPtr task)->void {
    // .. do something in worker thread

    task->Reschedule(TaskDelay{ 500 }, [](TasksQueue* queue, TaskPtr task)->void {
      // .. do something else in worker thread

      task->Reschedule(TaskThreadTarget::MAIN_THREAD, TaskPriority{ 150 }, [](TasksQueue* queue, TaskPtr task)->void {
        // .. do something to wrap it up in the main thread
      });
    });
  };
```

**lambda1** - Runs once, then reschedules the task back on the queue, but puts it on sleep for 0.5 sec. When the sleep has passed runs second time and reschedules the task to run on the main thread with priority 150. Finishes on the third run.

```
  auto externalObjectPtr = std::make_shared<SomeExternalClass>();
  auto resultsPtr = std::make_shared<ResultsStruct>();
  auto lambda2 = [externalObjectPtr, resultsPtr](TasksQueue* queue, TaskPtr task)->void {
    // .. set results or invoke callbacks in main thread and finish
    externalObjectPtr->someCallbackMethod(resultsPtr);
  };
  auto lambda3 = [resultsPtr](TasksQueue* queue, TaskPtr task)->void {
    
    // .. do something in a worker thread, store results in resultsPtr, then reschedule the task on main thread
    // to run lambda2
    
    resultsPtr->abc = results;
    resultsPtr->xyz = results;
    
    task->Reschedule(lambda2);
  };
```

**lambda2&3** - Lambda3 is executed first, it does some work, then reschedules the task on the main thread to run lambda2, which invokes a callback and exits.

The most widely used case, at least in our code, is *task5*. We use lambda's capturing of local variables to store shared pointers to external objects.

