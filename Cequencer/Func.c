//#include "Header.h"

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

void print_colors() {
	for (int x = 1; x < 16; x++)
	{
		setColor(x);
		printf("Color %d is this color!\n", x);

	}

}