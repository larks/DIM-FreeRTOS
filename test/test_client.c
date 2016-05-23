#include <dic.h>

static char str[80];
static char str_res[10][80];
static char client_str[80];
static int no_link = -1;
static char buff[80];

typedef struct {
	int i;
	int j;
	int k;
	double d;
	short s;
	short t;
	short u;
	float f;
	char str[20];
}TT;

static TT t;

static void rout( tag, size )
int *tag, *size;
{

	if(*tag == 11)
	{
		printf("Received ONCE_ONLY : %s\n",buff);
		return;
	}
	if(*tag == 10)
	{
		char *ptr; int i; float z;

		printf("t.i = %d, t.d = %2.2f, t.s = %d, t.f = %2.2f, t.str = %s\n",
			t.i,t.d,t.s,t.f,t.str);
		return;
	}
	if(!strcmp(str_res,"No Link"))
		printf("%s Received %s for Service%03d\n",client_str,
			str_res[*tag],*tag);
	else
		printf("%s Received %s\n",client_str, str_res[*tag]);
}

test_client(arg1,arg2)
char *arg1, *arg2;
{
	int i;
	char aux[80];

	strcpy(client_str,arg1);
	
	for(i = 0; i< 10; i++)
	{
		sprintf(str,"%s/Service_%03d",arg2,i);
		dic_info_service( str, TIMED, 10, str_res[i], 80, rout, i,
			  "No Link", 8 );
	}
	
	sprintf(aux,"%s/TEST_SWAP",arg2);
	dic_info_service( aux, TIMED, 10, &t, sizeof(t), rout, 10,
			  &no_link, 4 );

       	while(1)
	{
		pause();
	}
	
}
