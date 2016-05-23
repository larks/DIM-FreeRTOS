/*
 * DIC (Delphi Information Client) Package implements a library of
 * routines to be used by clients.
 *
 * Started on        : 10-11-91
 * Last modification : 10-08-94
 * Written by        : C. Gaspar
 * Adjusted by       : G.C. Ballintijn
 *
 * $Id: dic.c,v 1.1 1994/07/18 19:50:03 gerco Exp gerco $
 */

#ifdef VMS
#	include <cfortran.h>
#	include <cvtdef.h>
#	include <assert.h>
#endif

#include <dim.h>
#include <dic.h>

/*
#define DEBUG
*/

#ifdef VMS
#define TMP_ENABLE_AST      long int ast_enable = sys$setast(1);
#define TMP_DISABLE_AST     if (ast_enable != SS$_WASSET) sys$setast(0);
#endif

typedef enum {
	NOT_PENDING, WAITING_DNS_UP, WAITING_DNS_ANSWER, WAITING_SERVER_UP,
	DELETED 
} PENDING_STATES;

typedef struct serv {
	struct serv *next;
	struct serv *prev;
	char serv_name[MAX_NAME];
	int serv_id;
	FORMAT_STR format_data[MAX_NAME/4];
	int type;
	int timeout;
	int curr_timeout;
	int *serv_address;
	int serv_size;
	int *fill_address;
	int fill_size;
	void (*user_routine)();
	int tag;
	TIMR_ENT *timer_ent;
	int conn_id;
	PENDING_STATES pending;
	int tmout_done;
        int tid;
} SERVICE;

static SERVICE *Service_pend_head = 0;
static SERVICE *Cmnd_head = 0;
static int Dic_timer_q = 0;
static int Dns_dic_conn_id = 0;
static TIMR_ENT *Dns_dic_timr = NULL;
static TIMR_ENT *Retry_timr_ent = NULL;
static int Tmout_max = 0;
static int Tmout_min = 0;

_PROTO( SERVICE *insert_service, (int type, int timeout, char *name,
				  int *address, int size, void (*routine)(),
				  int tag, int *fill_addr, int fill_size,
				  int pending) );
_PROTO( void modify_service, (SERVICE *servp, int timeout,
				  int *address, int size, void (*routine)(),
				  int tag, int *fill_addr, int fill_size) );
_PROTO( SERVICE *locate_command, (char *serv_name) );
_PROTO( void service_tmout,      (int serv_id) );
_PROTO( static void request_dns_info,      (void) );
_PROTO( static int handle_dns_info,      (DNS_DIC_PACKET *) );
_PROTO( void close_dns_conn,     (void) );
_PROTO( static void release_conn, (int conn_id) );
_PROTO( static void copy_swap_buffer, (FORMAT_STR *format_data, void *buff_out, 
					void *buff_in, int size) );
_PROTO( static void get_format_data, (int format, FORMAT_STR *format_data, 
					char *def) );
_PROTO( static void execute_service,      (DIS_PACKET *packet, 
					SERVICE *servp, int size) );

_PROTO( double _swapd_by_addr, (double *d) );
_PROTO( int _swapl_by_addr, (int *l) );
_PROTO( short _swaps_by_addr, (short *s) );
_PROTO( void _swapd_buffer, (double *dout, double *din, int n) );
_PROTO( void _swapl_buffer, (int *lout, int *lin, int n) );
_PROTO( void _swaps_buffer, (short *sout, short *sin, int n) );

static void recv_rout( conn_id, packet, size, status )
int conn_id, size, status;
DIS_PACKET *packet;
{
	register SERVICE *servp, *auxp;
	register DIC_CONNECTION *dic_connp;
	int service_id, found = 0;

	dic_connp = &Dic_conns[conn_id] ;
	switch( status )
	{
	case STA_DISC:
#ifdef DEBUG
		print_date_time();
		printf("Conn %d, Server %s on node %s disconnected\n",
			conn_id, dic_connp->task_name, dic_connp->node_name);
#endif
		if( !(servp = (SERVICE *) dic_connp->service_head) )
		{
			release_conn( conn_id );
			break;
		}
		while( servp = (SERVICE *) dll_get_next(
					(DLL *) dic_connp->service_head,
				 	(DLL *) servp) )
		{
#ifdef DEBUG
			printf("\t %s was in the service list\n",servp->serv_name);
#endif
/*
	Will be done later by the DNS answer
			service_tmout( servp->serv_id );
*/
			if(!strcmp(dic_connp->task_name,"DIS_DNS"))
				service_tmout( servp->serv_id );
			servp->pending = WAITING_DNS_UP;
			servp->conn_id = 0;
			auxp = servp->prev;
			move_to_notok_service( servp );
			servp = auxp;
		}		
		if( servp = (SERVICE *) Cmnd_head ) {
			while( servp = (SERVICE *) dll_get_next(
							(DLL *) Cmnd_head,
							(DLL *) servp) )
			{
				if( servp->conn_id == conn_id )
				{
#ifdef DEBUG
					printf("\t%s was in the Command list\n", servp->serv_name);
#endif
					auxp = servp->prev;
					if( (servp->type == ONCE_ONLY ) &&
						(servp->pending == WAITING_SERVER_UP))
						service_tmout( servp->serv_id );
					else 
						dic_release_service( servp->serv_id );
					servp = auxp;
				}
			}
		}
		release_conn( conn_id );
		if( !Retry_timr_ent )
			Retry_timr_ent = dtq_add_entry( Dic_timer_q, 2,
							request_dns_info, 0 );
		/* request_dns_info(); */
		break;
	case STA_DATA:
		if( !(SERVICE *) dic_connp->service_head )
			break;
		service_id = vtohl(packet->service_id);
		if(service_id & 0x80000000)  /* Service removed by server */
		{
			service_id &= 0x7fffffff;
			if( servp = (SERVICE *) id_get_ptr(service_id)) 
			{
				service_tmout( servp->serv_id );
				servp->pending = WAITING_DNS_UP;
				servp->conn_id = 0;
				move_to_notok_service( servp );
			}
			if( dll_empty((DLL *)dic_connp->service_head) ) {
				if( servp = (SERVICE *) Cmnd_head ) {
					while( servp = (SERVICE *) dll_get_next(
							(DLL *) Cmnd_head,
							(DLL *) servp) )
					{
						if( servp->conn_id == conn_id)
							found = 1;
					}
				}
				if( !found)
					release_conn( conn_id );
			}
			if( !Retry_timr_ent )
				Retry_timr_ent = dtq_add_entry( Dic_timer_q, 2,
							request_dns_info, 0 );

			break;
		}
		if( servp = (SERVICE *) id_get_ptr(service_id))
		{
			size -= DIS_HEADER;
			execute_service(packet, servp, size);
			if( servp->type == ONCE_ONLY )
			{
/*
				dic_release_service( servp->serv_id );
*/
				servp->pending = NOT_PENDING;
				servp->tmout_done = 0;
				if( servp->timer_ent )
				{
					dtq_rem_entry( Dic_timer_q, servp->timer_ent );
					servp->timer_ent = 0;
				}
			}
			else {
				if( servp->timeout ) {
					if( servp->curr_timeout == servp->timeout )
					{
						dtq_rem_entry( Dic_timer_q, servp->timer_ent );
						servp->curr_timeout = servp->timeout*1.5;
						servp->timer_ent =
						    dtq_add_entry( Dic_timer_q, 
							servp->curr_timeout,
							service_tmout,
							servp->serv_id);
					} else
						dtq_clear_entry( servp->timer_ent );
				}
			}
		}
		break;
	case STA_CONN:
		break;
	default:	dim_panic( "recv_rout(): Bad switch" );
	}
}

static void execute_service(packet, servp, size)
DIS_PACKET *packet;
SERVICE *servp;
int size;
{
	if( servp->serv_address ) 
	{
		if( size > servp->serv_size ) 
			size = servp->serv_size; 
		copy_swap_buffer(servp->format_data, 
						 servp->serv_address, 
						 packet->buffer, size);
		if( servp->user_routine )
			(servp->user_routine)(&servp->tag, &size );
	} 
	else 
	{
		if( servp->user_routine )
		{
			copy_swap_buffer(servp->format_data, 
						 packet->buffer, 
						 packet->buffer, size);
			(servp->user_routine)( &servp->tag, packet->buffer, &size );
		}
	}
}

static void recv_dns_dic_rout( conn_id, packet, size, status )
int conn_id, size, status;
DNS_DIC_PACKET *packet;
{
	int found = 0;
	register SERVICE *servp;

	switch( status )
	{
	case STA_DISC:       /* connection broken */
		servp = Service_pend_head;
		while( servp = (SERVICE *) dll_get_next(
						(DLL *) Service_pend_head,
						(DLL *) servp) )
		{
			if( (servp->pending == WAITING_DNS_ANSWER) ||
			    (servp->pending == WAITING_SERVER_UP))
				servp->pending = WAITING_DNS_UP;
		}
		dna_close( Dns_dic_conn_id );
		Dns_dic_conn_id = 0;
		if( !dll_empty((DLL *) Service_pend_head) )
		{
			Dns_dic_conn_id = open_dns( recv_dns_dic_rout, 
					Tmout_min, Tmout_max );
		}
		break;
	case STA_CONN:        /* connection received */
		Dns_dic_conn_id = conn_id;
		if( !Retry_timr_ent )
			Retry_timr_ent = dtq_add_entry( Dic_timer_q, 1,
						       request_dns_info, 0 );
		/* request_dns_info(); */
		break;
	case STA_DATA:       /* normal packet */
		if( vtohl(packet->size) == DNS_DIC_HEADER )
		{
			handle_dns_info( packet );
		}
		break;
	default:	dim_panic( "recv_dns_dic_rout(): Bad switch" );
	}
}


void service_tmout( serv_id )
int serv_id;
{
	int size = 0;
	register SERVICE *servp;
	
	servp=(SERVICE *)id_get_ptr(serv_id);
	if(servp->tmout_done)
		return;
	servp->tmout_done = 1;
	if( servp->type == UPDATE )
		return;
	if( servp->fill_address )
	{
		size = servp->fill_size;
		if( servp->serv_address )
		{
			if( size > servp->serv_size ) 
				size = servp->serv_size; 
			memcpy(servp->serv_address, servp->fill_address, size);
			if( servp->user_routine )
				(servp->user_routine)( &servp->tag, &size );
		}
		else
		{
			if( servp->user_routine )
				(servp->user_routine)( &servp->tag, servp->fill_address, &size);
		}
	}
	if( servp->type == ONCE_ONLY )
	{
		dic_release_service( servp->serv_id );
/*
		if( servp->timer_ent )
		{
			dtq_rem_entry( Dic_timer_q, servp->timer_ent );
			servp->timer_ent = 0;
		}
*/
	}
	else
	{
		if( servp->timeout )
		{
			if( servp->curr_timeout != servp->timeout )
			{
				dtq_rem_entry( Dic_timer_q, servp->timer_ent );
				servp->curr_timeout = servp->timeout;
				servp->timer_ent = dtq_add_entry( Dic_timer_q, 
							servp->curr_timeout,
							service_tmout,
							servp->serv_id );
			}
		}
	}
}


unsigned dic_info_service( serv_name, req_type, req_timeout, serv_address,
			   serv_size, usr_routine, tag, fill_addr, fill_size )
char *serv_name;
int req_type, req_timeout, serv_size; 
void *serv_address;
void *fill_addr;
int fill_size, tag;
void (*usr_routine)();
{
	register SERVICE *servp;
	int conn_id;
#ifdef vms
/*
	TMP_ENABLE_AST
*/
#endif
	DISABLE_AST
	DIM_LOCK
	/* create a timer queue for timeouts if not yet done */
	if( !Dic_timer_q ) {
		conn_arr_create( SRC_DIC );
		Dic_timer_q = dtq_create();
	}

	/* store_service */
	if(req_type == ONCE_ONLY)
	{
		if( !Cmnd_head ) {
			Cmnd_head = (SERVICE *) malloc(sizeof(SERVICE) );
			dll_init( (DLL *) Cmnd_head );
		}
		if( servp = locate_command(serv_name) ) 
		{
			if( conn_id = servp->conn_id ) 
			{
				modify_service( servp, req_timeout,
					(int *)serv_address, serv_size, usr_routine, tag,
					(int *)fill_addr, fill_size);
				servp->pending = WAITING_SERVER_UP;
				if( send_service(conn_id, servp) )
				  {
				        DIM_UNLOCK
					ENABLE_AST
					return(1);
				  }
			}	
		}
	}
	servp = insert_service( req_type, req_timeout,
			serv_name, (int *)serv_address, serv_size, usr_routine, tag,
			(int *)fill_addr, fill_size, WAITING_DNS_UP );
            
	/* get_address of server from name_server */
	if( !locate_service(servp) )
	  {
		service_tmout( servp->serv_id );
	  }
#ifdef vms
/*
	TMP_DISABLE_AST
*/
#endif
	DIM_UNLOCK
	ENABLE_AST
	return((unsigned) servp->serv_id);
}


int dic_cmnd_service( serv_name, serv_address, serv_size )
char *serv_name;
void *serv_address;
int serv_size;
{
	int conn_id, efn;
	register SERVICE *servp;

	DISABLE_AST
	DIM_LOCK
	/* create a timer queue for timeouts if not yet done */
	if( !Dic_timer_q ) {
		conn_arr_create( SRC_DIC );
		Dic_timer_q = dtq_create();
	}

	/* store_service */
	if( !Cmnd_head ) {
		Cmnd_head = (SERVICE *) malloc(sizeof(SERVICE) );
		dll_init( (DLL *) Cmnd_head );
	}
	if( servp = locate_command(serv_name) ) 
	{
		if( conn_id = servp->conn_id ) 
		{
			servp->fill_address = (int *)serv_address;
			servp->fill_size = serv_size;
			if( send_command(conn_id, servp) )
			  {
			        DIM_UNLOCK
			        ENABLE_AST
				return(1);
			  }
		}	
	}
	servp = insert_service( COMMAND, 0,
				serv_name, 0, 0, 0, 0, (int *)serv_address, serv_size,
				WAITING_DNS_UP );	
	locate_service( servp );
	DIM_UNLOCK
	ENABLE_AST
	
	return(1);
}


SERVICE *insert_service( type, timeout, name, address, size, routine, 
			 tag, fill_addr, fill_size, pending )
int type, *address, size, *fill_addr, fill_size, tag, timeout;
char *name;
int pending;
void (*routine)();
{
	register SERVICE *newp;
	int *fillp;
	int service_id;

	newp = (SERVICE *) malloc(sizeof(SERVICE));
	newp->pending = 0;
	strncpy( newp->serv_name, name, MAX_NAME );
	newp->type = type;
	newp->timeout = timeout;
	newp->serv_address = address;
	newp->serv_size = size;
	newp->user_routine = routine;
	newp->tag = tag;
	fillp = (int *)malloc(fill_size);
	memcpy( (char *) fillp, (char *) fill_addr, fill_size );
	newp->fill_address = fillp;
	newp->fill_size = fill_size;
	newp->conn_id = 0;
	newp->format_data[0].par_bytes = 0;
	newp->next = (SERVICE *)0;
	service_id = id_get((void *)newp);
	newp->serv_id = service_id;
	if( !Service_pend_head )
	{
		Service_pend_head = (SERVICE *) malloc(sizeof(SERVICE));
		dll_init( (DLL *) Service_pend_head );
	}
	dll_insert_queue( (DLL *) Service_pend_head, (DLL *)newp );
	if( timeout ) {
		if(timeout < 10) timeout = 10;
		newp->curr_timeout = (type==ONCE_ONLY) ? timeout : timeout*1.5;
		newp->timer_ent = dtq_add_entry( Dic_timer_q,
						newp->curr_timeout,
						service_tmout, newp->serv_id );
	}
	else
		newp->timer_ent = NULL;
	newp->pending = pending;
	newp->tmout_done = 0;
#ifdef VxWorks
	newp->tid = taskIdSelf();
#endif
	return(newp);
}


void modify_service( servp, timeout, address, size, routine, 
			 tag, fill_addr, fill_size)
register SERVICE *servp;
int *address, size, *fill_addr, fill_size, tag, timeout;
void (*routine)();
{
	int *fillp;

	if( servp->timer_ent )
	{
		dtq_rem_entry( Dic_timer_q, servp->timer_ent );
		servp->timer_ent = 0;
	}
	servp->timeout = timeout;
	servp->serv_address = address;
	servp->serv_size = size;
	servp->user_routine = routine;
	servp->tag = tag;
	free( servp->fill_address );
	fillp = (int *)malloc(fill_size);
	memcpy( (char *) fillp, (char *) fill_addr, fill_size );
	servp->fill_address = fillp;
	servp->fill_size = fill_size;
	if(timeout)
	{
		servp->curr_timeout = timeout;
		servp->timer_ent = dtq_add_entry( Dic_timer_q,
						servp->curr_timeout,
						service_tmout, servp->serv_id );
	}
	else
		servp->timer_ent = NULL;
}


move_to_ok_service( servp, conn_id )
register SERVICE *servp;
int conn_id;
{
	dll_remove( (DLL *) servp );
	dll_insert_queue( (DLL *) Dic_conns[conn_id].service_head,
			  (DLL *) servp );
}

move_to_cmnd_service( servp )
register SERVICE *servp;
{

	dll_remove( (DLL *) servp );
	dll_insert_queue( (DLL *) Cmnd_head, (DLL *) servp );
}

move_to_notok_service( servp )
register SERVICE *servp;
{

	dll_remove( (DLL *) servp );
	dll_insert_queue( (DLL *) Service_pend_head, (DLL *) servp );
}

void dic_change_address( serv_id, serv_address, serv_size)
unsigned serv_id;
void *serv_address;
int serv_size;
{
	register SERVICE *servp;

	DISABLE_AST
	DIM_LOCK
	if( serv_id == 0 )
	  {
	        DIM_UNLOCK
	        ENABLE_AST
		return;
	  }
	servp = (SERVICE *)id_get_ptr(serv_id);
	servp->serv_address = (int *)serv_address;
	servp->serv_size = serv_size;
	DIM_UNLOCK
	ENABLE_AST
}

void dic_release_service( service_id )
register unsigned service_id;
{
	register SERVICE *servp;
	register int conn_id, pending;
	static DIC_PACKET *dic_packet;
	static int packet_size = 0;
	DIC_DNS_PACKET dic_dns_packet;
	register DIC_DNS_PACKET *dic_dns_p = &dic_dns_packet;
	SERVICE_REQ *serv_reqp;

	DISABLE_AST
	DIM_LOCK
	if( !packet_size ) {
		dic_packet = (DIC_PACKET *)malloc(DIC_HEADER);
		packet_size = DIC_HEADER;
	}
	if( service_id == 0 )
	  {
	        DIM_UNLOCK
	        ENABLE_AST
		return;
	  }
	servp = (SERVICE *)id_get_ptr(service_id);
	pending = servp->pending;
	switch( pending )
	{
	case NOT_PENDING :
		conn_id = servp->conn_id;
		strncpy(dic_packet->service_name, servp->serv_name, MAX_NAME); 
		dic_packet->type = htovl(DELETE);
		dic_packet->service_id = htovl(service_id);
		dic_packet->size = htovl(DIC_HEADER);
		dna_write( conn_id, dic_packet, DIC_HEADER );
		release_service( servp );
		break;
	case WAITING_SERVER_UP :
		if( Dns_dic_conn_id > 0) {
			dic_dns_p->size = htovl(sizeof(DIC_DNS_PACKET));
			dic_dns_p->src_type = htovl(SRC_DIC);
			serv_reqp = &dic_dns_p->service;
			strcpy( serv_reqp->service_name, servp->serv_name );
			serv_reqp->service_id = htovl(-1);
			dna_write( Dns_dic_conn_id, dic_dns_p,
				  sizeof(DIC_DNS_PACKET) );
		}
		release_service( servp );
		break;
	case WAITING_DNS_UP :
		release_service( servp );
		break;
	case WAITING_DNS_ANSWER :
		servp->pending = DELETED;
		break;
	}
	DIM_UNLOCK
	ENABLE_AST
}


int release_service( servicep )
register SERVICE *servicep;
{
	register SERVICE *servp;
	register int conn_id = 0;
	register int found = 0;
	register DIC_CONNECTION *dic_connp;

	conn_id = servicep->conn_id;
	dic_connp = &Dic_conns[conn_id] ;
	dll_remove( (DLL *) servicep );
	if( servicep->timer_ent )
		dtq_rem_entry( Dic_timer_q, servicep->timer_ent );
	if(servicep->type != COMMAND)
		free( servicep->fill_address );
	id_free(servicep->serv_id);
	free( servicep );
	if( conn_id )
	{
		if( !dic_connp->service_head )
		{
			if( servp = (SERVICE *) Cmnd_head ) {
				while( servp = (SERVICE *) dll_get_next(
							(DLL *) Cmnd_head,
							(DLL *) servp) )
				{
					if( servp->conn_id == conn_id)
						found = 1;
				}
			}
			if( !found)
			{
#ifdef DEBUG
				print_date_time();
				printf("Conn %d, Server %s on node %s released\n",
					conn_id, dic_connp->task_name, dic_connp->node_name);
#endif
				release_conn( conn_id );
			}
			return(0);
		}
		if( dll_empty((DLL *)dic_connp->service_head) ) 
		{
			if( servp = (SERVICE *) Cmnd_head ) {
				while( servp = (SERVICE *) dll_get_next(
							(DLL *) Cmnd_head,
							(DLL *) servp) )
				{
					if( servp->conn_id == conn_id)
						found = 1;
				}
			}
			if( !found)
			{
#ifdef DEBUG
				print_date_time();
				printf("Conn %d, Server %s on node %s released\n",
					conn_id, dic_connp->task_name, dic_connp->node_name);
#endif
				release_conn( conn_id );
			}
		}
	}
}


int locate_service( servp )
register SERVICE *servp;
{
	if(!strcmp(servp->serv_name,"DIS_DNS/SERVER_INFO"))
	{
		Tmout_min = DID_DNS_TMOUT_MIN;
		Tmout_max = DID_DNS_TMOUT_MAX;
	}
	if(Tmout_min == 0)
	{
		Tmout_min = DIC_DNS_TMOUT_MIN;
		Tmout_max = DIC_DNS_TMOUT_MAX;
	}
	if( !Dns_dic_conn_id )
	  {
	    DISABLE_AST;
		Dns_dic_conn_id = open_dns( recv_dns_dic_rout,
					Tmout_min,
					Tmout_max );
		ENABLE_AST;
	  }
	else
	{
	        DISABLE_AST;
		request_dns_info();
		ENABLE_AST;
	}

	return(Dns_dic_conn_id);
}


SERVICE *locate_command( serv_name )
register char *serv_name;
{
	register SERVICE *servp;

	if( servp = (SERVICE *) dll_search( (DLL *) Cmnd_head, serv_name,
					    strlen(serv_name)+1) )
		return(servp);
	return((SERVICE *)0);
}


static void request_dns_info()
{
	static SERVICE *servp;
	int n_pend = 0;

	if( Retry_timr_ent ) 
	{
		dtq_rem_entry( Dic_timer_q, Retry_timr_ent );
		Retry_timr_ent = NULL;
	}
	if( !Dns_dic_conn_id )
	{
		Dns_dic_conn_id = open_dns( recv_dns_dic_rout,
					   Tmout_min,
					   Tmout_max );
	} else {
		servp = Service_pend_head;
		while( servp = (SERVICE *) dll_get_next(
						(DLL *) Service_pend_head,
						(DLL *) servp) )
		{
			if( servp->pending == WAITING_DNS_UP)
			{
				request_dns_single_info( servp );
				n_pend++;
			}
			if(n_pend == 20)
			{
				Retry_timr_ent = dtq_add_entry( Dic_timer_q, 1,
							request_dns_info, 0 );
				break;
			}
		}
	}
}


request_dns_single_info( servp )
SERVICE *servp;
{
	static DIC_DNS_PACKET Dic_dns_packet;
	static SERVICE_REQ *serv_reqp;

	if( Dns_dic_conn_id > 0)
	{
/*
		printf("Requesting DNS Info for %s, id %d\n",servp->serv_name,
			servp->serv_id);
*/
		Dic_dns_packet.src_type = htovl(SRC_DIC);
		serv_reqp = &Dic_dns_packet.service;
		strcpy( serv_reqp->service_name, servp->serv_name );
		serv_reqp->service_id = htovl(servp->serv_id);
		servp->pending = htovl(WAITING_DNS_ANSWER);
		Dic_dns_packet.size = htovl(sizeof(DIC_DNS_PACKET));
		dna_write( Dns_dic_conn_id, &Dic_dns_packet,
			   sizeof(DIC_DNS_PACKET) );
	}
}


static int handle_dns_info( packet )
DNS_DIC_PACKET *packet;
{
	register int conn_id;
	register SERVICE *servp;
	char *node_name, *task_name;
	int port, protocol, format;
	register DIC_CONNECTION *dic_connp ;

	servp = (SERVICE *)id_get_ptr(vtohl(packet->service_id));
/*
	printf("Receiving DNS Info for service_id %s, id %d\n",servp->serv_name,
		packet->service_id);
*/
	node_name = packet->node_name; 
	task_name =  packet->task_name;
	port = vtohl(packet->port); 
	protocol = vtohl(packet->protocol);
	format = vtohl(packet->format);

	if( Dns_dic_timr )
		dtq_clear_entry( Dns_dic_timr );
	if( servp->pending == DELETED ) {
		release_service( servp );	
		return(0);
	}
	if( !node_name[0] ) {
		servp->pending = WAITING_SERVER_UP;
		if( servp->type != COMMAND )
			service_tmout( servp->serv_id ); 
		else
			dic_release_service( servp->serv_id );
/*
		if( ( servp->type == COMMAND )||( servp->type == ONCE_ONLY ) )
			dic_release_service( servp->serv_id );
*/
		return(0);
	}
#ifdef OSK
	{
		register char *ptr;

		if(strncmp(node_name,"fidel",5))
		{
			for(ptr = node_name; *ptr; ptr++)
			{
				if(*ptr == '.')
				{
					*ptr = '\0';
					break;
				}
			}
		}
	}
#endif
	if( !(conn_id = find_connection(node_name, task_name)) ) 
	{
		if( conn_id = dna_open_client(node_name, task_name, port,
					      protocol, recv_rout) )
		{
#ifndef VxWorks
			if(format & MY_OS9)
			{
				dna_set_test_write(conn_id, TEST_TIME_OSK);
				format &= 0xfffff7ff;
			}
			else
			{
				dna_set_test_write(conn_id, TEST_TIME_VMS);
			}
#endif
			dic_connp = &Dic_conns[conn_id];
			strncpy( dic_connp->node_name, node_name,
				 MAX_NODE_NAME); 
			strncpy( dic_connp->task_name, task_name,
				 MAX_TASK_NAME); 
#ifdef DEBUG
			print_date_time();
			printf("Conn %d, Server %s on node %s Connected\n",
			conn_id, dic_connp->task_name, dic_connp->node_name);
#endif
			dic_connp->service_head = 
						malloc(sizeof(SERVICE));
			dll_init( (DLL *) dic_connp->service_head);
		} else {       
			if(( servp->type == COMMAND )||( servp->type == ONCE_ONLY ))
			{
				dic_release_service( servp->serv_id );
				return(0);
			}
			service_tmout( servp->serv_id );
			servp->pending = WAITING_DNS_UP;
			if( !Retry_timr_ent )
				Retry_timr_ent = dtq_add_entry( Dic_timer_q,
								10,
								request_dns_info,
								0 );
			return(0);
		}
	}
	get_format_data(format, servp->format_data, packet->service_def);
	servp->conn_id = conn_id;
	send_service( conn_id, servp );
	servp->pending = NOT_PENDING;
	servp->tmout_done = 0;
}

static void get_format_data(format, format_data, def)
register int format;
register FORMAT_STR *format_data;
register char *def;
{
	register FORMAT_STR *formatp = format_data;
	register int i, index = 0;
	register char code;
	int num;

	while(*def)
	{
		code = *(def++);
		formatp->par_num = 0;
		formatp->flags = 0;
		switch(code)
		{
			case 'l':
			case 'L':
				formatp->par_bytes = SIZEOF_LONG;
				formatp->flags |= SWAPL;
				break;
			case 'x':
			case 'X':
				formatp->par_bytes = SIZEOF_DOUBLE;
				formatp->flags |= SWAPD;
				break;
			case 's':
			case 'S':
				formatp->par_bytes = SIZEOF_SHORT;
				formatp->flags |= SWAPS;
				break;
			case 'f':
			case 'F':
				formatp->par_bytes = SIZEOF_LONG;
				formatp->flags |= SWAPL;
#ifdef vms
				if((format & 0xF0) != (MY_FORMAT & 0xF0))
					formatp->flags |= (format & 0xF0);
#endif
				break;
			case 'd':
			case 'D':
				formatp->par_bytes = SIZEOF_DOUBLE;
				formatp->flags |= SWAPD;
#ifdef vms
				if((format & 0xF0) != (MY_FORMAT & 0xF0))
					formatp->flags |= (format & 0xF0);
#endif
				break;
			case 'c':
			case 'C':
				formatp->par_bytes = SIZEOF_CHAR;
				formatp->flags |= NOSWAP;
				break;
		}
		if(*def != ':')
		{
			if(*def)
				printf("Bad service definition parsing\n");
		}
		else
		{
			def++;
			sscanf(def,"%d",&num);
			formatp->par_num = num;
			while((*def != ';') && (*def != '\0'))
				def++;
			if(*def)
	       	    def++;
		}
		index++;
		formatp++;
	}
	formatp->par_bytes = 0;
	if((format & 0xF) == (MY_FORMAT & 0xF)) 
	{
		for(i = 0, formatp = format_data; i<index;i++, formatp++)
			formatp->flags &= 0xF0;    /* NOSWAP */
	}
}

int send_service(conn_id, servp)
register int conn_id;
register SERVICE *servp;
{
	static DIC_PACKET *dic_packet;
	static int serv_packet_size = 0;

	if( !serv_packet_size ) {
		dic_packet = (DIC_PACKET *)malloc(DIC_HEADER);
		serv_packet_size = DIC_HEADER;
	}

	if( servp->type == COMMAND ) 
	{
		if( send_command(conn_id, servp) )
		{
			if( !locate_command(servp->serv_name) ) 
			{
				move_to_cmnd_service( servp );	
			}
			else
			{
				servp->pending = WAITING_DNS_UP;
				dic_release_service( servp->serv_id );
			}
		}
	} 
	else 
	{
		strncpy( dic_packet->service_name, servp->serv_name, MAX_NAME ); 
		dic_packet->type = htovl(servp->type);
		dic_packet->timeout = htovl(servp->timeout);
		dic_packet->service_id = htovl(servp->serv_id);
		dic_packet->format = htovl(MY_FORMAT);
		dic_packet->size = htovl(DIC_HEADER);
		dna_write(conn_id, dic_packet, DIC_HEADER);
		if( servp->type == ONCE_ONLY ) 
		{
			if( !locate_command(servp->serv_name) ) 
			{
				move_to_cmnd_service( servp );	
			}
/*
			else
			{
				servp->pending = WAITING_DNS_UP;
				dic_release_service( servp->serv_id );
			}
*/
		}
		else
			move_to_ok_service( servp, conn_id );	
	}
	return(1);
}	


int send_command(conn_id, servp)
register int conn_id;
register SERVICE *servp;
{
	static DIC_PACKET *dic_packet;
	static int cmnd_packet_size = 0;
	char *name, *address;
	register int size;

	size = servp->fill_size;

	if( !cmnd_packet_size ) {
		dic_packet = (DIC_PACKET *)malloc(DIC_HEADER + size);
		cmnd_packet_size = DIC_HEADER + size;
	}
	else
	{
		if( DIC_HEADER + size > cmnd_packet_size ) {
			free( dic_packet );
			dic_packet = (DIC_PACKET *)malloc(DIC_HEADER + size);
			cmnd_packet_size = DIC_HEADER + size;
		}
	}

	strncpy(dic_packet->service_name, servp->serv_name, MAX_NAME); 
	dic_packet->type = htovl(COMMAND);
	dic_packet->timeout = htovl(0);
	dic_packet->format = htovl(MY_FORMAT);
	copy_swap_buffer(servp->format_data, 
					 dic_packet->buffer, servp->fill_address, 
					 size);
	dic_packet->size = htovl( size + DIC_HEADER);
	if(!dna_write(conn_id, dic_packet, DIC_HEADER + size))
		return(0);
	return(1);
}

static void copy_swap_buffer(format_data, buff_out, buff_in, size)
register int size;
register FORMAT_STR *format_data;
register void *buff_out, *buff_in;
{
	register int num;
#ifdef unix
	int res;
#endif
	int i, old_par_num;
	register int curr_size = 0, curr_out = 0;
#ifdef vms
	unsigned int input_code, output_code;
#endif

	if(!format_data->par_bytes)
	{
		if(buff_in != buff_out)
			memcpy( buff_out, buff_in, size );
		return;
	}
	for( ; format_data->par_bytes; format_data++)
	{
		old_par_num = format_data->par_num;
		if((format_data->par_num * format_data->par_bytes) > size)
		{
			format_data->par_num = (size - curr_size)/format_data->par_bytes;
		}
		switch(format_data->flags & 0x3)
		{
			case NOSWAP :
				if(!(num = (format_data->par_num * format_data->par_bytes)))
					num = size - curr_size;
				if(num > (size - curr_size))
					num = size - curr_size;
				if(buff_in != buff_out)
					memcpy( buff_out, buff_in, num );
				inc_pter( buff_in, num);
				inc_pter( buff_out, num);
#ifdef unix
#ifndef aix
				/* On UNIX but AIX, if the next element is a double, 
				   align on a double */
				if((format_data + 1)->par_bytes == SIZEOF_DOUBLE)
				{
					if(res = num % SIZEOF_DOUBLE)
					{
						res = SIZEOF_DOUBLE - res;
						inc_pter( buff_out, res) ;
						curr_out += res;
					}
				}
				else 
#endif
				/* else align on an int */
				if(res = num % SIZEOF_LONG)
				{
					res = SIZEOF_LONG - res;
					inc_pter( buff_out, res);
					curr_out += res;
				}
#endif
				num = num / format_data->par_bytes;
				break;
			case SWAPS :
				if(!(num = format_data->par_num))
					num = (size - curr_size)/SIZEOF_SHORT;
#ifdef unix
				res = ((num*SIZEOF_SHORT) % SIZEOF_LONG);
#endif
				_swaps_buffer( (short *)buff_out, (short *)buff_in, num) ;
/*				
				for( i = 0; i<num; i++) 
				{
					*(short *)buff_out = _swaps_by_addr((short *)buff_in);
					inc_pter( buff_in, SIZEOF_SHORT);
					inc_pter( buff_out, SIZEOF_SHORT);
				}
*/
				inc_pter( buff_in, SIZEOF_SHORT * num);
				inc_pter( buff_out, SIZEOF_SHORT * num);
#ifdef unix
				/* In case there was an odd number of shorts align on int boudaries for UNIX */
				if(res)
				{
					res = SIZEOF_LONG - res;
					inc_pter( buff_out, res);
					curr_out += res;
				}
#endif
				break;
			case SWAPL :
				if(!(num = format_data->par_num))
					format_data->par_num = num = (size - curr_size)/SIZEOF_LONG;
				_swapl_buffer( (int *)buff_out, (int *)buff_in, num);
/*				
				for( i = 0; i<num; i++) 
				{
					*(int *)buff_out = _swapl_by_addr((int *)buff_in);
					inc_pter( buff_in, SIZEOF_LONG);
					inc_pter( buff_out, SIZEOF_LONG);
				}
*/
				inc_pter( buff_in, SIZEOF_LONG * num);
				inc_pter( buff_out, SIZEOF_LONG * num);
				break;
			case SWAPD :
				if(!(num = format_data->par_num))
					format_data->par_num = num = (size - curr_size)/SIZEOF_DOUBLE;
#ifdef unix
#ifndef aix
				/* On all UNIX machines but AIX, doubles are aligned on double boundaries */
				if(res = (curr_out % SIZEOF_DOUBLE))
				{
					res = SIZEOF_DOUBLE - res;
					inc_pter( buff_out, res);
					curr_out += res;
				}
#endif
#endif
				_swapd_buffer( (double *)buff_out, (double *)buff_in, num);
/*				
				for( i = 0; i<num; i++)
				{
					*(double *)buff_out = _swapd_by_addr((double *)buff_in);
					inc_pter( buff_in, SIZEOF_DOUBLE);
					inc_pter( buff_out, SIZEOF_DOUBLE);
				}
*/
				inc_pter( buff_in, SIZEOF_DOUBLE * num);
				inc_pter( buff_out, SIZEOF_DOUBLE * num);
				break;
		}
		curr_size += num * format_data->par_bytes;
		curr_out += curr_size;
#ifdef vms
		if(format_data->flags & 0xF0)
		{
			/* Floating point conversion on VMS... */
			switch(format_data->par_bytes) {
				case SIZEOF_FLOAT :
					if((format_data->flags & 0xF0) == IEEE_FLOAT)
					{
						num = format_data->par_num;
						(int *)buff_out -= num;
						for( i = 0; i<num; i++)
						{
							cvt$convert_float((void *)buff_out, CVT$K_IEEE_S,
										  (void *)buff_out, CVT$K_VAX_F,
										  0 );
							((int *)buff_out)++;
						}
					}
					break;
				case SIZEOF_DOUBLE :
#ifdef __alpha
					output_code = CVT$K_VAX_G;
#else
					output_code = CVT$K_VAX_D;
#endif
					switch(format_data->flags & 0xF0)
					{
						case VAX_FLOAT:
							input_code = CVT$K_VAX_D;
							break;	
						case AXP_FLOAT:
							input_code = CVT$K_VAX_G;
							break;	
						case IEEE_FLOAT:
							input_code = CVT$K_IEEE_T;
							break;	
					}							
					num = format_data->par_num;
					(double *)buff_out -= num;
					for( i = 0; i<num; i++)
					{
						cvt$convert_float((void *)buff_out, input_code,
										  (void *)buff_out, output_code,
										  0 );
						((double *)buff_out)++;
					}
					break;
			}
		}
#endif
		format_data->par_num = old_par_num;
	}
}

int find_connection(node, task)
register char *node, *task;
{
	register int i;
	register DIC_CONNECTION *dic_connp;

	for( i=0, dic_connp = Dic_conns; i<Curr_N_Conns; i++, dic_connp++ )
	{
		if((!strcmp(dic_connp->task_name, task))
			&&(!strcmp(dic_connp->node_name, node)))
			return(i);
	}
	return(0);
}
	
#ifdef VxWorks
void dic_destroy(int tid)
{
	register int i;
	register DIC_CONNECTION *dic_connp;
	register SERVICE *servp, *auxp;
	int found = 0;

	if(!Dic_conns)
	  return;
	for( i=0, dic_connp = Dic_conns; i<Curr_N_Conns; i++, dic_connp++ )
	{
	        if(servp = (SERVICE *) dic_connp->service_head)
		  {
		    while( servp = (SERVICE *) dll_get_next(
					(DLL *) dic_connp->service_head,
				 	(DLL *) servp) )
		      {
			if( servp->tid == tid )
			  {
			    auxp = servp->prev;
			    dic_release_service( servp->serv_id );
			    servp = auxp;
			    if(!dic_connp->service_head)
			      break; 
			  }
			else
			  found = 1;
		    }
		}
	}
	if(!found)
	  {
	    if(Dns_dic_conn_id)
	      {
		dna_close( Dns_dic_conn_id );
		Dns_dic_conn_id = 0;
	      }
	  }
}

void DIMDestroy(int tid)
{
  dis_destroy(tid);
  dic_destroy(tid);
}

#endif

static void release_conn(conn_id)
register int conn_id;
{
	register DIC_CONNECTION *dic_connp = &Dic_conns[conn_id];

	dic_connp->task_name[0] = '\0';
	if(dic_connp->service_head)
	{
		free((SERVICE *)dic_connp->service_head);
		dic_connp->service_head = (char *)0;
	}
	dna_close(conn_id);
}	


#ifdef VMS
/* CFORTRAN WRAPPERS */
FCALLSCFUN9(INT, dic_info_service, DIC_INFO_SERVICE, dic_info_service,
                 STRING, INT, INT, PVOID, INT, PVOID, INT, PVOID, INT)
FCALLSCFUN3(INT, dic_cmnd_service, DIC_CMND_SERVICE, dic_cmnd_service,
                 STRING, PVOID, INT)
FCALLSCSUB3(     dic_change_address, DIC_CHANGE_ADDRESS, dic_change_address,
                 INT, PVOID, INT)
FCALLSCSUB1(     dic_release_service, DIC_RELEASE_SERVICE, dic_release_service,
                 INT)
#endif
