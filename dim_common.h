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
#define ENOSYS 89 /* Function not implemented (from errno.h) */
#endif

/* #ifdef FreeRTOS: */
#include <unistd.h>
#include <time.h>

/* Not sure if the following is fine... */
#define DISABLE_SIG     ENOSYS
#define ENABLE_SIG      ENOSYS

#define DIM_LOCK taskENTER_CRITICAL(); /* Do I need to disable interrupts? */
#define DIM_UNLOCK taskEXIT_CRITICAL();

#define DISABLE_AST DIM_LOCK /* smør på flesk */
#define ENABLE_AST DIM_UNLOCK

int id_get(void *ptr, int type);
void id_free(int id, int type);
void *id_get_ptr(int id, int type);

unsigned int dtq_sleep(unsigned int secs);
void dtq_start_timer(int secs, void(*rout)(void*), void *tag);
int dtq_stop_timer(void *tag);
void dim_init(void);
void dim_no_threads(void);
void dna_set_test_write(int conn_id, int time);
void dna_rem_test_write(int conn_id);
int dim_set_dns_node(char *node);
int dim_get_dns_node(char *node);
int dim_set_dns_port(int port);
int dim_get_dns_port(void);
void dic_set_debug_on(void);
void dic_set_debug_off(void);
void dim_print_msg(char *msg, int severity);
void dim_print_date_time(void);
void dim_set_write_timeout(int secs);
int dim_get_write_timeout(void);
void dim_usleep(unsigned int t);
int dim_wait(void);
int dim_get_priority(int dim_thread, int prio);
int dim_set_priority(int dim_thread, int *prio);
int dim_set_scheduler_class(int sched_class);
int dim_get_scheduler_class(int *sched_class);
dim_long dim_start_thread(void(*rout)(void*), void *tag);
int dic_set_dns_node(char *node);
int dic_get_dns_node(char *node);
int dic_set_dns_port(int port);
int dic_get_dns_port(void);
int dis_set_dns_node(char *node);
int dis_get_dns_node(char *node);
int dis_set_dns_port(int port);
int dis_get_dns_port(void);
void dim_stop(void);
int dim_stop_thread(dim_long tid);
dim_long dis_add_dns(char *node, int port);
dim_long dic_add_dns(char *node, int port);
int dim_get_env_var(char *env_var, char *value, int value_size);
int dim_set_write_buffer_size(int bytes);
int dim_get_write_buffer_size(void);
int dim_set_read_buffer_size(int bytes);
int dim_get_read_buffer_size(void);
void dis_set_debug_on(void);
void dis_set_debug_off(void);
void dim_set_keepalive_timeout(int secs);
int dim_get_keepalive_timeout(void);
void dim_set_listen_backlog(int size);
int dim_get_listen_backlog(void);

void dim_print_date_time_millis(void);

/* ctime usage */
#define my_ctime(t,str,size) strcpy(str,(const char *)ctime(t))

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


#endif /* ifndef __COMMONDEFS */

