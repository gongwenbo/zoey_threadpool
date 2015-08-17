/*
 *created on 2015.8.13 by cool
 */

#include "zoey_threadpool.h"
#include <signal.h>

#define MAX_TASK_SIZE 99999999
static 	pthread_key_t  key;

int z_conf_check(zoey_threadpool_conf_t *conf);
inline void z_task_queue_init(zoey_task_queue_t* task_queue);
int z_thread_mutex_create(pthread_mutex_t *mutex);
inline void z_thread_mutex_destroy(pthread_mutex_t *mutex);
inline int z_thread_cond_create(pthread_cond_t *cond);
inline void z_thread_cond_destroy(pthread_cond_t *cond);
int z_threadpool_create(zoey_threadpool_t *pool);
void z_threadpool_cycle(void* argv);
void  z_threadpool_exit_cb(void* argv);
inline int z_thread_add(zoey_threadpool_t *pool);
inline void z_change_maxtask_num(zoey_threadpool_t *pool, unsigned int num);
inline int z_thread_key_create();
inline void z_thread_key_destroy();

int z_conf_check(zoey_threadpool_conf_t *conf)
{
	if (conf == NULL){
		return -1;
	}

	if (conf->threadnum < 1){
		return -1;
	}

	if (conf->maxtasknum < 1){
		conf->maxtasknum = MAX_TASK_SIZE;
	}
	return 0;
}


inline void  z_task_queue_init(zoey_task_queue_t* task_queue)
{
	task_queue->head = NULL;
	task_queue->tail = &task_queue->head;
}

int z_thread_mutex_create(pthread_mutex_t *mutex)
{
	int ret = 0;
	pthread_mutexattr_t attr;

	if (pthread_mutexattr_init(&attr) != 0){
		return -1;
	}

	if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK) != 0){
		pthread_mutexattr_destroy(&attr);
		return -1;
	}

	ret = pthread_mutex_init(mutex,&attr);

	pthread_mutexattr_destroy(&attr);

	return ret;
}

inline  z_thread_mutex_destroy(pthread_mutex_t *mutex)
{
	pthread_mutex_destroy(mutex);
}

inline int z_thread_cond_create(pthread_cond_t *cond)
{
	return pthread_cond_init(cond, NULL);
}

inline void z_thread_cond_destroy(pthread_cond_t *cond)
{
	pthread_cond_destroy(cond);
}


int z_threadpool_create(zoey_threadpool_t *pool)
{
	int i = 0;
	pthread_t  pid;
	pthread_attr_t attr;

	if (pthread_attr_init(&attr) != 0){
		return -1;
	}

	if (pool->thread_stack_size != 0)
	{
		if (pthread_attr_setstacksize(&attr, pool->thread_stack_size) != 0){
			pthread_attr_destroy(&attr);
			return -1;
		}
	}

	for (; i < pool->threadnum; ++i)
	{
		pthread_create(&pid, &attr, z_threadpool_cycle,pool);
	}	
	pthread_attr_destroy(&attr);
	return 0;
}



int z_thread_add(zoey_threadpool_t *pool)
{
	pthread_t  pid;
	pthread_attr_t attr;
	int ret = 0;
	if (pthread_attr_init(&attr) != 0){
		return -1;
	}
	if (pool->thread_stack_size != 0)
	{
		if (pthread_attr_setstacksize(&attr, pool->thread_stack_size) != 0){
			pthread_attr_destroy(&attr);
			return -1;
		}
	}
	ret = pthread_create(&pid, &attr, z_threadpool_cycle,pool);
	if (ret == 0)
	{
		pool->threadnum++;
	}
	pthread_attr_destroy(&attr);
	return ret;
}


inline void z_change_maxtask_num(zoey_threadpool_t *pool, unsigned int num)
{
	pool->tasks.maxtasknum = num;
	if (pool->tasks.maxtasknum < 1)
	{
		pool->tasks.maxtasknum = MAX_TASK_SIZE;
	}
}

//工作线程
void z_threadpool_cycle(void* argv)
{
	unsigned int exit_flag = 0;
	sigset_t set;
	zoey_task_t *ptask = NULL;
	zoey_threadpool_t *pool = (zoey_threadpool_t*)argv;

	//只注册以下致命信号，其他全部屏蔽
	sigfillset(&set);
	sigdelset(&set, SIGILL);
	sigdelset(&set, SIGFPE);
	sigdelset(&set, SIGSEGV);
	sigdelset(&set, SIGBUS);
	
	if (pthread_setspecific(key,(void*)&exit_flag) != 0){
		return;
	}
	if (pthread_sigmask(SIG_BLOCK, &set, NULL) != 0){
		return;
	}
	while(!exit_flag){
		if (pthread_mutex_lock(&pool->mutex) != 0){  //加锁
			return;
		}

		while(pool->tasks.head == NULL){
			if (pthread_cond_wait(&pool->cond, &pool->mutex) != 0){
				pthread_mutex_unlock(&pool->mutex);
				return;
			}
		}
		
		ptask = pool->tasks.head;     //从任务队列中获取一个任务任务节点
		pool->tasks.head = ptask->next;

		pool->tasks.curtasknum--;   //当前任务数--

		if (pool->tasks.head == NULL){
			pool->tasks.tail = &pool->tasks.head;
		}

		if (pthread_mutex_unlock(&pool->mutex) != 0){ //解锁
			return;
		}

		ptask->handler(ptask->argv);  //执行任务，一般任务函数必须返回0    其他值导致线程退出。
		free(ptask);
		ptask = NULL;
	}
	pthread_exit(0);
}


//线程池退出函数  
void z_threadpool_exit_cb(void* argv)
{
	unsigned int *lock = argv;
	unsigned int *pexit_flag = NULL;
	pexit_flag = (int *)pthread_getspecific(key);
	*pexit_flag = 1;
	pthread_setspecific(key, (void*)pexit_flag);
	*lock = 0;
}

inline int z_thread_key_create()
{
	return pthread_key_create(&key, NULL);
}

inline void z_thread_key_destroy()
{
	pthread_key_delete(key);
}
