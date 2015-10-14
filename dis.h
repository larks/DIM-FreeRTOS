#ifndef __DISDEFS
#define __DISDEFS

#include "dim_common.h"

int dis_start_serving(char *task_name);
void dis_stop_serving(void);
int dis_get_next_cmnd(dim_long *tag, int *buffer, int *size );
int dis_get_client(char *name );
int dis_get_conn_id(void);
unsigned dis_add_service(char *service_name, char *service_type,
						 void *service_address, int service_size,
						 void (*usr_routine)(void*,void**,int*,int*), dim_long tag);
unsigned dis_add_cmnd(char *service_name, char *service_type,
			          void (*usr_routine)(void*,void*,int*), dim_long tag);
void dis_add_client_exit_handler(void (*usr_routine)(int*));
void dis_set_client_exit_handler(int conn_id, int tag);
void dis_add_exit_handler(void (*usr_routine)(int*));
void dis_add_error_handler(void (*usr_routine)(int, int, char*));
void dis_report_service(char *service_name);
int dis_update_service(unsigned service_id);
int dis_remove_service(unsigned service_id);
void dis_send_service(unsigned service_id, int *buffer, int size);
int dis_set_buffer_size(int size);
void dis_set_quality(unsigned service_id, int quality);
int dis_set_timestamp(unsigned service_id, int secs, int millisecs);
int dis_selective_update_service(unsigned service_id, int *client_id_list);
void dis_disable_padding(void);
int dis_get_timeout(unsigned service_id, int client_id);
char *dis_get_error_services(void);
char *dis_get_client_services(int conn_id);
int dis_start_serving_dns(dim_long dns_id, char *task_name/*, int *id_list*/);
void dis_stop_serving_dns(dim_long dns_id);
unsigned dis_add_service_dns(dim_long dns_id, char *service_name, char *service_type,
							 void *service_address, int service_size,
							 void (*usr_routine)(void*,void**,int*,int*), dim_long tag);
unsigned dis_add_cmnd_dns(dim_long dns_id, char *service_name, char *service_type,
						  void (*usr_routine)(void*,void*,int*), dim_long tag);
int dis_get_n_clients(unsigned service_id);
int dis_get_timestamp(unsigned service_id, int *secs, int *millisecs);
#endif
