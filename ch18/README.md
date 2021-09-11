# 第 18 章 多线程服务器端的实现

## 1、理解线程的概念

### 1.1 引入线程的概念

如前所述，创建进程的工作本身会给操作系统带来相当沉重的负担。而且每个进程具有独立的内存空间，所以进程间通信的实现难度也会随之提高。换言之，多进程模型的缺点可概括如下。

* 创建进程的过程会带来一定的开销
* 为了完成进程间数据交换，需要特殊的IPC技术

相比上述两点，“每秒少则数10次，多则数千次的‘上下文切换(Context Switching)’是创建进程时最大的开销。”



由于需要运行的进程数远远多于CPU核心数，为了提高CPU运行效率，采用了事件片轮转的方式实现了宏观上的并发执行。即每个进程独占一个CPU核心运行一定时间后，切换为下一个进程执行，由于每个进程执行的时间片极短，因此宏观上感觉所有进程在并发执行。但频繁的进程切换也需要产生较大的开销。



为了保持多进程的优点，同时在一定程度上克服其缺点，人们引入了线程"Thread"。这是一种将进程的各种劣势降至最低限度（不是直接消除）而设计的一种“轻量级进程”。线程相比于进程具有如下优点：

* 线程的创建和上下文切换比进程的创建和上下文切换更快
* 线程间交换数据时无需特殊技术



### 1.2 线程和进程的差异

每个进程的内存空间都由保存全局变量的“数据区”，向malloc等函数的动态分配提供空间的堆(Heap)，函数运行时使用的栈(Stack)构成。每个进程都拥有这种独立的空间，多个进程的内存结构如下图所示。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-19%20%E4%B8%8B%E5%8D%888.41.10.png" alt="截屏2021-05-19 下午8.41.10" style="zoom: 67%;" />


而如果获得多个代码执行流为主要目的，则不应该向图18-1那样完全分离内存结构，而只需分离栈区域，通过这种方式可以获得如下优势：

* 上下文切换时不需要切换数据区和堆。
* 可以利用数据区和堆交换数据。

线程的内存结构如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-19%20%E4%B8%8B%E5%8D%888.44.48.png" alt="截屏2021-05-19 下午8.44.48" style="zoom:67%;" />



进程和线程可以定义为如下形式：

* 进程：在操作系统构成单独执行流的单位。
* 线程：在进程构成单独执行流的单位。

操作系统，进程，线程之间的关系如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-19%20%E4%B8%8B%E5%8D%888.50.06.png" alt="截屏2021-05-19 下午8.50.06" style="zoom: 67%;" />



## 2、线程创建及运行

线程具有单独的执行流，因此需要单独定义线程的main函数，还需要请求操作系统在单独的执行流中执行该函数，完成该功能的函数如下：

```c
#include <pthread.h>

int pthread_create(
pthread_t* restrict thread, const pthread_attr_t * restrict attr, 
void * (* start_routine)(void *), void * restrict arg);

// 成功时返回0， 失败时返回其他值

/*
	thread 保存新创建线程ID的变量地址值，线程与进程相同，也需要用于区分不同线程的ID。
	arrt 用于传递线程属性的参数，传递NULL时，创建默认属性的线程
	start_routing 相等于线程的main函数，在单独执行流中执行的函数地址值（函数指针）
	arg 通过第三个参数传递的函数的形参变量地址值
*/

```

restrict的作用，确保多个指针不指向同一数据。

通过以下示例了解该函数的功能。

thread1.c

```c
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void * thread_main(void *args);

int main(int argc, char* argv[]){
    pthread_t t_id;
    int thread_param = 5;

    if(pthread_create(&t_id, NULL, thread_main, (void*)&thread_param) !=0){
        puts("pthread_create() error");
        return -1;
    }
    
    sleep(10);
    puts("end of main");

    return 0;
}

void * thread_main(void * args){
    int cnt = *((int*)args);
    for(int i=0; i<cnt; i++){
        sleep(1);
        puts("running thread");
    }
    return NULL;
}
```



编译运行

编译时需要使用 -lpthread来使用线程库

```shell
$ gcc thread1.c -o ./bin/thread1 -lpthread
$ gcc thread1.c -o ./bin/thread1 -lpthread
$ ./bin/thread1 
running thread
running thread
running thread
running thread
running thread
end of main
```



thread1.c的执行流程如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-19%20%E4%B8%8B%E5%8D%889.20.22.png" alt="截屏2021-05-19 下午9.20.22" style="zoom: 67%;" />



代码中 sleep(10);是为了给线程运行提供时间，不然main函数结束后，线程也就终止运行了。如果不使用sleep(10)，程序就如下图所示执行：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-19%20%E4%B8%8B%E5%8D%889.22.png" alt="截屏2021-05-19 下午9.22.04" style="zoom:50%;" />



上述调用sleep函数只是为了演示，在实际开发中，通常利用下面的函数控制线程的执行流。

```c
#include <pthread.h>

int pthread_join(pthread_t thread, void **status);
// 成功时返回0， 失败时返回其他值

/*
	thread 该参数值ID的线程终止后才会从该函数返回
	status 保存线程的main函数返回值的指针变量的地址值
*/
```

下面通过示例了解该函数：

thread2.c

```c
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void *thread_main(void *args);

int main(int argc, char *argv[])
{
    pthread_t t_id;
    int thread_param = 5;
    void * thr_ret;

    if (pthread_create(&t_id, NULL, thread_main, (void *)&thread_param) != 0)
    {
        puts("pthread_create() error");
        return -1;
    }

    if(pthread_join(t_id, &thr_ret)!=0){
        puts("pthread_join() error");
        return -1;
    }

    printf("Thread return message: %s",(char*)thr_ret);
    free(thr_ret);
    return 0;
}

void *thread_main(void *args)
{
    int cnt = *((int *)args);
    char *msg = (char*)malloc(sizeof(char)*50);
    strcpy(msg, "Hi, I am thread\n");
    for (int i = 0; i < cnt; i++)
    {
        sleep(1);
        puts("running thread");
    }
    return msg;
}
```



编译运行

```c
$ gcc thread2.c -o ./bin/thread2 -lpthread
$ ./bin/thread2
running thread
running thread
running thread
running thread
running thread
Thread return message: Hi, I am thread
```

该示例的执行流程图如下：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-19%20%E4%B8%8B%E5%8D%889.35.25.png" alt="截屏2021-05-19 下午9.35.25" style="zoom:50%;" />



## 3、可在临界区调用的函数

关于线程的运行需要考虑“多个线程同时调用函数时（执行时）可能产生问题”。这类函数内部存在临界区(Critical Section)，也就是说，多个线程同时执行这部分代码时，可能引起问题，临界区中至少存在一条这类代码。

根据临界区是否引起问题，函数可分为以下两类：

* 线程安全函数(Thread-safe function)
* 非线程安全函数(Thread-unsafe function)

线程安全函数被多个线程同时调用时也不会引起问题，反之，非线程安全函数同时被调用时可能会引发问题。

Unix或windows在定义非线程安全函数的同时，提供了具有相同功能的线程安全函数。比如第 8 章介绍的如下函数就不是线程安全函数

```c
struct hostent * gethostbyname(const char* hostname);
```

同时提供了线程安全的同一功能的函数：

```c
struct hostent * gethostbyname_r (const char * name, struct hostent * result, char * buffer, int buflen, int *h_errnop);
```



Unix下线程安全函数的名称后缀通常为_r。



可以在编译时通过添加 -D_REENTRANT选项定义宏，可以将非线程安全函数调用给问线程安全函数调用。

```shell
$ gcc -D_REENTRANT thread.c -o ./bin/thread -lpthread
```



## 4、工作（Worker）线程模型

接下来给出多线程示例，计算1到10的和，但并不是在main函数中进行累加运算，而是创建2个线程，其中一个线程计算1到5的和，另一个线程计算6到10的和，main函数只负责输出运算结果。这种模式的编程模型称为“工作线程（Worker thread）”模型。计算1到5之和的线程与计算6到10之和的线程将称为main线程管理的工作(Worker)。以下是该示例的执行流程图：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-19%20%E4%B8%8B%E5%8D%8811.26.28.png" alt="截屏2021-05-19 下午11.26.28" style="zoom:50%;" />



thread3.c

```c
#include <stdio.h>
#include <pthread.h>

void* thread_summation(void* arg);
int sum=0;

int main(int argc, char* argv[]){
    pthread_t id_t1, id_t2;
    int range1[] = {1,5};
    int range2[] = {6,10};

    pthread_create(&id_t1, NULL, thread_summation, (void*)range1);
    pthread_create(&id_t2, NULL, thread_summation, (void*)range2);

    pthread_join(id_t1, NULL);
    pthread_join(id_t2, NULL);
    printf("result : %d\n",sum);
    return 0;
}

void * thread_summation(void *arg){
    int start = ((int*)arg)[0];
    int end = ((int*)arg)[1];
    while(start<=end){
        sum+=start;
        start++;
    }
    return NULL;
}

```



编译运行

```shell

$ gcc -D_REENTRANT thread3.c -o ./bin/thread3 -lpthread
$ ./bin/thread3
result : 55
```



运行结果是55，结果虽然正确，但示例本身存在问题。此处存在临界区相关问题。因此再介绍另一示例，该示例与上述示例相似，只是增加了发生临界区相关错误的可能性。

thread4.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREAD 100
void* thread_inc(void* arg);
void* thread_des(void* arg);
long long num=0;

int main(int argc, char* argv[]){
    pthread_t thread_id[NUM_THREAD];
    for(int i=0; i<NUM_THREAD; i++){
        if(i%2){
            pthread_create(&thread_id[i], NULL, thread_inc, NULL);
        }else{
            pthread_create(&thread_id[i], NULL, thread_des, NULL);
        }
    }

    for(int i=0; i<NUM_THREAD; i++){
        pthread_join(thread_id[i], NULL);
    }

    printf("result : %lld\n",num);
    return 0;
}

void * thread_inc(void * arg){
    for(int i=0; i<10000;i++){
        num+=1;
    }

    return NULL;
}

void *thread_des(void *arg)
{
    for (int i = 0; i < 10000; i++)
    {
        num -= 1;
    }

    return NULL;
}
```



编译运行

```shell
$ gcc -D_REENTRANT thread4.c -o ./bin/thread4 -lpthread
$ ./bin/thread4
result : -14490
$ ./bin/thread4
result : 9245
$ ./bin/thread4
result : -16355
$ ./bin/thread4
```



运行结果并不是0，而且每次运行结果都不相同，虽然原因尚不得而知，但可以肯定的是，这对于线程的运用是个大问题。



## 5、线程存在的问题和临界区

### 5.1 多个线程访问同一变量是问题

示例 thread4.c存在的问题如下：

“2个线程正在同时访问全局变量num”

此处的“同时访问”与平时所接触的同时有一定区别，下面通过示例解释“同时访问”的含义，并说明为何会存在问题。



假设2个线程要执行变量逐次加1的工作，如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-20%20%E4%B8%8A%E5%8D%8812.03.43.png" alt="截屏2021-05-20 上午12.03.43" style="zoom:50%;" />



图18-8中描述的是2个线程准备将变量num的值加1的情况。在此状态下，线程1将变量num的值增加到100，线程2再访问num时，变量num中将按照我们的预想保存101。下图是线程1将num完全增加后的情形。

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-20%20%E4%B8%8A%E5%8D%8812.07.14.png" alt="截屏2021-05-20 上午12.07.14" style="zoom:50%;" />



图18-9中需要注意值的增加方式，值的增加需要CPU运算完成，变量num中的值不会自动增加。线程1首先读该变量的值并将其传递给CPU，获得加1之后的结果100，最后再把结果写回num，这样num中就保存100。接下来线程2的执行过程如图：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-20%20%E4%B8%8A%E5%8D%8812.11.13.png" alt="截屏2021-05-20 上午12.11.13" style="zoom:50%;" />



变量num将保存101，这是最理想的情况。线程1完全增加num值之前，线程2完全有可能通过切换得到cpu资源。如下图所示，线程1读取变量num的值并完成加1运算时的情况，只是加1后的结果尚未写入变量num

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-20%20%E4%B8%8A%E5%8D%8812.13.59.png" alt="截屏2021-05-20 上午12.13.59" style="zoom: 67%;" />



接下来就要将100保存到变量num中，但执行该操作前，执行流程跳转到了线程2，线程2完成了加1运算，并将加1之后的结果写入变量num，如下图所示：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-20%20%E4%B8%8A%E5%8D%8812.16.06.png" alt="截屏2021-05-20 上午12.16.06" style="zoom:67%;" />



从图18-12中可以看出，变量num的值尚未被线程1加1到100，因此线程2读到变量num的值为99，结果是线程2将num值改为100，还剩下线程1将运算后的结果写入变量num的操作。接下来给出过程：

<img src="https://raw.githubusercontent.com/wangjunstf/pics/main/uPic/%E6%88%AA%E5%B1%8F2021-05-20%20%E4%B8%8A%E5%8D%8812.18.19.png" alt="截屏2021-05-20 上午12.18.19" style="zoom:67%;" />



此时线程1将自己的运算结果100再次写入变量num，结果变量num变成100，虽然线程1和线程2各做了1次加1运算，却得到了意想不到的结果。因此，线程访问变量num时应该阻止其他线程访问，直到线程1完成运算。这就是同步(Synchronization)。



### 5.2 临界区位置

临界区的定义：

“函数内同时运行多个线程时引起问题的多条语句构成的代码块”



全局变量num是否是临界区？

不是，因为它不是引起问题的语句。该变量并非同时运行，只是代表内存区域的声明而已。临界区通常位于线程运行的函数内部。下面观察thread4.c中两个函数：

```c
void * thread_inc(void * arg){
    for(int i=0; i<10000;i++){
        num+=1;     // 临界区
    }

    return NULL;
}

void *thread_des(void *arg)
{
    for (int i = 0; i < 10000; i++)
    {
        num -= 1;     // 临界区
    }

    return NULL;
}
```



由代码可知，临界区并非num本身，而是访问num的两条语句。这2条语句可能由多个线程同时运行，这也是引起问题的直接原因。产生的问题可以整理为如下3种情况：

* 2个线程同时执行thread_inc函数
* 2个线程同时执行thread_des函数
* 2个线程同时执行thread_inc函数和thread_des函数

“线程1执行thread_inc函数的num+=1语句的同时，线程2执行thread_des函数的num-=1语句”

也就是说：2条不同语句由不同线程同时执行时，也有可能构成临界区。前提是这2条语句访问同一内存区域。



## 6、线程同步

线程同步用于解决线程访问顺序引发的问题。需要同步的情况可以从如下两方面考虑。

* 同时访问同一内存空间时发生的情况。
* 需要指定访问同一内存空间的线程执行顺序的情况。

线程同步采用的常用技术为："互斥量"和“信号量”，二者概念上十分接近。



### 6.1 互斥量

互斥量是"Mutual Exclusion"的简写，表示不允许多个线程同时访问。互斥量主要用于解决线程同步访问的问题。

互斥量就像一把锁，当一个线程使用某个变量时，就把该变量锁住，直到该线程使用完该变量解锁后，其他线程才能访问该变量。

接下来介绍互斥量的创建和销毁函数。

```c
#include <pthread.h>

int pthread_mutex_init(pthread_mutex_t * mutex, const pthread_mutexattr_t* attr);
int pthread_mutex_destroy(pthread_mutex_t * mutex);

// 成功时返回0，失败时返回其他值

/*
	mutex 创建互斥量时传递保存互斥量的变量地址值，销毁时传递需要销毁的互斥量地址值
	attr 传递即将创建的互斥量属性，没有特别需要指定的属性时传递NULL
*/
```



为了创建相当于锁系统的互斥量，需要声明如下pthread_mutex_t型变量：

pthread_mutex_t mutex;



接下来介绍利用互斥量锁住和释放临界区时使用的函数。

```c
#include <pthread.h>

int pthread_mutex_lock(pthread_mutext_t *mutex);
int pthread_mutex_unlock(pthread_mutext_t *mutex);
// 成功时返回0，失败时返回其他值
```



可以通过如下结构保护临界区：

```c
pthread_mutex_lock(&mutex);
//临界区开始
//...
//临界区结束
pthread_mutex_unlock(&mutex);
```

互斥量就像一把锁，阻止多个进程访问临界区。



### 6.2 死锁

当线程退出临界区时，如果忘了调用pthread_mutex_unlock函数，那么其他为了进程临界区而调用pthread_mutex_lock函数的线程就无法摆脱阻塞状态。这样的情况称为“死锁”(Dead-lock)。



接下来利用互斥量解决示例thread4.c中遇到的问题：

mutex.c

```c
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define NUM_THREAD 100
void *thread_inc(void *arg);
void *thread_des(void *arg);
long long num = 0;

pthread_mutex_t mutex;

int main(int argc, char *argv[])
{
    pthread_t thread_id[NUM_THREAD];
    pthread_mutex_init(&mutex, NULL);
    for (int i = 0; i < NUM_THREAD; i++)
    {
        if (i % 2)
        {
            pthread_create(&thread_id[i], NULL, thread_inc, NULL);
        }
        else
        {
            pthread_create(&thread_id[i], NULL, thread_des, NULL);
        }
    }

    for (int i = 0; i < NUM_THREAD; i++)
    {
        pthread_join(thread_id[i], NULL);
    }

    printf("result : %lld\n", num);
    return 0;
}

void *thread_inc(void *arg)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < 1000000; i++)
    {
        num += 1;
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *thread_des(void *arg)
{
    for (int i = 0; i < 1000000; i++)
    {
        pthread_mutex_lock(&mutex);
        num -= 1;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}
```



编译运行

```shell
$ gcc mutex.c -D_REENTRANT  -o ./bin/mutex -lpthread
$ ./bin/mutex 
result : 0
$ ./bin/mutex 
result : 0
$ ./bin/mutex 
result : 0
```



从运行结果看，已经解决了thread4.c中的问题。但确认运行时间需要等待较长时间。因为互斥lock，unlock函数的调用过程比想象中花费更长时间。



观察下列代码：

```c
void *thread_inc(void *arg)
{
    pthread_mutex_lock(&mutex);
    for (int i = 0; i < 1000000; i++)
    {
        num += 1;
    }
    pthread_mutex_unlock(&mutex);

    return NULL;
}
```



以上临界区划分范围较大，但这考虑到如下优点所做的决定：

“最大限度减少互斥量lock，unlock函数的调用次数”



缺点就是变量num值怎加到1000000前，都不允许其他线程访问。



到底应该扩大临界区还是缩小临界区，没有标准答案，需要根据实际情况考虑。



### 6.3 信号量

信号量与互斥量极为相似，在互斥量的基础上很容易理解信号量。此处只涉及利用“二进制信号量”（只用0和1）完成“控制线程顺序”为中心的同步方法。

下面给出信号了创建及销毁方法

```c
#include <semaphore.h>

int sem_init(sem_t *sem, int pshared, unsigned int value);
int sem_destroy(sem_t * sem);

// 成功时返回0， 失败时返回其他值

/*
	sem 创建信号量时传递的保存信号量的变量地址值，销毁时传递需要销毁的信号量变量地址值
	pshared 传递其他值时，创建可由多个进程共享的信号量； 传递0时，创建只允许1个进程内部使用的信号量。我们需要完成同一进程内的线程同步，故传递0
	value 指定新创建 的信号量初始值
*/
```



接下来介绍相当于互斥量lock，unlock的函数：

```c
#include <semaphore.h>

int sem_post(sem_t * sem);
int sem_wait(sem_t * sem);

//成功时返回0，失败时返回其他

/*
	传递保存信号量读取值的变量地址值，传递给sem_post时信号量增1，传递给sem_wait时信号量减1
*/
```



信号量的值不能小于0，因此，在信号量为0的情况下调用sem_wait函数时，调用函数的线程将进入阻塞状态。此时如果有其他线程调用sem_post函数，信号量的值将变为1，原本阻塞的线程将该信号量重新减为0并跳出阻塞状态。

可以通过如下结构同步临界区，假设信号量的初始值为1

sem_wait(&sem);   //信号量变为0

//临界区开始

//。。。

//临界区的结束

sem_post(&sem); //信号量变为1



调用sem_wait函数进入临界区的线程在调用sem_post函数之前不允许其他线程进入临界区。

现用信号量实现以下功能：

“线程A从用户输入的到值后存入全局变量num，此时线程B将取走该值并累加。该过程进行5次，完成后输出总和并退出程序”

semaphore.c

```c
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

void *read(void *arg);
void *accu(void *arg);

static sem_t sem_one;
static sem_t sem_two;
static int num;

int main(int argc, char* argv[]){
    pthread_t id_t1, id_t2;
    sem_init(&sem_one,0,0);
    sem_init(&sem_two,0,1);

    pthread_create(&id_t1, NULL, read, NULL);
    pthread_create(&id_t2, NULL, accu, NULL);

    pthread_join(id_t1, NULL);
    pthread_join(id_t2, NULL);

    sem_destroy(&sem_one);
    sem_destroy(&sem_two);

    return 0;
}

void *read(void *arg){
    for(int i=0; i<5; i++){
        fputs("Input num: ", stdout);
        sem_wait(&sem_two);
        scanf("%d",&num);
        sem_post(&sem_one);
    }
    return NULL;
}

void *accu(void *arg){
    int sum=0;
    for(int i=0; i<5; i++){
        sem_wait(&sem_one);
        sum+=num;
        sem_post(&sem_two);
    }
    printf("sum = %d\n",sum);
    return NULL;
}
```



编译运行

```shell
$ gcc semaphore.c -D_REENTRANT -o ./bin/semaphore -lpthread
$ ./bin/semaphore 
Input num: 1
Input num: 2
Input num: 3
Input num: 4
Input num: 5
sum = 15
```



## 7、线程的销毁和多线程并发服务器端的实现

### 7.1 销毁线程的3种方法

Linux线程并不是在首次调用的线程main函数返回时自动销毁，所以用如下2种方式之一加以明确。否则由线程创建的内存空间将一直存在。

* 调用pthread_join函数
* 调用pthread_detach函数

之前使用pthread_join函数，不仅会等待线程终止，还会引导线程销毁。但该函数的问题是，线程终止前，调用该函数的线程将进入阻塞状态。

为了解决上述问题，通常使用下面的方式来引导线程销毁：

```c
#include <pthread>

int pthread_detach(pthread_t thread);
// 成功时返回0，失败时返回其他值

/*
	thread 终止的同时需要销毁的线程ID
*/
```



调用上述函数不会引起线程终止或进入阻塞状态，可以通过该函数引导销毁销毁线程创建的内存空间。



### 7.2 多线程服务器端的实现

接下来使用多线程实现以下功能：“多个客户端之间可以交换信息的简单的聊天程序”

chat_server.c

```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void errorHandling(const char *message);
void *handle_clnt(void *arg);
void *send_msg(char* msg, int len);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutex;

int main(int argc, char *argv[])
{
    int str_len, state;
    int serv_sock, clnt_sock;
    char message[BUF_SIZE];
    pthread_t t_id;

    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    if (argc != 2)
    {
        printf("Usage: %s <port> \n", argv[1]);
        exit(1);
    }

    pthread_mutex_init(&mutex, NULL);

    //调用socket函数创建套接字
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        errorHandling("socket() error");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    //调用bind函数分配IP地址和端口号
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        errorHandling("bind() error!");
    }

    //调用listen函数将套接字转化为可接收连接状态
    if (listen(serv_sock, 5) == -1)
    {
        errorHandling("listen() error!");
    }
    //write函数用于传输数据，若程序经accept函数运行到本行，说明已经有了连接请求
    //调用accept函数受理连接请求，如果在没有连接请求的情况下调用该函数，则不会返回，直到有连接请求为止。

    
    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        pthread_mutex_lock(&mutex);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutex);

        pthread_create(&t_id, NULL, handle_clnt, (void*)&clnt_sock);
        pthread_detach(t_id);
        printf("Connect client ID: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    pthread_mutex_destroy(&mutex);
    return 0;
}

void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void * handle_clnt(void * arg){
    int clnt_sock = *((int*)arg);
    int str_len = 0;
    char msg[BUF_SIZE];
    while((str_len=read(clnt_sock, msg, sizeof(msg)))!=0){
        send_msg(msg, str_len);
    }

    pthread_mutex_lock(&mutex);
    for(int i=0; i<clnt_cnt; i++){
        if(clnt_sock==clnt_socks[i]){
            while(i++<clnt_cnt-1){
                clnt_socks[i] = clnt_socks[i+1];
            }
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutex);
    close(clnt_sock);
    return NULL;
}

void* send_msg(char * msg, int len){
    pthread_mutex_lock(&mutex);
    for(int i=0 ; i<clnt_cnt; i++){
        write(clnt_socks[i], msg, len);
    }
    pthread_mutex_unlock(&mutex);
}
```





chat_clnt.c

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define NAME_SIZE 20

void errorHandling(const char *message);
void* send_msg(void* msg);
void *recv_msg(void* msg);

char name[NAME_SIZE] = "[default]";
char msg[BUF_SIZE];
int main(int argc, char *argv[])
{
    int sock;
    char message[BUF_SIZE];
    int str_len;
    struct sockaddr_in serv_adr;
    pthread_t snd_thread, rcv_thread;
    void *thread_return;

    if (argc != 4)
    {
        printf("Usage : %s <IP> <port> <name>\n", argv[0]);
        exit(1);
    }

    sprintf(name, "[%s]",argv[3]);
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        errorHandling("socket() error");
    }

    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_adr.sin_port = htons(atoi(argv[2]));

    if (connect(sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        errorHandling("connect() error!");
    }
    else
    {
        puts("Connected......");
    }

    pthread_create(&snd_thread, NULL, send_msg, (void*)&sock); 
    pthread_create(&rcv_thread, NULL, recv_msg, (void*)&sock);

    pthread_join(snd_thread, &thread_return);
    pthread_join(rcv_thread, &thread_return);
    close(sock);
    return 0;
}

void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void * send_msg(void *arg){
    int sock = *((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    while(1){
        fgets(msg, BUF_SIZE, stdin);
        if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")){
            close(sock);
            exit(0);
        }
        sprintf(name_msg, "%s %s",name, msg);
        write(sock, name_msg, strlen(name_msg));
    }
    return NULL;
}

void * recv_msg(void * arg){
    int sock = *((int*)arg);
    char name_msg[NAME_SIZE+BUF_SIZE];
    int str_len;
    while(1){
        str_len = read(sock, name_msg, NAME_SIZE+BUF_SIZE-1);
        if(str_len==-1){
            return (void*)-1;  //将-1转换为void指针
        }
        name_msg[str_len]=0;

        fputs(name_msg, stdout);
    }
    return NULL;
}





void read_routine(int sock, char *buf)
{
    while (1)
    {
        int str_len = read(sock, buf, BUF_SIZE);
        if (str_len == 0)
            return;

        buf[str_len] = 0;
        printf("Message from server: %s", buf);
    }
}

void write_routine(int sock, char *buf)
{
    while (1)
    {
        fgets(buf, BUF_SIZE, stdin);
        if (!strcmp(buf, "q\n") || !strcmp(buf, "Q\n"))
        {
            shutdown(sock, SHUT_WR);
            return;
        }

        write(sock, buf, strlen(buf));
    }
}

int itoc(int num, char *str)
{
    char tem[1024];
    int id = 0, id2 = 0;
    while (num)
    {
        int t = num % 10;
        tem[id++] = t + '0';
        num /= 10;
    }
    str[id--] = '\0';
    while (id >= 0)
    {
        str[id2++] = tem[id--];
    }
    return 0;
}
```

编译运行

```shell
$ gcc chat_server.c -D_REENTRANT -o ./bin/chat_server -lpthread
$ ./bin/char_server 9190 
Connect client ID: 127.0.0.1 
Connect client ID: 127.0.0.1 
```



```shell
$ gcc chat_clnt.c -D_REENTRANT -o ./bin/chat_clnt -lpthread
$ ./bin/chat_clnt 127.0.0.1 9190 dog
Connected......
Hi, I am dog
[dog] Hi, I am dog
[cat] Hi, I am cat
```



```shell
$ ./bin/chat_clnt 127.0.0.1 9190 cat
Connected......
[dog] Hi, I am dog
Hi, I am cat               
[cat] Hi, I am cat
```



## 8、习题（参考答案）

![截屏2021-05-21 下午4.27.17](/Users/wangjun/Desktop/截图/截屏2021-05-21 下午4.27.17.png)

![截屏2021-05-21 下午4.27.29](/Users/wangjun/Desktop/截图/截屏2021-05-21 下午4.27.29.png)


（1）为了让单核CPU同时执行多个进程，采用了事件片轮转的方式实现了宏观上的并发执行。即每个进程独占一个CPU核心运行一定时间后，切换为下一个进程执行，由于每个进程执行的时间片极短，因此宏观上感觉所有进程在并发执行。但频繁的进程切换也需要产生较大的开销。

来自百度百科的一段解释：上下文切换

上下文切换 (context switch) , 其实际含义是任务切换, 或者CPU寄存器切换。当多任务内核决定运行另外的任务时, 它保存正在运行任务的当前状态, 也就是CPU寄存器中的全部内容。这些内容被保存在任务自己的堆栈中, 入栈工作完成后就把下一个将要运行的任务的当前状况从该任务的栈中重新装入CPU寄存器, 并开始下一个任务的运行, 这一过程就是context switch。



（2）每个进程都具有独立的内存空间，包括“数据区”，”堆(Heap)“，“栈(Stack)”等。上下文切换时需要更多时间。

而多个线程共享数据区和堆，只独享栈区，在上下文切换时需要的时间更少。

因为多个线程共享数据区，因为可以通过数据区和堆区通信。



（3）进程：一个进程可以通过调用fork函数创建子进程，根据返回值pid区分父子进程，pid==0的部分为子进程执行区域，pid!=0的部分为父进程执行区域，其余部分父子进程共有。子进程结束后，由父进程负责回收其资源，可以通过调用wait,waitpid函数或信号处理方式。

线程：一个进程可以通过pthread_create函数创建多个线程，每个线程共享数据区和堆区，拥有独立的栈区。线程执行完毕由父进程负责回收其资源，通常调用pthread_join或pthread_detach。



（4）c：例如：假设有A，B两个线程，线程A负责向指定内存空间写入数据，线程B负责取走该数据。这是线程A执行的代码块和线程B执行的代码块就构成临界区。

d：只要两个线程同时访问同一个内存区域，就可能构成临界区。



（5）d



（6）Linux中可通过在进程中调用下列函数销毁创建的线程。

* 调用pthread_join函数
* 调用pthread_detach函数



（7）

```c
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define BUF_SIZE 100
#define MAX_CLNT 256

void errorHandling(const char *message);
void *handle_clnt(void *arg);
void *send_msg(char *msg, int len);

int clnt_cnt = 0;
int clnt_socks[MAX_CLNT];
pthread_mutex_t mutex;
pthread_mutex_t msg_mutex;

int main(int argc, char *argv[])
{
    int str_len, state;
    int serv_sock, clnt_sock;
    char message[BUF_SIZE];
    pthread_t t_id;

    struct sockaddr_in serv_adr, clnt_adr;
    socklen_t clnt_adr_sz;
    if (argc != 2)
    {
        printf("Usage: %s <port> \n", argv[1]);
        exit(1);
    }

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&msg_mutex, NULL);

    //调用socket函数创建套接字
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
    {
        errorHandling("socket() error");
    }
    memset(&serv_adr, 0, sizeof(serv_adr));
    serv_adr.sin_family = AF_INET;
    serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_adr.sin_port = htons(atoi(argv[1]));

    //调用bind函数分配IP地址和端口号
    if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1)
    {
        errorHandling("bind() error!");
    }

    //调用listen函数将套接字转化为可接收连接状态
    if (listen(serv_sock, 5) == -1)
    {
        errorHandling("listen() error!");
    }
    //write函数用于传输数据，若程序经accept函数运行到本行，说明已经有了连接请求
    //调用accept函数受理连接请求，如果在没有连接请求的情况下调用该函数，则不会返回，直到有连接请求为止。

    while (1)
    {
        clnt_adr_sz = sizeof(clnt_adr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
        pthread_mutex_lock(&mutex);
        clnt_socks[clnt_cnt++] = clnt_sock;
        pthread_mutex_unlock(&mutex);

        pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock);
        pthread_detach(t_id);
        printf("Connect client ID: %s \n", inet_ntoa(clnt_adr.sin_addr));
    }
    close(serv_sock);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&msg_mutex);
    return 0;
}

void errorHandling(const char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void *handle_clnt(void *arg)
{
    int clnt_sock = *((int *)arg);
    int str_len = 0;
    char msg[BUF_SIZE];
    
    pthread_mutex_lock(&msg_mutex);
    while ((str_len = read(clnt_sock, msg, sizeof(msg))) != 0)
    {
        write(clnt_sock, msg, str_len);
        
    }
    pthread_mutex_unlock(&msg_mutex);

    pthread_mutex_lock(&mutex);
    for (int i = 0; i < clnt_cnt; i++)
    {
        if (clnt_sock == clnt_socks[i])
        {
            while (i++ < clnt_cnt - 1)
            {
                clnt_socks[i] = clnt_socks[i + 1];
            }
            break;
        }
    }
    clnt_cnt--;
    pthread_mutex_unlock(&mutex);

    close(clnt_sock);
    return NULL;
}


```



（8）如果不同步，会导致发送给A线程的数据，被B线程取走，或者发送给A线程的数据还没被取走，已经被B接收的数据覆盖了。

如果同步，导致同时只能为一位客户端服务，只有当一个客户端断开连接时，消息缓冲才能被下一个线程访问。







```
#include <iostream>

using namespace std;

int main()
{
    int grade;
    int n;
    printf("学生个数: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++)
    {
        cout << "分数: ";
        cin>>grade;
    
        if(grade<80){
            if(grade>=70){
                cout << "中" <<endl;
            }else if(grade>=60){
                cout<< "及格" <<endl;
            }else{
                cout<< "不及格" <<endl;
            }
        }else{
            if(grade<90){
                cout<< "良" <<endl;
            }else{
                cout<< "优" <<endl;
            }
        }
    }
    return 0;
}
#include <iostream>

using namespace std;

int main()
{
    int grade;
    int n;
    printf("学生个数: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++)
    {
        cout << "分数: ";
        cin>>grade;
    
        if(grade<80){
            if(grade>=70){
                cout << "中" <<endl;
            }else if(grade>=60){
                cout<< "及格" <<endl;
            }else{
                cout<< "不及格" <<endl;
            }
        }else{
            if(grade<90){
                cout<< "良" <<endl;
            }else{
                cout<< "优" <<endl;
            }
        }
    }
    return 0;
}
#include <iostream>

using namespace std;

int main()
{
    int grade;
    int n;
    printf("学生个数: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++)
    {
        cout << "分数: ";
        cin>>grade;
    
        if(grade<80){
            if(grade>=70){
                cout << "中" <<endl;
            }else if(grade>=60){
                cout<< "及格" <<endl;
            }else{
                cout<< "不及格" <<endl;
            }
        }else{
            if(grade<90){
                cout<< "良" <<endl;
            }else{
                cout<< "优" <<endl;
            }
        }
    }
    return 0;
}
#include <iostream>

using namespace std;

int main()
{
    int grade;
    int n;
    printf("学生个数: ");
    scanf("%d", &n);

    for (int i = 0; i < n; i++)
    {
        cout << "分数: ";
        cin>>grade;
    
        if(grade<80){
            if(grade>=70){
                cout << "中" <<endl;
            }else if(grade>=60){
                cout<< "及格" <<endl;
            }else{
                cout<< "不及格" <<endl;
            }
        }else{
            if(grade<90){
                cout<< "良" <<endl;
            }else{
                cout<< "优" <<endl;
            }
        }
    }
    return 0;
}
```



