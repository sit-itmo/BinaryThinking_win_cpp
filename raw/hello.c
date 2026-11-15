#include <stdio.h>

int testabc(void)
{
	printf("\nHello World %d!\n", 10);
	return 0;
}

int main(void)
{
	printf("\nHello World!\n");
	testabc();
	printf("\nHello World!\n");
	testabc();
	printf("\x07"); fflush(stdout);
	printf("\007"); fflush(stdout);

	return 0;
}

