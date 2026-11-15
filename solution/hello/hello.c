#include <stdio.h>

struct point
{
	int x;
	int y;
};

int mega_table[1000] = { 5, 6, 7 };
struct point table[] = {
		{1, 0xAA}, {2, 0xBB}, {3, 0xCC}, {4, 0xDD}, {5, 0xEE}, {6, 0xFF}, {7, 7}, {8, 0},
		{1, 0xAA}, {2, 0xBB}, {3, 0xCC}, {4, 0xDD}, {5, 0xEE}, {6, 0xFF}, {7, 7}, {8, 0},
		{1, 0xAA}, {2, 0xBB}, {3, 0xCC}, {4, 0xDD}, {5, 0xEE}, {6, 0xFF}, {7, 7}, {8, 0},
		{1, 0xAA}, {2, 0xBB}, {3, 0xCC}, {4, 0xDD}, {5, 0xEE}, {6, 0xFF}, {7, 7}, {8, 0},
		{1, 0xAA}, {2, 0xBB}, {3, 0xCC}, {4, 0xDD}, {5, 0xEE}, {6, 0xFF}, {7, 7}, {8, 0} };

int BEGIN()
{
	return 0;
}

int add_magic(int x, int y)
{
	int u = 0;

	
	printf("\nu        = 0x%x!\n", *(&u + 0));
	printf("\nx        = 0x%x!\n", *(&u + 1));
	printf("\nprev_ebp = 0x%x!\n", *(&u + 2));
	printf("\nret_addr = 0x%x!\n", *(&u + 3));
	printf("\nx        = 0x%x!\n", *(&u + 4));
	printf("\ny        = 0x%x!\n", *(&u + 5));


	u = x + y;
	printf("\nU= %d!\n", u);
	return u;
}

int test(void)
{
	printf("\nHello World %d!\n", 10);
	return 0;
}

int test2(void);
int END(void);

int main(void)
{       
	int y = 0;
	
	y = add_magic(0xAAAAAAAA, 0xBBBBBBBB);

	printf("\nHello World!\n");
	test();
	printf("\x07"); fflush(stdout);
	printf("\007"); fflush(stdout);
	test2();
	return 0;
}

int test2(void)
{
	while (1);
	return 0;
}

int END()
{
	return 0;
}

