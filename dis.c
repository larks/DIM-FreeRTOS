/*
 * DIS (Delphi Information Server) Package implements a library of
 * routines to be used by servers.
 *
 * Started on        : 10-11-91
 * Last modification : 28-07-94
 * Written by        : C. Gaspar
 * Adjusted by       : G.C. Ballintijn
 *
 * $Id: dis.c,v 1.1 1994/07/18 19:50:03 gerco Exp gerco $
 */

#ifdef VMS
#	include <lnmdef.h>
#	include <cvtdef.h>
#	include <ssdef.h>
#	include <descrip.h>
#	include <cfortran.h>
#endif

#include <dim.h>
#include <dis.h>

#define ALL 0
#define MORE 1
#define NONE 2
/*
typedef enum { ALL, MORE, NONE } DIS_REGIST_TYPES;
*/

typedef struct serv {
	struct serv *next;
	char name[MAX_NAME];
	int type;
	char def[MAX_NAME];
	int id;
	FORMAT_STR format_data[MAX_NAME/4];
	int *address;
	int size;
	void (*user_routine)();
	int tag;
	int registered;
        int tid;
} SERVICE;

static SERVICE *Service_head = (SERVICE *)0;	

typedef struct req_ent {
	struct req_ent *next;
	int conn_id;
	int service_id;
	int req_id;
	int type;
	SERVICE *service_ptr;
	int timeout;
	int format;
	int first_time;
	TIMR_ENT *timr_ent;
} REQUEST;

static char Task_name[MAX_TASK_NAME];
static REQUEST *Request_head = (REQUEST *)0;
static TIMR_ENT *Dns_timr_ent = (TIMR_ENT *)0;
static DIS_DNS_PACKET Dis_dns_packet;
static int Dis_n_services = 0;
static int Dns_dis_conn_id = 0;
static int Protocol;
static int Port_number;
static int Dis_first_time = 1;
static int Dis_conn_id;

/* Do not forget to increase when this file is modified */
static int Version_number = 3;
static int Dis_timer_q = 0;


static void dis_insert_request(int conn_id, DIC_PACKET *dic_packet,
				  int size, int status );
void execute_service(int req_id);
void execute_command(SERVICE *servp, DIC_PACKET *packet);
void register_services(int flag);
void std_cmnd_handler(int *tag, int *cmnd_buff, int *size);
void client_info(int *tag, int **bufp, int *size);
void service_info(int *tag, int **bufp, int *size);
SERVICE *find_service(char *name);
static void copy_swap_buffer(int format, FORMAT_STR *format_data, 
					void *buff_out, void *buff_in, int size);
static void get_format_data(FORMAT_STR *format_data, char *def);


/* Add a service to the list of known services
 *
 * A service can be part of a global section - address, size.
 * or a service can be provided by a user routine - user_routine.
 */

static unsigned do_dis_add_service( register char * name, register char * type,
void * address, int size, void *(user_routine)(), int tag )
{
	register SERVICE *new_serv;
	register int service_id;

       
       	if( !Service_head ) {
		Service_head = (SERVICE *)malloc(sizeof(SERVICE));
		sll_init( (SLL *) Service_head );
	}
	if( find_service(name) )
	  {
		return((unsigned) 0);
	  }
	new_serv = (SERVICE *)malloc( sizeof(SERVICE) );
	strncpy( new_serv->name, name, MAX_NAME );
	if(type != (char *)0)
	{
		get_format_data(new_serv->format_data, type);
		strcpy(new_serv->def,type); 
	}
	else
	{
		new_serv->format_data[0].par_bytes = 0;
		new_serv->def[0] = '\0';
	}
	new_serv->type = 0;
	new_serv->address = (int *)address;
	new_serv->size = size;
	new_serv->user_routine = user_routine;
	new_serv->tag = tag;
	new_serv->registered = 0;
	new_serv->tid = 0;
	service_id = id_get((void *)new_serv);
	new_serv->id = service_id;
	new_serv->next = 0;
	sll_insert_queue( (SLL *) Service_head, (SLL *) new_serv );
	Dis_n_services++;
	return((unsigned)service_id);
}

#ifdef VxWorks
void dis_destroy(int tid)
{
register SERVICE *servp, *prevp;
int n_left = 0;

	prevp = Service_head;
	while( servp = (SERVICE *) sll_get_next((SLL *) prevp) ) 
	{
	  if(servp->tid == tid)
	    {
	      dis_remove_service(servp->id);
	    }
	  else
	    {
	    prevp = servp;
	    n_left++;
	    }
	}
	if(n_left == 3)
	  {
	    prevp = Service_head;
	    while( servp = (SERVICE *) sll_get_next((SLL *) prevp) ) 
	      {
		dis_remove_service(servp->id);
	      }
	    dna_close(Dis_conn_id);
	    dna_close(Dns_dis_conn_id);
	    Dns_dis_conn_id = 0;
	    Dis_first_time = 1;
	    dtq_rem_entry(Dis_timer_q, Dns_timr_ent);
	    Dns_timr_ent = NULL;
	  }
}
#endif

unsigned dis_add_service( char * name, char * type, void * address, int size, void (*user_routine)(), int tag)
{
	unsigned ret;
#ifdef VxWorks
	register SERVICE *servp;
#endif
#ifdef FreeRTOS
	register SERVICE *servp;
#endif

	DISABLE_AST
	DIM_LOCK
       	ret = do_dis_add_service( name, type, address, size,user_routine,tag);
#ifdef VxWorks
	servp = (SERVICE *)id_get_ptr(ret);
	servp->tid = taskIdSelf(); /* XXX: This kind of thing requires traceability, not something I want to enable 
	                                    also -- it is not reallyused other places than here and in a utilities function*/
#endif
#ifdef FreeRTOS
	servp = (SERVICE *)id_get_ptr(ret);
	servp->tid = xTaskGetCurrentTaskHandle();
#endif /* TODO: remove ifs */
	DIM_UNLOCK
	ENABLE_AST
	return(ret);
}

void dis_add_cmnd(char * name, char * type, void (*user_routine)(), int tag )
{
	register SERVICE *new_serv;
	register int service_id;

	DISABLE_AST
	DIM_LOCK
	if( !Service_head ) {
		Service_head = (SERVICE *)malloc(sizeof(SERVICE));
		sll_init( (SLL *) Service_head );
	}
	if( find_service(name) )
	  {
 	        DIM_UNLOCK
	        ENABLE_AST
		return;
	  }
	new_serv = (SERVICE *)malloc(sizeof(SERVICE));
	strncpy(new_serv->name, name, MAX_NAME);
	if(type != (char *)0)
	{
		get_format_data(new_serv->format_data, type);
		strcpy(new_serv->def,type); 
	}
	else
	{
		new_serv->format_data[0].par_bytes = 0;
		new_serv->def[0] = '\0';
	}
	new_serv->type = COMMAND;
	new_serv->address = 0;
	new_serv->size = 0;
	if(user_routine)
		new_serv->user_routine = user_routine;
	else
		new_serv->user_routine = std_cmnd_handler;
	new_serv->tag = tag;
	new_serv->tid = 0;
	new_serv->registered = 0;
	service_id = id_get((void *)new_serv);
	new_serv->id = service_id;
	new_serv->next = 0;
	sll_insert_queue( (SLL *) Service_head, (SLL *) new_serv );
	DIM_UNLOCK
	ENABLE_AST
}

static void get_format_data(format_data, def)
register FORMAT_STR *format_data;
register char *def;
{
	register int index = 0;
	register char code;
	int num;

	if(*def)
	{
		while(*def)
		{
			code = *(def++);
			format_data->par_num = 0;
			format_data->flags = 0;
			switch(code)
			{
				case 'l':
				case 'L':
					format_data->par_bytes = SIZEOF_LONG;
					format_data->flags |= SWAPL;
					break;
				case 'x':
				case 'X':
					format_data->par_bytes = SIZEOF_DOUBLE;
					format_data->flags |= SWAPD;
					break;
				case 's':
				case 'S':
					format_data->par_bytes = SIZEOF_SHORT;
					format_data->flags |= SWAPS;
					break;
				case 'f':
				case 'F':
					format_data->par_bytes = SIZEOF_LONG;
					format_data->flags |= SWAPL;
#ifdef vms
					format_data->flags |= (0x1 << 4);
#endif
					break;
				case 'd':
				case 'D':
					format_data->par_bytes = SIZEOF_DOUBLE;
					format_data->flags |= SWAPD;
#ifdef vms
					format_data->flags |= (0x1 << 4);
#endif
					break;
				case 'c':
				case 'C':
					format_data->par_bytes = SIZEOF_CHAR;
					format_data->flags |= NOSWAP;
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
				format_data->par_num = num;
				while((*def != ';') && (*def != '\0'))
					def++;
				if(*def)
	        	    def++;
			}
			index++;
			format_data++;
		}
    }
	format_data->par_bytes = 0;
}

void recv_dns_dis_rout( int conn_id, DNS_DIS_PACKET * packet, int size, int status )
{
	switch(status)
	{
	case STA_DISC:       /* connection broken */
		if( Dns_timr_ent ) {
			dtq_rem_entry( Dis_timer_q, Dns_timr_ent );
			Dns_timr_ent = NULL;
		}
		dna_close(Dns_dis_conn_id);
		Dns_dis_conn_id = open_dns(recv_dns_dis_rout, 
					DIS_DNS_TMOUT_MIN, DIS_DNS_TMOUT_MAX );
		break;
	case STA_CONN:        /* connection received */
		Dns_dis_conn_id = conn_id;
		register_services(ALL);
		Dns_timr_ent = dtq_add_entry( Dis_timer_q,
					      rand_tmout(WATCHDOG_TMOUT_MIN, 
							 WATCHDOG_TMOUT_MAX),
					      register_services, NONE ); 
		break;
	default :       /* normal packet */
		switch( vtohl(packet->type) )
		{
		case DNS_DIS_REGISTER :
			print_date_time();
			printf(" NAME SERVER requests service registration\n");
			register_services(ALL);
			break;
		case DNS_DIS_KILL :
			print_date_time();
printf(" Some Services already known to NAME SERVER, commiting suicide\n");
			exit(0);
			break;
		}
		break;
	}
}


/* register services within the name server
 *
 * Send services uses the DNA package. services is a linked list of services
 * stored by add_service.
 */
void register_services(register int flag)
{
	register DIS_DNS_PACKET *dis_dns_p = &Dis_dns_packet;
	register int n_services;
	register SERVICE *servp;
	register SERVICE_REG *serv_regp;
	register int conn_id;

	get_node_name( dis_dns_p->node_name );
	strcpy( dis_dns_p->task_name, Task_name );
	dis_dns_p->port = htovl(Port_number);
	dis_dns_p->protocol = htovl(Protocol);
	dis_dns_p->src_type = htovl(SRC_DIS);
	dis_dns_p->format = htovl(MY_FORMAT);
	serv_regp = dis_dns_p->services;
	n_services = 0;
	if( flag == NONE ) {
		dis_dns_p->n_services = htovl(n_services);
		dis_dns_p->size = htovl( DIS_DNS_HEADER + 
			(n_services*sizeof(SERVICE_REG)));
		if(Dns_dis_conn_id > 0)
		{
			if(!dna_write(Dns_dis_conn_id, &Dis_dns_packet, 
				DIS_DNS_HEADER + n_services*sizeof(SERVICE_REG)))
			{
				release_all_requests(Dns_dis_conn_id);
				print_date_time();
				printf(" Couldn't write to name server\n");
			}
		}
		return;
	}
	servp = Service_head;
	while( servp = (SERVICE *) sll_get_next((SLL *) servp) ) 
	{
		if( flag == MORE ) 
		{
			if( servp->registered )
				continue;
		}
		strcpy( serv_regp->service_name, servp->name );
		strcpy( serv_regp->service_def, servp->def );
		if(servp->type == COMMAND)
			serv_regp->service_id = htovl( servp->id | 0x10000000);
		else
			serv_regp->service_id = htovl( servp->id );
		serv_regp++;
		n_services++;
		servp->registered = 1;
		if( n_services == MAX_SERVICE_UNIT ) 
		{
			dis_dns_p->n_services = htovl(n_services);
			dis_dns_p->size = htovl(DIS_DNS_HEADER +
				n_services * sizeof(SERVICE_REG));
			if(Dns_dis_conn_id > 0)
			{
				if( !dna_write(Dns_dis_conn_id,
				       &Dis_dns_packet, 
				       DIS_DNS_HEADER + n_services *
						sizeof(SERVICE_REG)) )
				{
					release_all_requests( Dns_dis_conn_id );
					print_date_time();
					printf(" Couldn't write to name server\n");
				}
			}
			serv_regp = dis_dns_p->services;
			n_services = 0;
		}
	}
	if( n_services ) 
	{
		dis_dns_p->n_services = htovl(n_services);
		dis_dns_p->size = htovl(DIS_DNS_HEADER +
					n_services * sizeof(SERVICE_REG));
		if(Dns_dis_conn_id > 0)
		{
			if( !dna_write(Dns_dis_conn_id, &Dis_dns_packet,
				DIS_DNS_HEADER + n_services * sizeof(SERVICE_REG)))
			{
				release_all_requests( Dns_dis_conn_id );
				print_date_time();
				printf(" Couldn't write to name server\n");
			}
		}
	}
}

void unregister_service(register SERVICE * servp)
{
	register DIS_DNS_PACKET *dis_dns_p = &Dis_dns_packet;
	register int n_services;
	register SERVICE_REG *serv_regp;
	register int conn_id;

	get_node_name( dis_dns_p->node_name );
	strcpy( dis_dns_p->task_name, Task_name );
	dis_dns_p->port = htovl(Port_number);
	dis_dns_p->protocol = htovl(Protocol);
	dis_dns_p->src_type = htovl(SRC_DIS);
	dis_dns_p->format = htovl(MY_FORMAT);
	serv_regp = dis_dns_p->services;
	strcpy( serv_regp->service_name, servp->name );
	strcpy( serv_regp->service_def, servp->def );
	serv_regp->service_id = htovl( servp->id | 0x80000000);
	serv_regp++;
	n_services = 1;
	servp->registered = 0;
	dis_dns_p->n_services = htovl(n_services);
	dis_dns_p->size = htovl(DIS_DNS_HEADER +
				n_services * sizeof(SERVICE_REG));
	if(Dns_dis_conn_id > 0)
	{
		if( !dna_write(Dns_dis_conn_id, &Dis_dns_packet, 
				       DIS_DNS_HEADER + n_services * sizeof(SERVICE_REG)) )
		{
			release_all_requests( Dns_dis_conn_id );
			print_date_time();
			printf(" Couldn't write to name server\n");
		}
	}
}


/* start serving client requests
 *
 * Using the DNA package start accepting requests from clients.
 * When a request arrives the routine "dis_insert_request" will be executed.
 */
int dis_start_serving(char * task)
{
	static int server_open = 0;
	char str0[MAX_NAME], str1[MAX_NAME],str2[MAX_NAME];
	int protocol;

	DISABLE_AST
	DIM_LOCK
	  /*
#ifdef VxWorks
	taskDeleteHookAdd(remove_all_services);
	printf("Adding delete hook\n");
#endif
*/
	if(Dis_first_time)
	{
		Dis_first_time = 0;
		sprintf(str0, "%s/VERSION_NUMBER", task);
		sprintf(str1, "%s/CLIENT_INFO", task);
		sprintf(str2, "%s/SERVICE_INFO", task);
		if( strlen(task) > 16 )
			task[16] = '\0';
		Port_number = SEEK_PORT;
		if( !(Dis_conn_id = dna_open_server( task, dis_insert_request, &Protocol, 
				      &Port_number) ))
		  {
		        DIM_UNLOCK
		        ENABLE_AST
			return(0);
		  }
		do_dis_add_service( str0, 0, &Version_number,
				 sizeof(Version_number), 0, 0 );
		do_dis_add_service( str1, 0, 0, 0, client_info, 0 );
		do_dis_add_service( str2, 0, 0, 0, service_info, 0 );
		strcpy( Task_name, task );
	}
	if(!Dis_timer_q)
	  Dis_timer_q = dtq_create();
	if( !Dns_dis_conn_id )
	{
		if(!strcmp(task,"DIS_DNS"))
		{
			register_services(ALL);
			DIM_UNLOCK
			ENABLE_AST
			return(id_get(&Dis_dns_packet));
		}
		else
		  {
	    
			Dns_dis_conn_id = open_dns(recv_dns_dis_rout,
					DIS_DNS_TMOUT_MIN, DIS_DNS_TMOUT_MAX );
		    
		  }
	}
	else
	{
		register_services(MORE);
	}
	DIM_UNLOCK	
	ENABLE_AST
	return(1);
}

#ifndef VxWorks
char *my_malloc(size)
int size;
{
	register char *addr;

	if(!(addr = malloc(size)))
	{
		do
		{
			sleep(1);
			addr = malloc(size);
		} while(!addr);
	}
	return(addr);
}
#endif

/* asynchrounous reception of requests */
/*
	Called by DNA package.
	A request has arrived, queue it to process later - dis_ins_request
*/
static void dis_insert_request(int conn_id, DIC_PACKET *dic_packet, int size, int status)
register int conn_id;
register DIC_PACKET *dic_packet;
register int size, status;
{
	register SERVICE *servp;
	register REQUEST *newp;
	void (*usr_rout)();

	/* status = 1 => new connection, status = -1 => conn. lost */
	
	if(!Request_head) 
	{
		Request_head = (REQUEST *)malloc(sizeof(REQUEST));
		sll_init( (SLL *) Request_head );
	}
	if(status != 0)
	{
		if(status == -1)
		/* release all requests from conn_id */
		{
			release_all_requests(conn_id);
		}
	} else {
		if(!(servp = find_service(dic_packet->service_name)))
		  {
			return;
		  }
		dic_packet->type = vtohl(dic_packet->type);
		if(dic_packet->type == COMMAND) 
		{
			execute_command(servp, dic_packet);
			return;
		}
		if(dic_packet->type == DELETE) 
		{
			release_request(conn_id, vtohl(dic_packet->service_id));
			return;
		}
		newp = (REQUEST *)/*my_*/malloc(sizeof(REQUEST));
		newp->service_ptr = servp;
		newp->service_id = vtohl(dic_packet->service_id);
		newp->type = dic_packet->type;
		newp->timeout = vtohl(dic_packet->timeout);
		newp->format = vtohl(dic_packet->format);
		newp->conn_id = conn_id;
		newp->next = 0;
		newp->first_time = 1;
		newp->timr_ent = 0;
		newp->req_id = id_get((void *)newp);
		if(newp->type == ONCE_ONLY) 
		{
			execute_service(newp->req_id);
			id_free(newp->req_id);
			free(newp);
			return;
		}
		sll_insert_queue( (SLL *) Request_head, (SLL *) newp );
		if((newp->type != MONIT_ONLY) && (newp->type != UPDATE))
		{
			execute_service(newp->req_id);
		}
		if(newp->type != MONIT_ONLY)
		{
			if(newp->timeout != 0)
			{
				newp->timr_ent = dtq_add_entry( Dis_timer_q,
							newp->timeout, 
							execute_service,
							newp->req_id );
			}
		}
	}
}

/* A timeout for a timed or monitored service occured, serve it. */

void execute_service( req_id )
int req_id;
{
	int *buffp, size, i, done = 0;
	register REQUEST *reqp;
	register SERVICE *servp;
	static DIS_PACKET *dis_packet;
	static int packet_size = 0;
	char str[80], def[MAX_NAME];
	register char *ptr;

	reqp = (REQUEST *)id_get_ptr(req_id);
	if(!reqp)
		return;
	servp = reqp->service_ptr;
	ptr = servp->def;
	if(servp->type == COMMAND)
	{
		sprintf(str,"This is a COMMAND Service");
		buffp = (int *)str;
		size = 26;
		sprintf(def,"c:26");
		ptr = def;
	}
	else if( servp->user_routine != 0 ) 
	{
/*
		printf("Executing %s routine\n", servp->name);
*/
		(servp->user_routine)( &servp->tag, &buffp, &size,
					&reqp->first_time );
		reqp->first_time = 0;
		
	} 
	else 
	{
		buffp = servp->address;
		size = servp->size;
/*
		printf("Executing %s buffer, size = %d :\n", servp->name, size);
		printf("\t%s\n",buffp);
*/
	}
	if( !packet_size ) {
		dis_packet = (DIS_PACKET *)malloc(DIS_HEADER + size);
		packet_size = DIS_HEADER + size;
	}
	if( !size )
		return;
	else 
	{
		if( DIS_HEADER + size > packet_size ) {
			free( dis_packet );
			dis_packet = (DIS_PACKET *)malloc(DIS_HEADER + size);
			packet_size = DIS_HEADER + size;
		}
	}
	dis_packet->service_id = htovl(reqp->service_id);
	copy_swap_buffer(reqp->format, servp->format_data, dis_packet->buffer,
		buffp, size);
/*
	memcpy(dis_packet->buffer, buffp, size);
*/
	dis_packet->size = htovl(DIS_HEADER + size);
/*
	if(!strcmp(Task_name,"DIS_DNS"))
	{
		if( !dna_write(reqp->conn_id, dis_packet, size+DIS_HEADER) ) 
		{
			release_all_requests( reqp->conn_id );
			print_date_time();
			printf(" Couldn't write to client\n");
		}
	}
	else
	{
*/
		if( !dna_write_nowait(reqp->conn_id, dis_packet, size+DIS_HEADER) ) 
		{
			release_all_requests( reqp->conn_id );
			print_date_time();
			printf(" Couldn't write to client, releasing connection\n");
		}
/*
	}
*/
}

void remove_service( req_id )
int req_id;
{
	register REQUEST *reqp;
	register SERVICE *servp;
	static DIS_PACKET *dis_packet;
	static int packet_size = 0;
	int service_id;

	reqp = (REQUEST *)id_get_ptr(req_id);
	servp = reqp->service_ptr;
	if( !packet_size ) {
		dis_packet = (DIS_PACKET *)malloc(DIS_HEADER);
		packet_size = DIS_HEADER;
	}
	service_id = (reqp->service_id | 0x80000000);
	dis_packet->service_id = htovl(service_id);
	dis_packet->size = htovl(DIS_HEADER);
	if( !dna_write(reqp->conn_id, dis_packet, DIS_HEADER) ) 
	{
		release_all_requests( reqp->conn_id );
		print_date_time();
		printf(" Couldn't write to client\n");
	}
}

static void copy_swap_buffer(format, format_data, buff_out, buff_in, size)
int format;
register int size;
register FORMAT_STR *format_data;
register void *buff_out, *buff_in;
{
	register int num, curr_size = 0, old_par_num;
	int i, res;
#ifdef vms
	unsigned int input_code;
#endif

	if(!format_data->par_bytes) {
		if(buff_in != buff_out)
			memcpy( buff_out, buff_in, size );
		return;
	}
	for( ; format_data->par_bytes; format_data++) 
	{
		old_par_num = format_data->par_num;
		switch(format_data->flags & 0x3) {
			case NOSWAP :
				if(!(num = format_data->par_num))
					format_data->par_num = num = size - curr_size;
				memcpy( buff_out, buff_in, num);
				inc_pter( buff_in, num);
#ifdef unix
				if(res = num % SIZEOF_LONG) 
				{
					res = SIZEOF_LONG - res;
					inc_pter( buff_in, res);
					curr_size += res;
				}
#endif
				inc_pter( buff_out, num);
				break;
			case SWAPS :
				if(!(num = format_data->par_num))
				{
					num = size - curr_size;
					format_data->par_num = num/SIZEOF_SHORT;
				} 
				else 
				{
					num *= SIZEOF_SHORT;
				}
				memcpy( buff_out, buff_in, num);
				inc_pter( buff_in, num);
#ifdef unix
				if(res = num % SIZEOF_LONG)
				{
					res = SIZEOF_LONG - res;
					inc_pter( buff_in, res);
					curr_size += res;
				}
#endif
				inc_pter( buff_out, num);
				break;
			case SWAPL :
				if(!(num = format_data->par_num))
				{
					num = size - curr_size;
					format_data->par_num = num/SIZEOF_LONG;
				} 
				else 
					num *= SIZEOF_LONG;
				memcpy( buff_out, buff_in, num);
				inc_pter( buff_in, num);
				inc_pter( buff_out, num);
				break;
			case SWAPD :
				if(!(num = format_data->par_num))
				{
					num = size - curr_size;
					format_data->par_num = num/SIZEOF_DOUBLE;
				} 
				else
					num *= SIZEOF_DOUBLE;
#ifdef unix
#	ifndef aix
				if(res = curr_size%SIZEOF_DOUBLE)
				{
					res = SIZEOF_DOUBLE - res;
					inc_pter( buff_in, res);
					curr_size += res;
				}
#	endif
#endif
				memcpy( buff_out, buff_in, num );
				inc_pter( buff_in, num);
				inc_pter( buff_out, num);
				break;
		}
		curr_size += num;
#ifdef vms
		if(	(format_data->flags & 0x10) && ((format & 0xF0) == IEEE_FLOAT) )
		{
			switch(format_data->flags & 0x3)
			{
				case SWAPL :
					num = format_data->par_num;
					(int *)buff_out -= num;
					for( i = 0; i < num; i++) 
					{
						cvt$convert_float((void *)buff_out, CVT$K_VAX_F, 
										  (void *)buff_out, CVT$K_IEEE_S,
										  0 );
						((int *)buff_out)++;
					}
					break;
				case SWAPD :
#ifdef __alpha
					input_code = CVT$K_VAX_G;
#else
					input_code = CVT$K_VAX_D;
#endif
					num = format_data->par_num;
					(double *)buff_out -= num;
					for( i = 0; i < num; i++ )
					{
						cvt$convert_float((void *)buff_out, input_code,
										  (void *)buff_out, CVT$K_IEEE_T,
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

void execute_command(servp, packet)
register SERVICE *servp;
DIC_PACKET *packet;
{
	int tag, buf, size;

	if(servp->user_routine != 0)
	{
		size = vtohl(packet->size) - DIC_HEADER;
		(servp->user_routine)(&servp->tag, packet->buffer, &size);
	}
}

void dis_report_service(serv_name)
char *serv_name;
{
	register SERVICE *servp;
	register REQUEST *reqp;

	
	DISABLE_AST
	DIM_LOCK
	servp = find_service(serv_name);
	reqp = Request_head;
	while(reqp = (REQUEST *) sll_get_next((SLL *) reqp))
	{
		if(reqp->service_ptr == servp)
		{
			if(reqp->type != TIMED_ONLY)
				execute_service(reqp->req_id);
		}
	}
	DIM_UNLOCK
	ENABLE_AST
}

int dis_update_service(service_id)
register unsigned service_id;
{
	register REQUEST *reqp;
	register SERVICE *servp;
	register int found = 0;

	DISABLE_AST
	DIM_LOCK
	if(!service_id)
	{
		print_date_time();
		printf(" Invalid service id\n");
		DIM_UNLOCK
		ENABLE_AST
		return(found);
	}
	servp = (SERVICE *)id_get_ptr(service_id);
	reqp = Request_head;
	while( reqp = (REQUEST *) sll_get_next((SLL *) reqp) ) 
	{
		if( reqp->service_ptr == servp ) 
		{
			if( reqp->type != TIMED_ONLY ) 
			{
				execute_service(reqp->req_id);
				found++;
			}
		}
	}
	DIM_UNLOCK
	ENABLE_AST
	return(found);
}


void dis_send_service(service_id, buffer, size)
register unsigned service_id;
int *buffer;
int size;
{
	register REQUEST *reqp;
	register SERVICE *servp;
	static DIS_PACKET *dis_packet;
	static int packet_size = 0;
	int i, done = 0;

	DISABLE_AST
	DIM_LOCK
	if( !service_id ) {
		print_date_time();
		printf(" Invalid service id\n");
		DIM_UNLOCK
		ENABLE_AST
		return;
	}
	servp = (SERVICE *)id_get_ptr(service_id);
	if(!packet_size)
	{
		dis_packet = (DIS_PACKET *)malloc(DIS_HEADER+size);
		packet_size = DIS_HEADER + size;
	} 
	else 
	{
		if( DIS_HEADER+size > packet_size ) 
		{
			free(dis_packet);
			dis_packet = (DIS_PACKET *)malloc(DIS_HEADER+size);
			packet_size = DIS_HEADER+size;
		}
	}
	reqp = Request_head;
	while( reqp = (REQUEST *) sll_get_next((SLL *) reqp) ) 
	{
		if( reqp->service_ptr == servp ) 
		{
			dis_packet->service_id = htovl(reqp->service_id);
			memcpy(dis_packet->buffer, buffer, size);
			dis_packet->size = htovl(DIS_HEADER + size);
/*
			if(!strcmp(Task_name,"DIS_DNS"))
			{
				if( !dna_write(reqp->conn_id, dis_packet,
						size + DIS_HEADER) )
				{
					release_all_requests( reqp->conn_id );
					print_date_time();
					printf(" Couldn't write to client\n");
				}
			}
			else
			{
*/
				if( !dna_write_nowait(reqp->conn_id, dis_packet,
						size + DIS_HEADER) )
				{
					release_all_requests( reqp->conn_id );
					print_date_time();
					printf(" Couldn't write to client, releasing connection\n");
				}
/*
			}
*/
		}
	}
	DIM_UNLOCK
	ENABLE_AST
}

int dis_remove_service(service_id)
unsigned service_id;
{
	register REQUEST *reqp;
	register SERVICE *servp;
	SERVICE *serv;
	int found = 0;
	int offset;

	DISABLE_AST
	DIM_LOCK
	if(!service_id)
	{
		print_date_time();
		printf(" Invalid service id\n");
		DIM_UNLOCK
		ENABLE_AST
		return(found);
	}
	servp = serv = (SERVICE *)id_get_ptr(service_id);

	/* remove from name server */
	unregister_service(servp);
	/* Release client requests and remove from actual clients */
	if(Request_head) 
	{
		offset = (char *)&(reqp->service_ptr) - (char *)reqp - 4;
		while( reqp = (REQUEST *) sll_search_next_remove((SLL *) Request_head,
							 offset, (char *) &serv, sizeof(void *)) )
		{
			if(reqp->timr_ent)
				dtq_rem_entry(Dis_timer_q, reqp->timr_ent);
			remove_service(reqp->req_id);
			id_free(reqp->req_id);
			free(reqp);
			found = 1;
		}
	}
	/* remove itself */
	sll_remove((SLL *)Service_head, (SLL *)servp);
	id_free(servp->id);
	free(servp);
	DIM_UNLOCK
	ENABLE_AST
	return(found);
}

/* find service by name */
SERVICE *find_service(name)
char *name;
{
	SERVICE *servp;

	servp = (SERVICE *)
		    sll_search( (SLL *) Service_head, name, strlen(name)+1);
	return(servp);
}

release_all_requests(conn_id)
int conn_id;
{
	register REQUEST *reqp, *prevp;

	DISABLE_AST;
	while( reqp = (REQUEST *) sll_search_next_remove((SLL *) Request_head,
							 0, (char *) &conn_id, 4) )
	{
		if(reqp->timr_ent)
			dtq_rem_entry(Dis_timer_q, reqp->timr_ent);
		id_free(reqp->req_id);
		reqp->service_ptr = 0;
		free(reqp);
	}
	dna_close(conn_id);
	ENABLE_AST;
}

release_request(conn_id, service_id)
int conn_id;
int service_id;
{
	register REQUEST *reqp, *prevp;
	int data[2];

	data[0] = conn_id;
	data[1] = service_id;
	if( reqp = (REQUEST *) sll_search_next_remove((SLL *) Request_head,
						      0, (char *) data, 8) )
	{
		if( reqp->timr_ent )
			dtq_rem_entry( Dis_timer_q, reqp->timr_ent );
		id_free(reqp->req_id);
		free( reqp );
	}
}

typedef struct cmnds{
	struct cmnds *next;
	int tag;
	int size;
	int buffer[MAX_CMND];
} DIS_CMND;

static DIS_CMND *Cmnds_head = (DIS_CMND *)0;

void std_cmnd_handler(tag, cmnd_buff, size)
int *tag, *cmnd_buff, *size;
{
	register DIS_CMND *new_cmnd;
/* queue the command */

	if(!Cmnds_head)
	{
		Cmnds_head = (DIS_CMND *)malloc(sizeof(DIS_CMND));
		sll_init((SLL *) Cmnds_head);
	}
	new_cmnd = (DIS_CMND *)malloc(sizeof(DIS_CMND));
	new_cmnd->next = 0;
	new_cmnd->tag = *tag;
	new_cmnd->size = *size;
	memcpy(new_cmnd->buffer, cmnd_buff, *size);
	sll_insert_queue((SLL *) Cmnds_head, (SLL *) new_cmnd);
}

int dis_get_next_cmnd(tag, buffer, size)
int *tag, *buffer, *size;
{
	register DIS_CMND *cmndp;
	register int ret_val = -1;

	DISABLE_AST
	DIM_LOCK
	if(!Cmnds_head)
	{
		Cmnds_head = (DIS_CMND *)malloc(sizeof(DIS_CMND));
		sll_init((SLL *) Cmnds_head);
	}
	if(cmndp = (DIS_CMND *) sll_remove_head((SLL *) Cmnds_head))
	{
		if (*size >= cmndp->size)
		{
			*size = cmndp->size;
			ret_val = 1;
		}
		memcpy(buffer, cmndp->buffer, *size);
		*tag = cmndp->tag;
		free(cmndp);
		DIM_UNLOCK
		ENABLE_AST
		return(ret_val);
	}
	DIM_UNLOCK
	ENABLE_AST
	return(0);
}

#ifdef VMS
dis_convert_str(c_str, for_str)
char *c_str;
struct dsc$descriptor_s *for_str;
{
	int i;

	strcpy(for_str->dsc$a_pointer, c_str);
	for(i = strlen(c_str); i< for_str->dsc$w_length; i++)
		for_str->dsc$a_pointer[i] = ' ';
}
#endif

void client_info(tag, bufp, size)
int *tag;
int **bufp;
int *size;
{
	register REQUEST *reqp;
	int curr_conns[MAX_CONNS];
	int i, index, found, max_size;
	static int curr_allocated_size = 0;
	static DNS_CLIENT_INFO *dns_info_buffer;
	register DNS_CLIENT_INFO *dns_client_info;

	index = 0;
	reqp = Request_head;
	while(reqp = (REQUEST *) sll_get_next((SLL *) reqp))
	{
		found = 0;
		for(i=0;i<index; i++)
		{
			if(reqp->conn_id == curr_conns[i])
			{
				found = 1;
				break;
			}
		}
		if(!found)
			curr_conns[index++] = reqp->conn_id;
	}
	
	max_size = (index+1)*sizeof(DNS_CLIENT_INFO);
	if(!curr_allocated_size)
	{
		dns_info_buffer = (DNS_CLIENT_INFO *)malloc(max_size);
		curr_allocated_size = max_size;
	}
	else if (max_size > curr_allocated_size)
	{
		free(dns_info_buffer);
		dns_info_buffer = (DNS_CLIENT_INFO *)malloc(max_size);
		curr_allocated_size = max_size;
	}
	dns_client_info = dns_info_buffer;
	for(i=0; i<index;i++)
	{
		dna_get_node_task(curr_conns[i], dns_client_info->node,
										dns_client_info->task);
		dns_client_info++;
	}
	dns_client_info->node[0] = '\0';
	*bufp = (int *)dns_info_buffer;
	*size = max_size;

}
		
void service_info(tag, bufp, size)
int *tag;
int **bufp;
int *size;
{
	register SERVICE *servp;
	int i, index, found, max_size;
	static int curr_allocated_size = 0;
	static char *service_info_buffer;

	max_size = Dis_n_services * (MAX_NAME*2 + 4);
	if(!curr_allocated_size)
	{
		service_info_buffer = (char *)malloc(max_size);
		curr_allocated_size = max_size;
	}
	else if (max_size > curr_allocated_size)
	{
		free(service_info_buffer);
		service_info_buffer = (char *)malloc(max_size);
		curr_allocated_size = max_size;
	}
	service_info_buffer[0] = '\0';

	servp = Service_head;
	while( servp = (SERVICE *) sll_get_next((SLL *) servp) ) 
	{
		strcat(service_info_buffer, servp->name);
		strcat(service_info_buffer, "|");
		if(servp->def[0])
		{
			strcat(service_info_buffer, servp->def);
		}
		strcat(service_info_buffer, "|");
		if(servp->type == COMMAND)
		{
			strcat(service_info_buffer, "C");
		}
		strcat(service_info_buffer, "\n");
	}
	*bufp = (int *)service_info_buffer;
	*size = strlen(service_info_buffer);
}
		
#ifdef VMS
/* CFORTRAN WRAPPERS */
FCALLSCFUN1(INT, dis_start_serving, DIS_START_SERVING, dis_start_serving,
                 STRING)
FCALLSCFUN3(INT, dis_get_next_cmnd, DIS_GET_NEXT_CMND, dis_get_next_cmnd,
                 PINT, PVOID, PINT)
FCALLSCFUN6(INT, dis_add_service, DIS_ADD_SERVICE, dis_add_service,
                 STRING, PVOID, PVOID, INT, PVOID, INT)
FCALLSCSUB4(     dis_add_cmnd, DIS_ADD_CMND, dis_add_cmnd,
                 STRING, PVOID, PVOID, INT)
FCALLSCSUB1(     dis_report_service, DIS_REPORT_SERVICE, dis_report_service,
                 STRING)
FCALLSCSUB2(     dis_convert_str, DIS_CONVERT_STR, dis_convert_str,
                 PVOID, PVOID)
FCALLSCFUN1(INT, dis_update_service, DIS_UPDATE_SERVICE, dis_update_service,
                 INT)
FCALLSCFUN1(INT, dis_remove_service, DIS_REMOVE_SERVICE, dis_remove_service,
                 INT)
FCALLSCSUB3(     dis_send_service, DIS_SEND_SERVICE, dis_send_service,
                 INT, PVOID, INT)
#endif
