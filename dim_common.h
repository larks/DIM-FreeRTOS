#ifndef __COMMONDEFS
#define __COMMONDEFS

/* Service type definition */
/* ONCE_ONLY??*/
#ifndef ONCE_ONLY 
#define ONCE_ONLY	0x01
#define TIMED		0x02
#define MONITORED	0x04
#define COMMAND		0x08
#define DIM_DELETE	0x10
#define MONIT_ONLY	0x20
#define UPDATE 		0x40
#define TIMED_ONLY	0x80
#define MONIT_FIRST 0x100
#define MAX_TYPE_DEF    0x100
#define STAMPED       0x1000

typedef enum { SRC_NONE, SRC_DIS, SRC_DIC, SRC_DNS, SRC_DNA, SRC_USR }SRC_TYPES;

#include <sys/types.h> 
typedef int64_t	longlong;
typedef long dim_long;

#endif

#ifndef _DIM_PROTO
	#define _DIM_PROTO(func,param)	func ()
	#define _DIM_PROTOE(func,param) _DIM_PROTO(func,param)
	#define DllExp
#endif

#ifndef NOTHREADS
	#define NOTHREADS
#endif

#ifdef NOTHREADS
	#ifndef DIMLIB
		#ifndef sleep
			#define sleep(t) dtq_sleep(t)
		#endif
	#endif
#endif

#ifdef __unix__
	#include <signal.h>
	#include <unistd.h>

	extern int DIM_Threads_OFF;

	#define DISABLE_SIG     sigset_t set, oset; if (DIM_Threads_OFF) {\
													sigemptyset(&set);\
													sigaddset(&set,SIGIO);\
													sigaddset(&set,SIGALRM);\
													sigprocmask(SIG_BLOCK,&set,&oset);}
	#define ENABLE_SIG       if (DIM_Threads_OFF) {\
													sigprocmask(SIG_SETMASK,&oset,0);}

	/*
	#define DISABLE_SIG     sigset_t set, oset; sigemptyset(&set);\
							sigaddset(&set,SIGIO);\
							sigaddset(&set,SIGALRM);\
							sigprocmask(SIG_BLOCK,&set,&oset);
	#define ENABLE_SIG      sigprocmask(SIG_SETMASK,&oset,0);
	*/

	#define DISABLE_AST     DISABLE_SIG DIM_LOCK
	#define ENABLE_AST      DIM_UNLOCK ENABLE_SIG
	
	#ifdef VxWorks
	#define DIM_LOCK taskLock();
	#define DIM_UNLOCK taskUnlock();
	#else

	#ifndef NOTHREADS
	#include <pthread.h>

	_DIM_PROTOE( void dim_lock,		() );
	_DIM_PROTOE( void dim_unlock,	() );
	_DIM_PROTOE( void dim_wait_cond,		() );
	_DIM_PROTOE( void dim_signal_cond,	() );

	#define DIM_LOCK 	dim_lock();
	#define DIM_UNLOCK	dim_unlock();

	#else
	#include <time.h>
	#define DIM_LOCK
	#define DIM_UNLOCK
	#endif
	#endif
#endif

#ifdef FreeRTOS
	#include <signal.h>
	#include <unistd.h>
	
	extern int DIM_Threads_OFF;
	/* Not sure if the following is fine... */
	#define DISABLE_SIG     sigset_t set, oset; if (DIM_Threads_OFF) {\
												sigemptyset(&set);\
												sigaddset(&set,SIGIO);\
												sigaddset(&set,SIGALRM);\
												sigprocmask(SIG_BLOCK,&set,&oset);}
	#define ENABLE_SIG       if (DIM_Threads_OFF) {\
													sigprocmask(SIG_SETMASK,&oset,0);}
	
	
	#define DIM_LOCK taskENTER_CRITICAL(); /* Do I need to disable interrupts? */
	#define DIM_UNLOCK taskEXIT_CRITICAL();
#endif


#ifdef OSK
#define INC_LEVEL               1
#define DEC_LEVEL               (-1)
#define DISABLE_AST     sigmask(INC_LEVEL);
#define ENABLE_AST      sigmask(DEC_LEVEL);
#endif


_DIM_PROTOE( int id_get,           (void *ptr, int type) );
_DIM_PROTOE( void id_free,         (int id, int type) );
_DIM_PROTOE( void *id_get_ptr,     (int id, int type) );

_DIM_PROTOE( unsigned int dtq_sleep,	(unsigned int secs) );
_DIM_PROTOE( void dtq_start_timer,      (int secs, void(*rout)(void*), void *tag) );
_DIM_PROTOE( int dtq_stop_timer,		(void *tag) );
_DIM_PROTOE( void dim_init,				() );
_DIM_PROTOE( void dim_no_threads,		() );
_DIM_PROTOE( void dna_set_test_write,	(int conn_id, int time) );
_DIM_PROTOE( void dna_rem_test_write,	(int conn_id) );
_DIM_PROTOE( int dim_set_dns_node,		(char *node) );
_DIM_PROTOE( int dim_get_dns_node,		(char *node) );
_DIM_PROTOE( int dim_set_dns_port,		(int port) );
_DIM_PROTOE( int dim_get_dns_port,		() );
_DIM_PROTOE( void dic_set_debug_on,		() );
_DIM_PROTOE( void dic_set_debug_off,	() );
_DIM_PROTOE( void dim_print_msg,		(char *msg, int severity) );
_DIM_PROTOE( void dim_print_date_time,		() );
_DIM_PROTOE( void dim_set_write_timeout,		(int secs) );
_DIM_PROTOE( int dim_get_write_timeout,		() );
_DIM_PROTOE( void dim_usleep,	(unsigned int t) );
_DIM_PROTOE( int dim_wait,		(void) );
_DIM_PROTOE( int dim_get_priority,		(int dim_thread, int prio) );
_DIM_PROTOE( int dim_set_priority,		(int dim_thread, int *prio) );
_DIM_PROTOE( int dim_set_scheduler_class,		(int sched_class) );
_DIM_PROTOE( int dim_get_scheduler_class,		(int *sched_class) );
_DIM_PROTOE( dim_long dim_start_thread,    (void(*rout)(void*), void *tag) );
_DIM_PROTOE( int dic_set_dns_node,		(char *node) );
_DIM_PROTOE( int dic_get_dns_node,		(char *node) );
_DIM_PROTOE( int dic_set_dns_port,		(int port) );
_DIM_PROTOE( int dic_get_dns_port,		() );
_DIM_PROTOE( int dis_set_dns_node,		(char *node) );
_DIM_PROTOE( int dis_get_dns_node,		(char *node) );
_DIM_PROTOE( int dis_set_dns_port,		(int port) );
_DIM_PROTOE( int dis_get_dns_port,		() );
_DIM_PROTOE( void dim_stop,				() );
_DIM_PROTOE( int dim_stop_thread,		(dim_long tid) );
_DIM_PROTOE( dim_long dis_add_dns,		(char *node, int port) );
_DIM_PROTOE( dim_long dic_add_dns,		(char *node, int port) );
_DIM_PROTOE( int dim_get_env_var,		(char *env_var, char *value, int value_size) );
_DIM_PROTOE( int dim_set_write_buffer_size,		(int bytes) );
_DIM_PROTOE( int dim_get_write_buffer_size,		() );
_DIM_PROTOE( int dim_set_read_buffer_size,		(int bytes) );
_DIM_PROTOE( int dim_get_read_buffer_size,		() );
_DIM_PROTOE( void dis_set_debug_on,		() );
_DIM_PROTOE( void dis_set_debug_off,	() );
_DIM_PROTOE( void dim_set_keepalive_timeout,		(int secs) );
_DIM_PROTOE( int dim_get_keepalive_timeout,		() );
_DIM_PROTOE( void dim_set_listen_backlog,		(int size) );
_DIM_PROTOE( int dim_get_listen_backlog,		() );

#ifdef WIN32
#define getpid _getpid
_DIM_PROTOE( void dim_pause,		() );
_DIM_PROTOE( void dim_wake_up,	() );
_DIM_PROTOE( void dim_lock,		() );
_DIM_PROTOE( void dim_unlock,	() );
_DIM_PROTOE( void dim_sleep,	(unsigned int t) );
_DIM_PROTOE( void dim_win_usleep,	(unsigned int t) );
#define sleep(t)	dim_sleep(t);
#define usleep(t)	dim_win_usleep(t);
#define pause() 	dim_pause();
#define wake_up()	dim_wake_up();
#define DIM_LOCK 	dim_lock();
#define DIM_UNLOCK	dim_unlock();
#define DISABLE_AST	DIM_LOCK
#define ENABLE_AST  DIM_UNLOCK
#endif

_DIM_PROTOE( void dim_print_date_time_millis,		() );

/* ctime usage */
#if defined (solaris) || (defined (LYNXOS) && !defined (__Lynx__) )
#define my_ctime(t,str,size) ctime_r(t,str,size)
#else 
#if defined (__linux__) || defined (__Lynx__)
#define my_ctime(t,str,size) ctime_r(t,str)
#else
#define my_ctime(t,str,size) strcpy(str,(const char *)ctime(t))
#endif
#endif

/* DIM Error Severities*/
typedef enum { DIM_INFO, DIM_WARNING, DIM_ERROR, DIM_FATAL }DIM_SEVERITIES;
/* DIM Error codes */
#define DIMDNSUNDEF 0x1		/* DIM_DNS_NODE undefined			FATAL */
#define DIMDNSREFUS 0x2		/* DIM_DNS refuses connection		FATAL */
#define DIMDNSDUPLC 0x3		/* Service already exists in DNS	FATAL */
#define DIMDNSEXIT  0x4		/* DNS requests server to EXIT		FATAL */
#define DIMDNSTMOUT 0x5		/* Server failed sending Watchdog	WARNING */

#define DIMSVCDUPLC 0x10	/* Service already exists in Server	ERROR */
#define DIMSVCFORMT 0x11	/* Bat format string for service	ERROR */
#define DIMSVCINVAL 0x12	/* Service ID invalid				ERROR */
#define DIMSVCTOOLG 0x13	/* Service name too long			ERROR */

#define DIMTCPRDERR	0x20	/* TCP/IP read error				ERROR */
#define DIMTCPWRRTY	0x21	/* TCP/IP write	error - Retrying	WARNING */
#define DIMTCPWRTMO	0x22	/* TCP/IP write error - Disconnect	ERROR */
#define DIMTCPLNERR	0x23	/* TCP/IP listen error				ERROR */
#define DIMTCPOPERR	0x24	/* TCP/IP open server error			ERROR */
#define DIMTCPCNERR	0x25	/* TCP/IP connection error			ERROR */
#define DIMTCPCNEST	0x26	/* TCP/IP connection established	INFO */

#define DIMDNSCNERR	0x30	/* Connection to DNS failed			ERROR */
#define DIMDNSCNEST	0x31	/* Connection to DNS established	INFO */
		
#endif                         







