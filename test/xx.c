#include <stdio.h>
/*
#include <pthread.h>
*/
#include <thread.h>
#include <signal.h>
#include <dim.h>
#include <dic.h>

#define NUM_THREADS 10

void *change_global_data(void *);     /*  for thr_create()   */


main(int argc,char * argv[])  
{
	int i=0;
	
       	dim_init_threads();
	for (i=0; i< NUM_THREADS; i++)     {
	  thr_create(NULL, 0, change_global_data, (void *)i, 0, NULL);
	}
	
	while(thr_join(NULL, NULL, NULL));
}

rout(int *tag, char *str, int *size)
{
  printf("Thread %d received %s\n",*tag, str);
}

void *change_global_data(void *p)   
{
	static mutex_t Global_mutex;
	char serv_name[80];	
	int par;
	
	par = (int)p;
	/*
       	mutex_lock(&Global_mutex);
	*/
	sprintf(serv_name,"xx/Service_%03d",par);
	dic_info_service(serv_name,TIMED,10,0,0,rout,par,"no_link",8);
	printf("Thread %d requested service %s\n",par,serv_name);
	/*
	mutex_unlock(&Global_mutex);
	*/
       	while(1)
	  pause();
}

