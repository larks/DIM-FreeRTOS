#include <dis.h>

static char str[80];

static char buffer[16*1024];
static int size;

time_server(argc,argv)
int argc;
char **argv;
{
	int i, *ptr;
	char aux[80];

	if(argc != 3)
	{
		printf("Usage : Server <name> <service_size>\n");
		exit();
	}
	sprintf(str,"%s/TIME_SERVICE",argv[1]);
	sscanf(argv[2],"%d",&size);
	dis_add_service( str, "C", buffer, size, (void *)0, 0 );
	dis_start_serving( argv[1] );
	while(1) {
		pause();
	} 
}
