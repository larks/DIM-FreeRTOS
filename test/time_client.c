#include <dic.h>

static char str[80], client_str[80], buffer[16*1024];

static char no_link = -1;

static int N_service = 0, Server_down = 0;
static int Service_size = 0;

static void rout( tag, size )
int *tag, *size;
{

	if(buffer[0] == no_link)
	{
		printf("%s Received NO_LINK\n",client_str);
		Server_down = 1;
		return;
	}
	Service_size = *size;
	Server_down = 0;
	N_service++;
	dic_info_service( str, ONCE_ONLY, 10, buffer, 16*1024, rout, 0,
			  &no_link, 1 );
}

static void time_rout()
{

	printf("%s Received %d services/s -> %2.2f Kb/s\n",
		client_str, N_service/10,
		(float)N_service*(float)Service_size/(float)10/(float)1024);
	N_service = 0;
	if(Server_down)
		dic_info_service( str, ONCE_ONLY, 10, buffer, 16*1024, rout, 0,
			  &no_link, 1 );
	dtq_start_timer(10,time_rout,0);
}

time_client(argc,argv)
int argc;
char **argv;
{
	int i;
	char aux[80];

	if(argc != 3)
	{
		printf("Usage : Client <client_name> <server_name>\n");
		exit();
	}
	strcpy(client_str,argv[1]);
	sprintf(str,"%s/TIME_SERVICE",argv[2]);
	dic_info_service( str, ONCE_ONLY, 10, buffer, 16*1024, rout, 0,
			  &no_link, 1 );
	dtq_start_timer(10,time_rout,0);
	while(1)
	{
		pause();
	}
}
