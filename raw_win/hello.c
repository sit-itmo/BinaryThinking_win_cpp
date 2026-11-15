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

int test(void)
{
	printf("\nHello World %d!\n", 10);
	return 0;
}

int test2(void);
int END(void);

int main(void)
{
	FILE* p_file = fopen("mycode.bin", "wb");
	char* p_cur = &BEGIN;
	char* p_end = &END;

	while (p_cur < p_end)
	{
		fwrite(p_cur, 1, 1, p_file);
		p_cur++;
	}
	fclose(p_file);

	test();
	printf("\nHello World!\n");
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

