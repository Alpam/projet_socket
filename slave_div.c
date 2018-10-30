/*
* =====================================================================================
*
*       Filename:  slave_div.c
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
#include "slave_div.h"

int main(int argc, char **argv) {
	if (argc != 3) {
		printf("donne ip port\n");
		return 0;
	}
	core_slave("/", argv[1], argv[2], (*division));
	return 0;
}

char *division(char *str, char *id) {
	//on vérifie le type des arguments,
	//qu'il n'y en a bien que 2
	//et pas de divison par 0
	int i, j, fflag = 0, nbr_arg = 1, len;
	int mi_r, mi_l;
	float mf_r, mf_l;
	char buf[100];
	char *rtr;
	memset(buf, '\0', 100);
	for(i=0;i<strlen(str);i++){
		if(str[i]=='.'){
			fflag++;
		}
		if(str[i]==','){
			nbr_arg++;
		}
	}
	if(nbr_arg != 2){
		len = strlen(id) + 42;
		rtr = malloc(len);
		strcat(rtr, "I ");
		strcat(rtr, id);
		strcat(rtr, " ");
		strcat(rtr, "error: fonction ne prend que 2 arguments");
		return rtr;
	}
	for(i=0,j=0;i<strlen(str);i++,j++){
		if(str[i]==','){
			if(fflag){
				mf_r = atof(buf);
			}
			else{
				mi_r = atoi(buf);
			}
			j=-1;
			memset(buf, '\0', 100);
		}
		else if(str[i]==')'){
			if(fflag){
					mf_l = atof(buf);
				}
			else{
					mi_l = atoi(buf);
			}
			memset(buf, '\0', 100);
		}
		else{
			buf[j]=str[i];
		}
	}

	if(fflag){
		if(mf_l){
			mf_r = mf_r / mf_l;
			sleep(10);
			sprintf(buf, "%f", mf_r);
			len = strlen(buf) + strlen(id) + 3;
			rtr = malloc(len);
			strcat(rtr, "R ");
			strcat(rtr, id);
			strcat(rtr, " ");
			strcat(rtr, buf);
			return rtr;
		}
	}
	else{
		if(mi_l){
			mi_r = mi_r / mi_l;
			sleep(10);
			sprintf(buf, "%d", mi_r);
			len = strlen(buf) + strlen(id) + 3;
			rtr = malloc(len);
			strcat(rtr, "R ");
			strcat(rtr, id);
			strcat(rtr, " ");
			strcat(rtr, buf);
			return rtr;
		}
	}
	len = strlen(id) + 24;
	rtr = malloc(len);
	strcat(rtr, "I ");
	strcat(rtr, id);
	strcat(rtr, " ");
	strcat(rtr, "error: division par 0");
	return rtr;

}
