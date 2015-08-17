/*
 *created on 2015.8.13 by cool
 */
#ifndef ZOEY_THREADPOOL
#define ZOEY_THREADPOOL

#include <pthread.h>

typedef void (*CB_FUN)(void *);

//任务结构体
typedef struct task
{
	void		*argv; //任务函数的参数（任务执行结束前，要保证参数地址有效）
	CB_FUN		handler; //任务函数（返回值必须为0   非0值用作增加线程，和销毁线程池）
	struct task *next; //任务链指针
}zoey_task_t;

//任务队列
typedef struct task_queue
{
	zoey_task_t *head;  //队列头
	zoey_task_t **tail;	//队列尾
	unsigned int maxtasknum; //最大任务限制
	unsigned int curtasknum; //当前任务数
}zoey_task_queue_t;

//线程池
typedef struct threadpool
{
	pthread_mutex_t    mutex;  //互斥锁
	pthread_cond_t     cond;	//条件锁
	zoey_task_queue_t       tasks;//任务队列

	unsigned int       threadnum; //线程数
	unsigned int       thread_stack_size; //线程堆栈大小

}zoey_threadpool_t;

//配置参数
typedef struct threadpool_conf
{
	unsigned int threadnum;    //线程数
	unsigned int thread_stack_size;//线程堆栈大小
	unsigned int maxtasknum;//最大任务限制
}zoey_threadpool_conf_t;

//初始化一个线程池
zoey_threadpool_t* zoey_threadpool_init(zoey_threadpool_conf_t *conf);

//添加一个任务
int zoey_threadpool_add_task(zoey_threadpool_t *pool, CB_FUN handler, void* argv);

//销毁线程池
void zoey_threadpool_destroy(zoey_threadpool_t *pool);

//增加一个线程
int zoey_thread_add(zoey_threadpool_t *pool);
//更改最大任务限制
void zoey_set_max_tasknum(zoey_threadpool_t *pool,unsigned int num);
#endif
