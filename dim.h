#ifndef __DIMDEFS
#define __DIMDEFS
/*
 * DNA (Delphi Network Access) implements the network layer for the DIM
 * (Delphi Information Managment) System.
 *
 * Started           : 10-11-91
 * Last modification : 03-08-94
 * Written by        : C. Gaspar
 * Adjusted by       : G.C. Ballintijn
 *
 * $Id: dim.h,v 1.1 1994/07/03 15:20:47 gerco Exp gerco $
 */

#ifndef OSK
#	define register
#	ifdef _OSK
#		define OSK
#	endif
#endif

#ifndef _PROTO
#ifndef OSK		/* Temorary hack */
#	if (__STDC__ == 1)|| defined(_ANSI_EXT) /* || defined(ultrix) */
#		define	_PROTO(func,param)	func param
#	else
#		define _PROTO(func,param)	func ()
#	endif
#else
#	define _PROTO(func,param)	func ()
#endif
#endif

#define MY_LITTLE_ENDIAN	0x1
#define MY_BIG_ENDIAN 		0x2

#define VAX_FLOAT		0x10
#define IEEE_FLOAT 		0x20
#define AXP_FLOAT		0x30

#define MY_OS9			0x100

#ifdef VMS
#include <ssdef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <starlet.h>
#include <time.h>
#define NOSHARE noshare
#define DISABLE_AST     long int ast_enable = sys$setast(0);
#define ENABLE_AST      if (ast_enable == SS$_WASSET) sys$setast(1);
#define	vtohl(l)	(l)
#define	htovl(l)	(l)
#ifdef __alpha
#define MY_FORMAT MY_LITTLE_ENDIAN+AXP_FLOAT
#else
#define MY_FORMAT MY_LITTLE_ENDIAN+VAX_FLOAT
#endif
#endif

#ifdef unix
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#ifdef VxWorks
#include <sigLib.h>
#endif
#define NOSHARE 
#define DISABLE_AST     sigset_t set, oset;sigemptyset(&set);sigaddset(&set,SIGIO);sigaddset(&set,SIGALRM);sigprocmask(SIG_BLOCK,&set,&oset);
#define ENABLE_AST      sigprocmask(SIG_SETMASK,&oset,0);
#ifdef MIPSEL
#define	vtohl(l)	(l)
#define	htovl(l)	(l)
#define MY_FORMAT MY_LITTLE_ENDIAN+IEEE_FLOAT
#endif
#ifdef MIPSEB
#define	vtohl(l)	_swapl(l)
#define	htovl(l)	_swapl(l)
#define	vtohs(s)	_swaps(s)
#define	htovs(s)	_swaps(s)
/*
#ifndef VxWorks
*/
#define MY_FORMAT MY_BIG_ENDIAN+IEEE_FLOAT
/*
#else
#define MY_FORMAT MY_BIG_ENDIAN+IEEE_FLOAT+MY_OS9
#endif
*/
#endif
_PROTO( int _swapl,  (int l) );
_PROTO( short _swaps,   (short s) );
#ifdef solaris
#include <thread.h>
mutex_t Global_DIM_mutex;
#define DIM_LOCK mutex_lock(&Global_DIM_mutex);
#define DIM_UNLOCK mutex_unlock(&Global_DIM_mutex);
#else
#ifdef VxWorks
#define DIM_LOCK taskLock();
#define DIM_UNLOCK taskUnlock();
#else
#define DIM_LOCK 
#define DIM_UNLOCK
#endif
#endif
#endif

#ifdef OSK
#include <types.h>
#ifndef _UCC
#include <machine/types.h>
#else
#define register
#endif
#include <inet/in.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#define NOSHARE 
#define INC_LEVEL               1
#define DEC_LEVEL               (-1)
#define DISABLE_AST     sigmask(INC_LEVEL);
#define ENABLE_AST      sigmask(DEC_LEVEL);
#define	vtohl(l)	_swapl(l)
#define	htovl(l)	_swapl(l)
#define	vtohs(s)	_swaps(s)
#define	htovs(s)	_swaps(s)
#define MY_FORMAT MY_BIG_ENDIAN+IEEE_FLOAT+MY_OS9
typedef unsigned short	ushort;
_PROTO( char *getenv,  (char *name) );
_PROTO( void *malloc,  (unsigned size) );
_PROTO( void *realloc, (void *ptr, unsigned size) );
_PROTO( int _swapl,   (int l) );
_PROTO( short _swaps,   (short s) );
#endif


#define	TRUE	1
#define	FALSE	0


#define DNS_TASK	"DIM_DNS"
#define DNS_PORT	2505			/* Name server port          */
#define SEEK_PORT	0			/* server should seek a port */

#define MIN_BIOCNT	 	50
#ifdef OSK
#define DIS_DNS_TMOUT_MIN	5
#define DIS_DNS_TMOUT_MAX	10
#define DIC_DNS_TMOUT_MIN	5
#define DIC_DNS_TMOUT_MAX	10
#define MAX_SERVICE_UNIT 	32
#define CONN_BLOCK		32
#define MAX_CONNS		32
#define ID_BLOCK		64
#define TCP_RCV_BUF_SIZE	4096
#define TCP_SND_BUF_SIZE	4096
#else
#define DIS_DNS_TMOUT_MIN	30
#define DIS_DNS_TMOUT_MAX	60
#define DIC_DNS_TMOUT_MIN	60
#define DIC_DNS_TMOUT_MAX	120
#define MAX_SERVICE_UNIT 	100
#define CONN_BLOCK		256
#define MAX_CONNS		256
#define ID_BLOCK		512
#define TCP_RCV_BUF_SIZE	16384
#define TCP_SND_BUF_SIZE	16384
#endif
#define DID_DNS_TMOUT_MIN	5
#define DID_DNS_TMOUT_MAX	10
/*
#define WATCHDOG_TMOUT_MIN	120
#define WATCHDOG_TMOUT_MAX	180
*/
#define WATCHDOG_TMOUT_MIN	15
#define WATCHDOG_TMOUT_MAX	25
#define MAX_NODE_NAME		40
#define MAX_TASK_NAME		40
#define MAX_NAME 		132
#define MAX_CMND 		16384
/*
#define MAX_IO_DATA 	65535
*/
#define MAX_IO_DATA		(TCP_SND_BUF_SIZE - 16)

typedef enum { SRC_NONE, SRC_DIS, SRC_DIC, SRC_DNS, SRC_DNA }SRC_TYPES;
typedef enum { DNS_DIS_REGISTER, DNS_DIS_KILL}DNS_DIS_TYPES;
typedef enum { RD_HDR, RD_DATA, RD_DUMMY } CONN_STATE;
typedef enum { NOSWAP, SWAPS, SWAPL, SWAPD} SWAP_TYPE;

#define DECNET			0		/* Decnet as transport layer */
#define TCPIP			1		/* Tcpip as transport layer  */
#define BOTH			2		/* Both protocols allowed    */

#define	STA_DISC		(-1)		/* Connection lost           */
#define	STA_DATA		0		/* Data received             */
#define	STA_CONN		1		/* Connection made           */

#define	START_PORT_RANGE	5000		/* Lowest port to use        */
#define	STOP_PORT_RANGE		6000		/* Highest port to use       */
#define	TEST_TIME_OSK		10		/* Interval to test conn.    */
#define	TEST_TIME_VMS		180		/* Interval to test conn.    */
#define	TEST_WRITE_TAG		25		/* DTQ tag for test writes   */

#define	OPN_MAGIC		0xc0dec0de	/* Magic value 1st packet    */
#define	HDR_MAGIC		0xfeadfead	/* Magic value in header     */
#define	TST_MAGIC		0x11131517	/* Magic value, test write   */
#define	TRP_MAGIC		0x71513111	/* Magic value, test reply   */

/* String Format */

typedef struct{
	short par_num;
	char par_bytes;
	char flags;     /* bits 0-1 is type of swap, bit 4 id float conversion */
}FORMAT_STR;

/* Packet sent by the client to the server inside DNA */
typedef struct{
	int code;
	char node[MAX_NODE_NAME];
	char task[MAX_TASK_NAME];
} DNA_NET;

/* Packet sent by the client to the server */
typedef struct{
	int size;
	char service_name[MAX_NAME];
	int service_id;
	int type;
	int timeout;
	int format;
	int buffer[1];
} DIC_PACKET;

#define DIC_HEADER		(MAX_NAME + 20)

/* Packet sent by the server to the client */
typedef struct{
	int size;
	int service_id;
	int buffer[1];
} DIS_PACKET;

#define DIS_HEADER		8

/* Packet sent by the server to the name_server */
typedef struct{
	char service_name[MAX_NAME];
	int service_id;
	char service_def[MAX_NAME];
} SERVICE_REG;
	
typedef struct{
	int size;
	SRC_TYPES src_type;
	char node_name[MAX_NODE_NAME];
	char task_name[MAX_TASK_NAME];
	int pid;
	int port;
	int protocol;
	int format;
	int n_services;
	SERVICE_REG services[MAX_SERVICE_UNIT];
} DIS_DNS_PACKET;

#define DIS_DNS_HEADER		(MAX_NODE_NAME + MAX_TASK_NAME + 28) 

/* Packet sent by the name_server to the server */
typedef struct {
	int size;
	int type;
} DNS_DIS_PACKET;

#define DNS_DIS_HEADER		8

/* Packet sent by the client to the name_server */
typedef struct{
	char service_name[MAX_NAME];
	int service_id;
} SERVICE_REQ;
	
typedef struct{
	int size;
	SRC_TYPES src_type;
	SERVICE_REQ service;
} DIC_DNS_PACKET;

#define DIC_DNS_HEADER		12

/* Packet sent by the name_server to the client */
typedef struct {
	int size;
	int service_id;
	char service_def[MAX_NAME];
	char node_name[MAX_NODE_NAME];
	char task_name[MAX_TASK_NAME];
	int pid;
	int port;
	int protocol;
	int format;
} DNS_DIC_PACKET;

#define DNS_DIC_HEADER		(MAX_NODE_NAME + MAX_TASK_NAME + MAX_NAME + 24) 

typedef struct {
	char name[MAX_NAME];
	char node[MAX_NODE_NAME];
	char task[MAX_TASK_NAME];
	int type;
	int status;
	int n_clients;
} DNS_SERV_INFO;

typedef struct {
	char name[MAX_NAME];
	int type;
	int status;
	int n_clients;
} DNS_SERVICE_INFO;

typedef struct {
	char node[MAX_NODE_NAME];
	char task[MAX_TASK_NAME];
	int pid;
	int n_services;
} DNS_SERVER_INFO;

typedef struct {
	DNS_SERVER_INFO server;
	DNS_SERVICE_INFO services[1];
} DNS_DID;

typedef struct {
	char node[MAX_NODE_NAME];
	char task[MAX_TASK_NAME];
} DNS_CLIENT_INFO;

typedef struct {
	int header_size;
	int data_size;
	int header_magic;
} DNA_HEADER;

/* Connection handling */

typedef struct timer_entry{
	struct timer_entry *next;
	struct timer_entry *next_done;
	int time;
	int time_left;
	void (*user_routine)();
	int tag;
} TIMR_ENT;

typedef struct {
	int busy;
	void (*read_ast)();
	int *buffer;
	int buffer_size;
	char *curr_buffer;
	int curr_size;
	int full_size;
	int protocol;
	CONN_STATE state;
	int writing;
	int saw_init;
} DNA_CONNECTION;

extern NOSHARE DNA_CONNECTION *Dna_conns;

typedef struct {
	int channel;
	int mbx_channel;
	void (*read_rout)();
	char *buffer;
	int size;
	unsigned short *iosb_r;
	unsigned short *iosb_w;
	char node[MAX_NODE_NAME];
	char task[MAX_TASK_NAME];
	int port;
	int reading;
	int timeout;
	TIMR_ENT *timr_ent;
	time_t last_used;
} NET_CONNECTION;
 
extern NOSHARE NET_CONNECTION *Net_conns;

typedef struct {
	char node_name[MAX_NODE_NAME];
	char task_name[MAX_TASK_NAME];
	char *service_head;
} DIC_CONNECTION;

extern NOSHARE DIC_CONNECTION *Dic_conns;

typedef struct {
	SRC_TYPES src_type;
	char node_name[MAX_NODE_NAME];
	char task_name[MAX_TASK_NAME];
	int pid;
	int port;
	char *service_head;
	char *node_head;
	int protocol;
	int validity;
	int n_services;
	TIMR_ENT *timr_ent;
} DNS_CONNECTION;

extern NOSHARE DNS_CONNECTION *Dns_conns;

extern NOSHARE int Curr_N_Conns;


/* PROTOTYPES */


/* DNA */
_PROTO( int dna_start_read,    (int conn_id, int size) );
_PROTO( void dna_test_write,   (int conn_id) );
_PROTO( int dna_write,         (int conn_id, void *buffer, int size) );
_PROTO( int dna_write_nowait,  (int conn_id, void *buffer, int size) );
_PROTO( int dna_open_server,   (char *task, void (*read_ast)(), int *protocol,
				int *port) );
_PROTO( int dna_get_node_task, (int conn_id, char *node, char *task) );
_PROTO( int dna_open_client,   (char *server_node, char *server_task, int port,
                                int server_protocol, void (*read_ast)()) );
_PROTO( int dna_close,         (int conn_id) );
_PROTO( void dna_report_error, (int conn_id, int code, char *routine_name) );


/* TCPIP */
_PROTO( int tcpip_open_client,     (int conn_id, char *node, char *task,
                                    int port) );
_PROTO( int tcpip_open_server,     (int conn_id, char *task, int *port) );
_PROTO( int tcpip_open_connection, (int conn_id, int channel) );
_PROTO( int tcpip_start_read,      (int conn_id, char *buffer, int size,
                                    void (*ast_routine)()) );
_PROTO( int tcpip_start_listen,    (int conn_id, void (*ast_routine)()) );
_PROTO( int tcpip_write,           (int conn_id, char *buffer, int size) );
_PROTO( void tcpip_get_node_task,  (int conn_id, char *node, char *task) );
_PROTO( int tcpip_close,           (int conn_id) );
_PROTO( int tcpip_failure,         (int code) );
_PROTO( void tcpip_report_error,   (int code) );


/* DTQ */
_PROTO( void print_date_time,    (void) );
_PROTO( int dtq_create,          (void) );
_PROTO( int dtq_delete,          (int queue_id) );
_PROTO( TIMR_ENT *dtq_add_entry, (int queue_id, int time,
                                  void (*user_routine)(), int tag) );
_PROTO( int dtq_clear_entry,     (TIMR_ENT *entry) );
_PROTO( int dtq_rem_entry,       (int queue_id, TIMR_ENT *entry) );
_PROTO( void dtq_start_timer,    (int time, void (*user_routine)(), int tag) );
_PROTO( void dtq_stop_timer,     (int tag) );
                         

/* UTIL */
typedef struct dll {
	struct dll *next;
	struct dll *prev;
	char user_info[1];
} DLL;

typedef struct sll {
	struct sll *next;
	char user_info[1];
} SLL;


_PROTO( void DimDummy,        () );     
_PROTO( void conn_arr_create, (SRC_TYPES type) );
_PROTO( int conn_get,         (void) );
_PROTO( int conn_free,        (int conn_id) );
_PROTO( void *arr_increase,   (void *conn_ptr, int conn_size, int n_conns) );
_PROTO( void id_arr_create,   () );
_PROTO( int id_get,           (void *ptr) );
_PROTO( void id_free,         (int id) );
_PROTO( void *id_get_ptr,     (int id) );
_PROTO( void *id_arr_increase,(void *id_ptr, int id_size, int n_ids) );

_PROTO( void dll_init,         ( DLL *head ) );
_PROTO( void dll_insert_queue, ( DLL *head, DLL *item ) );
_PROTO( DLL *dll_search,       ( DLL *head, char *data, int size ) );
_PROTO( DLL *dll_get_next,     ( DLL *head, DLL *item ) );
_PROTO( int dll_empty,         ( DLL *head ) );
_PROTO( void dll_remove,       ( DLL *item ) );

_PROTO( void sll_init,               ( SLL *head ) );
_PROTO( int sll_insert_queue,        ( SLL *head, SLL *item ) );
_PROTO( SLL *sll_search,             ( SLL *head, char *data, int size ) );
_PROTO( SLL *sll_get_next,           ( SLL *item ) );
_PROTO( int sll_empty,               ( SLL *head ) );
_PROTO( int sll_remove,              ( SLL *head, SLL *item ) );
_PROTO( SLL *sll_remove_head,        ( SLL *head ) );
_PROTO( SLL *sll_search_next_remove, ( SLL *item, int offset, char *data, int size ) );

_PROTO( int get_dns_node_name, ( char *node_name ) );

#define SIZEOF_CHAR 1
#define SIZEOF_SHORT 2
#define SIZEOF_LONG 4
#define SIZEOF_FLOAT 4
#define SIZEOF_DOUBLE 8

#if defined(OSK) && !defined(_UCC)
#	define inc_pter(p,i) (char *)p += (i)
#else
#	define inc_pter(p,i) p = (void *)((char *)p + (i))
#endif

#endif
