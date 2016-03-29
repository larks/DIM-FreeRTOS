#include <signal.h>
#include <dis.h>

char str[10][80];

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

TT t;

test_server(arg)
char *arg;
{
	int i, *ptr;
	char aux[80];

	for(i = 0; i< 10; i++)
	{
		sprintf(str[i],"%s/Service_%03d",arg,i);
		dis_add_service( str[i], "C", str[i], strlen(str[i])+1, (void *)0, 0 );
	}
	t.i = 123;
	t.j = 123;
	t.k = 123;
	t.d = 56.78;
	t.s = 12;
	t.t = 12;
	t.u = 12;
	t.f = 4.56;
	ptr = (int *)&t;
	strcpy(t.str,"hello world");
	for(i = 0; i<20; i++)
	{
		printf(" %08X %d\n",*ptr, *ptr);
		ptr++;
	}
	sprintf(aux,"%s/TEST_SWAP",arg);
	dis_add_service( aux, "l:3;d:1;s:3;f:1;c:20", &t, sizeof(t), 
		(void *)0, 0 );

	dis_start_serving( arg );
	while(1) {
		pause();
	} 
}
