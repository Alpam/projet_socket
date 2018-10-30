/*
* =====================================================================================
*
*       Filename:  slave_mul.c
*
*    Description:
*
*        Version:  1.0
*        Created:  01/05/2018 00:13:58
*       Revision:  none
*       Compiler:  gcc
*
*         Author:  Paul Robin (), paul.robin@etu.unistra.fr
*
* =====================================================================================
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "common.h"
#include "slave_mul.h"

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("donne ip port\n");
		return 0;
	}
	core_slave("*", argv[1], argv[2], (*multiplication));
	return 0;
}

char *multiplication(char *str, char *id) {
	//fais la multiplication en vérifiant le type et renvoyant en conséquence
	double rflt = 1;
	int rint = 1, i = 0, j = 0, fflag = 0;
	char buf[100];
	char *rtr;
	memset(buf, '\0', 100);
	while (1) {
		if (str[i] == ',' || str[i] == ')') {
			if (fflag) {
				rflt = rflt * atof(buf);
			}
			else {
				rint = rint * atoi(buf);
			}
			if (str[i] == ')') { break; }
			i++;
			memset(buf, '\0', 100);
			j = 0;
		}
		else {
			if (str[i] == '.') {
				fflag = 1;
				rflt = (double)rint;
			}
			buf[j] = str[i];
			j++;
			i++;
		}
	}
	if (fflag) {
		sprintf(buf, "%f", rflt);
	}
	else {
		sprintf(buf, "%d", rint);
	}
	sleep(10);
	int len = strlen(buf) + strlen(id) + 3;
	rtr = malloc(len);
	strcat(rtr, "R ");
	strcat(rtr, id);
	strcat(rtr, " ");
	strcat(rtr, buf);
	return rtr;
}
