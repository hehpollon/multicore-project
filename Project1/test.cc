#include <stdio.h>
#include <string.h>
int main(void) {

	int num;
	char temp;

	fscanf(stdin, "%d\n", &num);
	printf("%d \n", num);
	temp = fgetc(stdin);
	printf("%c", temp);
	fscanf(stdin, "%d", &num);
	printf("%d ", num);
	
	

    return 0;
}

