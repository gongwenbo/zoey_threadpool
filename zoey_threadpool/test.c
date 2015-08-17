#include<stdio.h>
#include<pthread.h>
#include "zoey_threadpool.h"

int testfun(void *argv)
{
	int *num = (int*)argv;
	printf("testfun threadid = %u  num = %d\n",pthread_self(),*num);
	//sleep(3);
	return 0;
}

int main()
{
	int array[10000] = {0};
	int i = 0;
	zoey_threadpool_conf_t conf = {5,0,5};
	zoey_threadpool_t *pool = zoey_threadpool_init(&conf);
	if (pool == NULL)
	{
		puts("error");
		return 0;
	}
	for (; i < 10000; i++)
	{
		array[i] = i;
		if (i == 80)
		{
			zoey_thread_add(pool);
			zoey_thread_add(pool);
		}
		
		if (i == 100)
		{
			zoey_set_max_tasknum(pool, 0); //0为不做上限
		}
		while(1)
		{
			if (zoey_threadpool_add_task(pool, testfun, &array[i]) == 0)
			{
				break;
			}
			printf("error in i = %d\n",i);
		
		}
	}
	zoey_threadpool_destroy(pool);

	while(1)
	{
		puts("this is main thread");
		sleep(5);
	}
	return 0;
}
