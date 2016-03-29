/*
 * DNS (Delphi Name Server) Package implements the name server for the DIM
 * (Delphi Information Management) system
 *
 * Started date      : 26-10-92
 * Last modification : 02-08-94
 * Written by        : C. Gaspar
 * Adjusted by       : G.C. Ballintijn
 *
 * $Id: dns.c,v 1.1 1994/07/18 19:50:03 gerco Exp gerco $
 */

#include <stdio.h>
#include <dim.h>
#include <dis.h>

#define MAX_HASH_ENTRIES 2000
FILE	*foutptr;

typedef struct node {
	struct node *client_next;
	struct node *client_prev;
	struct node *next;
	struct node *prev;
	int conn_id;
	int service_id;
	struct serv *servp;
} NODE;

typedef struct red_node {
	struct red_node *next;
	struct red_node *prev;
	int conn_id;
	int service_id;
	struct serv *servp;
} RED_NODE;

typedef struct serv {
	struct serv *server_next;
	struct serv *server_prev;
	struct serv *next;
	struct serv *prev;
	char serv_name[MAX_NAME];
	char serv_def[MAX_NAME];
	int state;
	int conn_id;
	int server_format;
	int serv_id;
	RED_NODE *node_head;
} DNS_SERVICE;

typedef struct red_serv {
	struct red_serv *next;
	struct red_serv *prev;
	char serv_name[MAX_NAME];
	char serv_def[MAX_NAME];
	int state;
	int conn_id;
	int server_format;
	int serv_id;
	RED_NODE *node_head;
} RED_DNS_SERVICE;

static RED_DNS_SERVICE *Service_hash_table[MAX_HASH_ENTRIES];
static int Curr_n_services = 0;
static int Curr_n_servers = 0;
static int Last_conn_id;
/*
static int Debug = TRUE;
*/
static int Debug = FALSE;

static int Timer_q;
int Service_info_id, Server_info_id, wake_up;

_PROTO( DNS_SERVICE *service_exists, (char *name) );
_PROTO( void check_validity,         (int conn_id) );
_PROTO( void send_dns_server_info,   (int conn_id, int **bufp, int *size) );
_PROTO( void print_stats,            (void) );
_PROTO( void set_debug_on,           (void) );
_PROTO( void set_debug_off,          (void) );
_PROTO( void kill_servers,           (void) );
_PROTO( void print_hash_table,       (void) );
_PROTO( static void release_conn,    (int conn_id) );


static void recv_rout( conn_id, packet, size, status )
int conn_id, size, status;
DIC_DNS_PACKET *packet;
{

	switch(status)
	{
	case STA_DISC:     /* connection broken */
/*
		if(Debug)
		{
			print_date_time();
			printf("Disconnect received - conn: %d to %s@%s\n", conn_id,
				Net_conns[conn_id].task,Net_conns[conn_id].node );
		}
*/
		release_conn( conn_id );
		break;
	case STA_CONN:     /* connection received */
		/* handle_conn( conn_id ); */
		break;
	case STA_DATA:     /* normal packet */
		switch( vtohl(packet->src_type) )
		{
		case SRC_DIS :
			handle_registration(conn_id, (DIS_DNS_PACKET *)packet, 1);
			break;
		case SRC_DIC :
			handle_client_request(conn_id,(DIC_DNS_PACKET *)packet);
			break;
		default:
			printf("conn: %d to %s@%s, Bad packet\n", conn_id,
				Net_conns[conn_id].task,Net_conns[conn_id].node );
			printf("packet->size = %d\n", vtohl(packet->size));
			printf("packet->src_type = %d\n", vtohl(packet->src_type));
			printf( "closing the connection.\n" );
			release_conn( conn_id );
/*
			panic( "recv_rout(): Bad switch(1)" );
*/
		}
		break;
	default:
		printf( "recv_rout(): Bad switch(2), exiting...\n" );
		exit(0);
	}
}


handle_registration( conn_id, packet, tmout_flag )
int conn_id;
DIS_DNS_PACKET *packet;
int tmout_flag;
{
	DNS_SERVICE *servp;
	DNS_DIS_PACKET dis_packet;
	int i, service_id;
	int format;

	Dns_conns[conn_id].validity = time(NULL);
	if( !Dns_conns[conn_id].service_head ) 
	{
		Dns_conns[conn_id].service_head =
			(char *) malloc(sizeof(DNS_SERVICE));
		dll_init( (DLL *) Dns_conns[conn_id].service_head );
		Dns_conns[conn_id].n_services = 0;
		Dns_conns[conn_id].timr_ent = NULL;
		Curr_n_servers++;
		Dns_conns[conn_id].src_type = SRC_DIS;
		Dns_conns[conn_id].protocol = vtohl(packet->protocol);
		strncpy( Dns_conns[conn_id].node_name, packet->node_name,
			MAX_NODE_NAME ); 
		strncpy( Dns_conns[conn_id].task_name, packet->task_name,
			MAX_TASK_NAME ); 
		Dns_conns[conn_id].port = vtohl(packet->port);
		if(tmout_flag)
			Dns_conns[conn_id].timr_ent = dtq_add_entry( Timer_q,
				(int)(WATCHDOG_TMOUT_MAX * 1.2), check_validity, conn_id);
/*
#ifdef VMS
		format = vtohl(packet->format);
		if( format & MY_OS9)
		{
			dna_set_test_write(conn_id, 5);
		}
#endif
*/
	} 
	else 
	{
		if( (Dns_conns[conn_id].n_services == -1) &&
		    vtohl(packet->n_services) )
		{
			print_date_time();
			printf( " Server %s out of error\n",
				Dns_conns[conn_id].task_name );
			Dns_conns[conn_id].n_services = 0;
/*
			if(tmout_flag)
				Dns_conns[conn_id].timr_ent =
					dtq_add_entry(  Timer_q,
						(int)(WATCHDOG_TMOUT_MAX * 1.2),
						check_validity, conn_id );
*/
		}
	}
/*
	printf("Server %s on node %s registered %d services\n",
			Dns_conns[conn_id].task_name, Net_conns[conn_id].node,
			vtohl(packet->n_services));
*/
	for( i = 0; i < vtohl(packet->n_services); i++ ) 
	{
		if(servp = service_exists(packet->services[i].service_name))
		{

			/* if service available on another server send kill signal */
			if((servp->conn_id) && (servp->conn_id != conn_id))
			{
				dis_packet.type = htovl(DNS_DIS_KILL);
				dis_packet.size = htovl(DNS_DIS_HEADER);
				format = vtohl(packet->format);
#ifdef VMS
				if((format & MY_OS9) || (servp->state == -1))
				{
					if( !dna_write(servp->conn_id, &dis_packet, DNS_DIS_HEADER) )
					{
						printf("Couldn't write, releasing %d\n",servp->conn_id);
/*
						release_conn(servp->conn_id);
*/
					}
					print_date_time();
					printf(" Service %s already declared, killing server %s\n",
						servp->serv_name, Dns_conns[servp->conn_id].task_name);
          
					release_client(servp->conn_id);
					release_conn(servp->conn_id);
				}
				else
				{
#endif
					if( !dna_write(conn_id, &dis_packet, DNS_DIS_HEADER) )
					{
						printf("Couldn't write, releasing %d\n",servp->conn_id);
/*
						release_conn(conn_id);
*/
					}
					release_conn(conn_id);
					print_date_time();
printf(" Service %s already declared by conn %d - %s@%s, killing server %s@%s\n",
						servp->serv_name, servp->conn_id, 
						Dns_conns[servp->conn_id].task_name,
						Dns_conns[servp->conn_id].node_name,
						Dns_conns[conn_id].task_name,
						Dns_conns[conn_id].node_name);
					return(0);
#ifdef VMS
				}
#endif
			}
			else if( servp->state != -1 ) 
			{
				if( !dll_empty((DLL *) servp->node_head)) 
				{
					/*there are interested clients waiting*/
					strncpy( servp->serv_def,
						packet->services[i].service_def,MAX_NAME );
					servp->conn_id = conn_id;
					servp->state = 1;
					servp->server_format = vtohl(packet->format);
					servp->serv_id = vtohl(packet->services[i].service_id);
					dll_insert_queue((DLL *)
						Dns_conns[conn_id].service_head,
						(DLL *) servp);
					Dns_conns[conn_id].n_services++;
					inform_clients(servp);
					continue;
				} 
				else 
				{
					/* test if Service is to be removed */
					service_id = vtohl(packet->services[i].service_id);
					if(service_id & 0x80000000)
					{
						dll_remove((DLL *) servp);
						service_remove(&(servp->next));
						Curr_n_services--;
						free(servp);
						Dns_conns[conn_id].n_services--;
						if( dll_empty((DLL *) Dns_conns[conn_id].service_head))
						{ 
							Curr_n_servers--;
							if( Dns_conns[conn_id].timr_ent ) 
							{
								dtq_rem_entry( Timer_q, 
									Dns_conns[conn_id].timr_ent );
								Dns_conns[conn_id].timr_ent = NULL;
							}
							free((DNS_SERVICE *)Dns_conns[conn_id].service_head);
							Dns_conns[conn_id].service_head = 0;
							Dns_conns[conn_id].src_type = SRC_NONE;
							dna_close(conn_id);
						}
						continue;
                    }
				}
			} 
			else 
			{
				servp->state = 1;
				Dns_conns[conn_id].n_services++;
				if( !dll_empty((DLL *) servp->node_head) )
					inform_clients( servp );
				continue;
			}

		}
		if(!(servp = service_exists(packet->services[i].service_name)))
		{
			servp = (DNS_SERVICE *)malloc(sizeof(DNS_SERVICE));
			strncpy( servp->serv_name,
				packet->services[i].service_name,
				MAX_NAME );
			strncpy( servp->serv_def,
				packet->services[i].service_def,
				MAX_NAME );
			servp->state = 1;
			servp->conn_id = conn_id;
			servp->server_format = vtohl(packet->format);
			servp->serv_id = vtohl(packet->services[i].service_id);
			dll_insert_queue( (DLL *)
					  Dns_conns[conn_id].service_head, 
					  (DLL *) servp );
			Dns_conns[conn_id].n_services++;
			service_insert( &(servp->next) );
			servp->node_head = (RED_NODE *) malloc(sizeof(NODE));
			dll_init( (DLL *) servp->node_head );
			Curr_n_services++;
		} 
	}
	if( (vtohl(packet->n_services) > 0) &&
		(vtohl(packet->n_services) < MAX_SERVICE_UNIT) ) 
	{
		Last_conn_id = conn_id;
		dis_update_service(Server_info_id);
	}
	if( Debug )
		if(vtohl(packet->n_services) != 0)
		{
			print_date_time();
			printf( "Conn %3d : Server %s@%s registered %d services\n",
				conn_id, Dns_conns[conn_id].task_name,
				Dns_conns[conn_id].node_name, 
				vtohl(packet->n_services) );
		}
}	


void check_validity(conn_id)
int conn_id;
{
	int time_diff;
	DNS_DIS_PACKET dis_packet;

	if(Dns_conns[conn_id].validity < 0)
	{
		/* timeout reached kill all services and connection */
		if(Dns_conns[conn_id].n_services != -1)
		{
			print_date_time();
			printf(" Server %s has been set in error\n",
				Dns_conns[conn_id].task_name);
			set_in_error(conn_id);
			return;
		}
		Dns_conns[conn_id].validity = -Dns_conns[conn_id].validity;
	}
	time_diff = time(NULL) - Dns_conns[conn_id].validity;
	if(time_diff > (int)(WATCHDOG_TMOUT_MAX*1.1))
	{
		/* send register signal */
		dis_packet.type = htovl(DNS_DIS_REGISTER);
		dis_packet.size = htovl(DNS_DIS_HEADER);
		if( !dna_write(conn_id, &dis_packet, DNS_DIS_HEADER) )
		{
			printf("Validity : Couldn't write, releasing %d\n",conn_id);
			release_conn(conn_id);
		}
/*
		if(Debug)
		{
			print_date_time();
			printf(" Server %s not responding, requesting registration\n",
				Dns_conns[conn_id].task_name);
		}
*/
		Dns_conns[conn_id].validity = -Dns_conns[conn_id].validity;
	}
}		


handle_client_request( conn_id, packet )
int conn_id;
DIC_DNS_PACKET *packet;
{
	DNS_SERVICE *servp;
	NODE *nodep;
	RED_NODE *red_nodep; 
	int i;
	DNS_DIC_PACKET dic_packet;
	SERVICE_REG *serv_regp; 
	char *ptr;

	serv_regp = (SERVICE_REG *)(&(packet->service));
	if(Debug)
	{
		print_date_time();
		printf("Conn %3d : Client %s@%s requested %s\n",
			conn_id, Net_conns[conn_id].task, Net_conns[conn_id].node,
			serv_regp->service_name);
	}
	if( vtohl(serv_regp->service_id) == -1 )  /* remove service */
	{
		if(Debug)
			printf("\tRemoving Request\n");
		if( servp = service_exists(serv_regp->service_name) ) 
		{
			red_nodep = servp->node_head;
			while( red_nodep =
				(RED_NODE *) dll_get_next(
						(DLL *) servp->node_head,
						(DLL *) red_nodep) )
			{
				if( red_nodep->conn_id == conn_id ) 
				{
					dll_remove((DLL *) red_nodep);
					ptr = (char *)red_nodep - (2 * sizeof(void *));
					nodep = (NODE *)ptr;
					dll_remove((DLL *) nodep);
					red_nodep = red_nodep->prev;
					free(nodep);
					break;
				}
			}
			if(( dll_empty((DLL *) servp->node_head) ) && (servp->state == 0))
			{
				if(Debug)
					printf("\tand Removing Service\n");
				service_remove(&(servp->next));
				Curr_n_services--;
				free(servp);
			}
		}
		return(0);
	}
	/* Is already in v.format */
	dic_packet.service_id = serv_regp->service_id;
	dic_packet.node_name[0] = 0; 
	dic_packet.task_name[0] = 0;
	dic_packet.size = htovl(DNS_DIC_HEADER);
	if( !(servp = service_exists(serv_regp->service_name)) ) 
	{
		if(Debug)
		{
			printf("\tService does not exist, queueing request\n");
			if(strstr(serv_regp->service_name,"TCP_SC/AL_HV"))
			{
			}
		}
		if( !Dns_conns[conn_id].node_head ) 
		{
			Dns_conns[conn_id].src_type = SRC_DIC;
			Dns_conns[conn_id].node_head =
					malloc(sizeof(NODE));
			dll_init( (DLL *) Dns_conns[conn_id].node_head );
		}
		servp = (DNS_SERVICE *) malloc(sizeof(DNS_SERVICE));
		strncpy( servp->serv_name, serv_regp->service_name, MAX_NAME );
		servp->serv_def[0] = '\0';
		servp->state = 0;
		servp->conn_id = 0;
		service_insert(&(servp->next));
		Curr_n_services++;
		servp->node_head = (RED_NODE *)malloc(sizeof(NODE));
		dll_init( (DLL *) servp->node_head );
		nodep = (NODE *)malloc(sizeof(NODE));
		nodep->conn_id = conn_id;
		nodep->service_id = vtohl(serv_regp->service_id);
		nodep->servp = servp;
		dll_insert_queue((DLL *) Dns_conns[conn_id].node_head,
				 (DLL *) nodep);
		dll_insert_queue((DLL *) servp->node_head,
				 (DLL *) &(nodep->next));
	} 
	else 
	{
		if( servp->state == 1 ) 
		{
#ifdef VMS
			if(servp->server_format & MY_OS9)
			{
				dna_test_write(servp->conn_id);
			}
#endif
			Dns_conns[conn_id].src_type = SRC_DIC;
			strcpy( dic_packet.node_name,
				Dns_conns[servp->conn_id].node_name );
			strcpy( dic_packet.task_name,
				Dns_conns[servp->conn_id].task_name );
			dic_packet.port = htovl(Dns_conns[servp->conn_id].port);
			dic_packet.protocol = htovl(Dns_conns[servp->conn_id].protocol);
			dic_packet.format = htovl(servp->server_format);
			strcpy( dic_packet.service_def, servp->serv_def );
			if(Debug)
				printf("\tService exists in %s@%s\n",
					dic_packet.task_name, dic_packet.node_name);
		} 
		else 
		{
			if(Debug)
				printf("\tService exists in BAD state, queueing request\n");
			if(!(NODE *)Dns_conns[conn_id].node_head ) 
			{
				Dns_conns[conn_id].src_type = SRC_DIC;
				Dns_conns[conn_id].node_head = 
					(char *) malloc(sizeof(NODE));
				dll_init((DLL *)Dns_conns[conn_id].node_head);
			}
			nodep = (NODE *)malloc(sizeof(NODE));
			nodep->conn_id = conn_id;
			nodep->service_id = vtohl(serv_regp->service_id);
			nodep->servp = servp;
			dll_insert_queue((DLL *) Dns_conns[conn_id].node_head,
					 (DLL *) nodep);
			dll_insert_queue((DLL *) servp->node_head,
					 (DLL *) &(nodep->next));
		}
	}
	if( !dna_write(conn_id, &dic_packet, DNS_DIC_HEADER) )
	{
		printf("Handle req : Couldn't write, releasing %d\n",conn_id);
		release_conn(conn_id);
	}
}


inform_clients(servp)
DNS_SERVICE *servp;
{
	RED_NODE *nodep; 
	NODE *full_nodep; 
	DNS_DIC_PACKET packet;
	char *ptr;

	nodep = servp->node_head;
/*
	if(Debug)
	{
		print_date_time();
		printf("Informing Clients of %s at %s@%s:\n",servp->serv_name,
			Dns_conns[servp->conn_id].task_name,
			Dns_conns[servp->conn_id].node_name);
	}
*/
	while( nodep = (RED_NODE *) dll_get_next((DLL *) servp->node_head,
						 (DLL *) nodep) )
	{
		packet.service_id = htovl(nodep->service_id);
		strcpy(packet.node_name, Dns_conns[servp->conn_id].node_name);
		strcpy(packet.task_name, Dns_conns[servp->conn_id].task_name);
		packet.port = htovl(Dns_conns[servp->conn_id].port);
		packet.protocol = htovl(Dns_conns[servp->conn_id].protocol);
		packet.size = htovl(DNS_DIC_HEADER);
		packet.format = htovl(servp->server_format);
		strcpy( packet.service_def, servp->serv_def );
/*
		if(Debug)
		{
			print_date_time();
			printf("\tconn %d - %s@%s\n",nodep->conn_id,
				Net_conns[nodep->conn_id].task, Net_conns[nodep->conn_id].node);
		}
*/
		if(!dna_write(nodep->conn_id, &packet, DNS_DIC_HEADER))
		{
			printf("inform clients : Couldn't write, releasing %d\n",
				nodep->conn_id);
			release_conn(nodep->conn_id);
		}
		else 
		{
			dll_remove( (DLL *) nodep );
			ptr = (char *)nodep - (2 * sizeof(void *));
			full_nodep = (NODE *)ptr;
			dll_remove( (DLL *) full_nodep );
			nodep = nodep->prev;
			free( full_nodep );
		}
	}
}

#ifdef VMS
static release_client(conn_id)
int conn_id;
{
char *ptr_task;
char *ptr_node;
int i;

	ptr_task = Net_conns[conn_id].task;
	ptr_node = Net_conns[conn_id].node;
	for( i = 0; i< Curr_N_Conns; i++ )
	{
		if( (!strcmp(Net_conns[i].task,ptr_task)) &&
		    (!strcmp(Net_conns[i].node,ptr_node)) )
		{
			if(i != conn_id)
			{
				if( Dns_conns[i].src_type == SRC_DIC ) 
				{
					if(Debug)
					{
						print_date_time();
						printf(" Releasing client on conn %d - %s@%s\n",
							i, Net_conns[i].task, Net_conns[i].node);
					}
					release_conn(i);
				}
			}
		}
	}
}
#endif

static void release_conn(conn_id)
int conn_id;
{
	DNS_SERVICE *servp, *old_servp;
	NODE *nodep, *old_nodep;

	if( Dns_conns[conn_id].src_type == SRC_DIS ) 
	{
		if( Debug )
		{
			print_date_time();
			printf( "Conn %3d : Server %s@%s died\n",
				conn_id, Dns_conns[conn_id].task_name,
				Dns_conns[conn_id].node_name);
		}
		if(Dns_conns[conn_id].n_services == -1)
		{
			printf( "Conn %3d : Server %s@%s died\n",
				conn_id, Dns_conns[conn_id].task_name,
				Dns_conns[conn_id].node_name);
		}
		Curr_n_servers--;
		if( Dns_conns[conn_id].timr_ent ) 
		{
			dtq_rem_entry( Timer_q, Dns_conns[conn_id].timr_ent );
			Dns_conns[conn_id].timr_ent = NULL;
		}
		servp = (DNS_SERVICE *)Dns_conns[conn_id].service_head;
		while( servp = (DNS_SERVICE *) dll_get_next(
				(DLL *) Dns_conns[conn_id].service_head,
				(DLL *) servp) )
		{
			dll_remove((DLL *) servp);
			if(dll_empty((DLL *) servp->node_head)) 
			{
				service_remove(&(servp->next));
				Curr_n_services--;
				old_servp = servp;
				servp = servp->server_prev;
				free(old_servp);
			} 
			else 
			{
				servp->state = 0;
				servp->conn_id = 0;
				servp = servp->server_prev;
			}
		}
		if(Dns_conns[conn_id].n_services)
		{
			Dns_conns[conn_id].n_services = 0;
			Last_conn_id = conn_id;
			dis_update_service(Server_info_id);
		}
		free((DNS_SERVICE *)Dns_conns[conn_id].service_head);
		Dns_conns[conn_id].service_head = 0;
		Dns_conns[conn_id].src_type = SRC_NONE;
		dna_close(conn_id);
	}
	else if(Dns_conns[conn_id].src_type == SRC_DIC)
	{
		if(Debug)
		{
			print_date_time();
			printf("Conn %3d : Client %s@%s died\n",
				conn_id, Net_conns[conn_id].task, Net_conns[conn_id].node);
		}
		if( nodep = (NODE *)Dns_conns[conn_id].node_head ) 
		{
			while( nodep = (NODE *) dll_get_next(
					(DLL *) Dns_conns[conn_id].node_head,
					(DLL *) nodep) )
			{
				servp = nodep->servp;
				dll_remove( (DLL *) nodep );
				dll_remove( (DLL *) &(nodep->next) );
				old_nodep = nodep;
				nodep = nodep->client_prev;
				free(old_nodep);
				if( (dll_empty((DLL *) servp->node_head)) &&
				    (!servp->conn_id) )
				{
					service_remove(&(servp->next));
					Curr_n_services--;
					free( servp );
				}
			}
			free(Dns_conns[conn_id].node_head);
			Dns_conns[conn_id].node_head = 0;
		}
		Dns_conns[conn_id].src_type = SRC_NONE;
		dna_close(conn_id);
	} 
	else 
	{
		if(Debug)
		{
			print_date_time();
			printf("Conn %3d : ERROR - Strange thing %s@%s died\n",
				conn_id, Net_conns[conn_id].task,
				Net_conns[conn_id].node);
		}
		dna_close(conn_id);
	}
}


set_in_error(conn_id)
{
	DNS_SERVICE *servp;

	if(Dns_conns[conn_id].src_type == SRC_DIS)
	{
		servp = (DNS_SERVICE *)Dns_conns[conn_id].service_head;
		while( servp = (DNS_SERVICE *) dll_get_next(
				(DLL *) Dns_conns[conn_id].service_head,
				(DLL *) servp) )
			servp->state = -1;
/*
		if( Dns_conns[conn_id].timr_ent ) 
		{
			dtq_rem_entry( Timer_q, Dns_conns[conn_id].timr_ent );
			Dns_conns[conn_id].timr_ent = NULL;
		}
*/
		Dns_conns[conn_id].n_services = -1;
		Last_conn_id = conn_id;
		dis_update_service(Server_info_id);
	}
}


void get_dns_server_info(tag, bufp, size, first_time)
int *tag;
int **bufp;
int *size;
int *first_time;
{
	int i;
	
	if(*first_time)
	{

#ifdef VMS
		 sys$wake(0, 0); 	/* #### Veranderen #### */
#else
		wake_up = TRUE;
#endif
		*size = 0;

	}
	else
	{
		send_dns_server_info(Last_conn_id, bufp, size);
	}
}


void send_dns_server_info(conn_id, bufp, size)
int conn_id;
int **bufp;
int *size;
{
	static int curr_allocated_size = 0;
	static DNS_DID *dns_info_buffer;
	DNS_SERVICE *servp;
	DNS_SERVER_INFO *dns_server_info;
	DNS_SERVICE_INFO *dns_service_info;
	DNS_CONNECTION *connp;
	int i, max_size;
	int n_services;

	connp = &Dns_conns[conn_id];
	if(connp->src_type != SRC_DIS)
		return;
	n_services = connp->n_services;
	if(n_services == -1)
		n_services = 0;
	max_size = sizeof(DNS_SERVER_INFO) + 
				n_services * sizeof(DNS_SERVICE_INFO);  
	if(!curr_allocated_size)
	{
		dns_info_buffer = (DNS_DID *)malloc(max_size);
		curr_allocated_size = max_size;
	}
	else if (max_size > curr_allocated_size)
	{
		free(dns_info_buffer);
		dns_info_buffer = (DNS_DID *)malloc(max_size);
		curr_allocated_size = max_size;
	}
	dns_server_info = &dns_info_buffer->server;
	dns_service_info = dns_info_buffer->services;
	strncpy(dns_server_info->task, connp->task_name, MAX_TASK_NAME);
	strncpy(dns_server_info->node, connp->node_name, MAX_NODE_NAME);
	dns_server_info->n_services = htovl(connp->n_services);
	servp = (DNS_SERVICE *)connp->service_head;
	while( servp = (DNS_SERVICE *) dll_get_next((DLL *) connp->service_head,
						    (DLL *) servp) )
	{
		strncpy(dns_service_info->name, servp->serv_name, MAX_NAME); 
		dns_service_info->status = htovl(1);
		dns_service_info++;
	}
	*bufp = (int *)dns_info_buffer;
	*size = max_size;
}

main()
{
	int i, protocol, dns_port;
	int *bufp;
	int size;
	int conn_id, id;
	DIS_DNS_PACKET *dis_dns_packet;

	conn_arr_create( SRC_DNS );
	service_init();
	Timer_q = dtq_create();

	Server_info_id = dis_add_service( "DIS_DNS/SERVER_INFO", 0, 0, 0, 
						get_dns_server_info, 0 );
	dis_add_cmnd( "DIS_DNS/PRINT_STATS", 0, print_stats, 0 );
	dis_add_cmnd( "DIS_DNS/DEBUG_ON", 0, set_debug_on, 0 );
	dis_add_cmnd( "DIS_DNS/DEBUG_OFF", 0, set_debug_off, 0 );
	dis_add_cmnd( "DIS_DNS/KILL_SERVERS", 0, kill_servers, 0 );
	dis_add_cmnd( "DIS_DNS/PRINT_HASH_TABLE", 0, print_hash_table, 0 );
	dns_port = DNS_PORT;
	if( !dna_open_server(DNS_TASK, recv_rout, &protocol, &dns_port) )
		return(0);

	id = dis_start_serving("DIS_DNS");
	dis_dns_packet = (DIS_DNS_PACKET *) id_get_ptr(id);
	id_free(id);
	conn_id = conn_get();
	handle_registration(conn_id, dis_dns_packet, 0);

	while(1)
	{
#ifdef VMS
		sys$hiber(); 	/* #### veranderen #### */
#else
		wake_up = FALSE;
		while( !wake_up )
                {
			pause();
                }
#endif

 		for( i = 0; i< Curr_N_Conns; i++ )
		{
			if( Dns_conns[i].src_type == SRC_DIS )
			{
				send_dns_server_info( i, &bufp, &size );
				dis_send_service( Server_info_id, bufp, size );
			}
		}

	}
}


void print_stats()
{
	int i;
	int n_conns = 0;
	int n_services = 0;
	int n_servers = 0;
	int n_clients = 0;

	for(i = 0; i< Curr_N_Conns; i++)
	{
		switch(Dns_conns[i].src_type)
		{
		case SRC_DIS :
			printf("%d - Server %s@%s, %d services\n",
				i, Dns_conns[i].task_name,
				Dns_conns[i].node_name,
				Dns_conns[i].n_services );
			n_services +=  Dns_conns[i].n_services;
			n_servers++;
			n_conns++;
			break;
		case SRC_DIC :
			printf("%d - Client %s@%s\n",
				i, Net_conns[i].task, Net_conns[i].node); 
			n_conns++;
			n_clients++;
			break;
		default :
			if(Dna_conns[i].busy)
			{
				if(Net_conns[i].task[0] && Net_conns[i].node[0])
					printf("%d - Busy - %s@%s\n",
						i, Net_conns[i].task,
						Net_conns[i].node);
				else 
					printf("%d - Busy\n", i);
				n_conns++;
			}
			else
				printf("%d - Empty\n", i);
		}
	}
	printf("Number of Connections = %d : %d servers, %d clients\n", n_conns,
		n_servers, n_clients);
	printf("Number of Services = %d\n", n_services);
}


void set_debug_on()
{
	Debug = 1;
}


void set_debug_off()
{
	Debug = 0;
}


void kill_servers()
{
	int i;
	DNS_DIS_PACKET dis_packet;

	for(i = 0; i< Curr_N_Conns; i++)
	{
		if(Dns_conns[i].src_type == SRC_DIS)
		{
			if(!strcmp(Dns_conns[i].task_name,"DIS_DNS"))
				continue;
			print_date_time();
			printf(" Killing server %s@%s\n",
				Dns_conns[i].task_name, Dns_conns[i].node_name);
			dis_packet.type = htovl(DNS_DIS_KILL);
			dis_packet.size = htovl(DNS_DIS_HEADER);
			if( !dna_write(i, &dis_packet, DNS_DIS_HEADER) )
			{
				printf("Kill : Couldn't write, releasing %d\n",i);
				release_conn(i);
			}
		}
	}
}


service_init()
{
	int i;

	for( i = 0; i < MAX_HASH_ENTRIES; i++ ) {
		Service_hash_table[i] = (RED_DNS_SERVICE *) malloc(8);
		dll_init((DLL *) Service_hash_table[i]);
	}
}


service_insert(servp)
RED_DNS_SERVICE *servp;
{
	int index;

/*
	printf( "servp->serv_name = '%s'\n", servp->serv_name );
*/
	index = HashFunction(servp->serv_name, MAX_HASH_ENTRIES);
	dll_insert_queue((DLL *) Service_hash_table[index], 
			 (DLL *) servp);
}


service_remove(servp)
RED_DNS_SERVICE *servp;
{
	if( servp->node_head )
		free( servp->node_head );
	dll_remove( (DLL *) servp );
}


DNS_SERVICE *service_exists(name)
char *name;
{
	int index;
	RED_DNS_SERVICE *servp;
	char *ptr;

/*
	printf( "name = '%s'\n", name );
*/
	index = HashFunction(name, MAX_HASH_ENTRIES);
	if(servp = (RED_DNS_SERVICE *) dll_search(
					(DLL *) Service_hash_table[index],
			      		name, strlen(name)+1) )
	{
		ptr = (char *)servp - (2 * sizeof(void *));
		return((DNS_SERVICE *)ptr);
	}

	return((DNS_SERVICE *)0);
}			

void print_hash_table()
{
	int i;
	RED_DNS_SERVICE *servp;
	int n_entries, max_entry_index;
	int max_entries = 0;

#ifdef VMS
	if( ( foutptr = fopen( "scratch$week:[cp_operator]dim_dns.log", "w" ) 
		) == (FILE *)0 )
	{
		printf("Cannot open: scratch$week:[cp_operator]dim_dns.log for writing\n");
		return;
	}	
#endif								 
	
	for( i = 0; i < MAX_HASH_ENTRIES; i++ ) 
	{
		n_entries = 0;
#ifdef VMS
		fprintf(foutptr,"HASH[%d] : \n",i);
#endif								 
		servp = Service_hash_table[i];
		while( servp = (RED_DNS_SERVICE *) dll_get_next(
						(DLL *) Service_hash_table[i],
						(DLL *) servp) )
		{
#ifdef VMS
			fprintf(foutptr,"%s\n",servp->serv_name);
#endif								 
			n_entries++;
		}
#ifdef VMS
		fprintf(foutptr,"\n\n");
#endif								 
/*
		printf("HASH[%d] - %d entries\n", i, n_entries);  
*/
		if(n_entries > max_entries)
		{
			max_entries = n_entries;
			max_entry_index = i;
		}
	}
#ifdef VMS
	fclose(foutptr);
#endif								 
	printf("Maximum : HASH[%d] - %d entries\n", max_entry_index, max_entries);  
}
