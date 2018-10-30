/*
 * =====================================================================================
 *
 *       Filename:  common.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  29/04/2018 09:13:52
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Paul Robin (), paul.robin@etu.unistra.fr
 *
 * =====================================================================================
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

#include "common.h"

void core_slave(const char* symbole, const char* ip, const char *port,char *(*mission)(char *,char *)){
	//détermine le protocole utilisé
	//si l'adresse localhost est fournie on lance l'esclave en ipv4
	int t_ip = 0;
	char *tmp_ip = malloc(strlen(ip));
	for(int i=0;i<strlen(ip);i++){
		if(ip[i]=='.'){
			t_ip = 4;
			strcpy(tmp_ip,ip);
			break;
		}
		else if(ip[i]==':'){
			t_ip = 6;
			strcpy(tmp_ip,ip);
			break;
		}
	}
	if(t_ip == 0){
		if(strncmp(ip,"localhost",9)==0){
			t_ip = 4;
			strcpy(tmp_ip,"127.0.0.1");
		}
	}
	int socket_listener;
	if(t_ip == 6){
		socket_listener = socket(AF_INET6,
								 SOCK_DGRAM,
								 IPPROTO_UDP);
	}
	else{
		socket_listener = socket(AF_INET,
								 SOCK_DGRAM,
								 IPPROTO_UDP);
	}

	pthread_t thread_quit_loop;
	pthread_create(&thread_quit_loop,
								 NULL,
								 quit_loop,
								 NULL);

	//creation de la structure principale de l'esclave
	struct_for_listener_c cl;
	cl.ip               = tmp_ip;
	cl.id               = NULL;
	cl.fct              = mission;
	cl.port             = atoi(port);
	cl.smbl             = symbole;
	cl.t_ip             = t_ip;
	cl.flag             = 1;
	cl.socket_listener  = socket_listener;
	cl.thread_quit_loop = thread_quit_loop;
	//lancement de la socket d'écoute
	pthread_t thread_listener;
	pthread_create(&thread_listener,
				   NULL,
				   listener_c,
				   &cl);

	//concaténation de l'ip et port pour les transmettre au maitre
	int len = strlen(tmp_ip)+strlen(port)+strlen(symbole)+4;
	char *my_addr = malloc(len);
	strcat(my_addr,"C ");
	strcat(my_addr,symbole);
	strcat(my_addr," ");
	strcat(my_addr,tmp_ip);
	strcat(my_addr," ");
	strcat(my_addr,port);

	//creation de la socket vers le maitre
	//envoie du message de connection au maitre
	int sock_master;
	struct sockaddr_in6 master;
	struct sockaddr_in master_4;
	if(t_ip == 6){
		sock_master = socket(AF_INET6,
												 SOCK_DGRAM,
												 IPPROTO_UDP);
		inet_pton(AF_INET6, "::1", &master.sin6_addr);
		master.sin6_family = AF_INET6;
		master.sin6_port   = htons(25000);
		cl.master = master;
		throwto(sock_master, master ,my_addr);
	}
	else{
		sock_master = socket(AF_INET,
												 SOCK_DGRAM,
												 IPPROTO_UDP);
		inet_pton(AF_INET, "127.0.0.1", &master_4.sin_addr);
		master_4.sin_family = AF_INET;
		master_4.sin_port   = htons(25001);
		cl.master_4 = master_4;
		throwto_4(sock_master, master_4 ,my_addr);
	}
	free(my_addr);
	
	pthread_join(thread_quit_loop,NULL);
	if(cl.flag){
		shutdown(socket_listener,SHUT_RDWR);
	}
	pthread_join(thread_listener,NULL);
	//message de deconnection pour le maitre
	if(cl.id != NULL){
		char *tmp = malloc(strlen(cl.id)+2);
		strcat(tmp,"D ");
		strcat(tmp,cl.id);
		if(t_ip == 6){
			throwto(sock_master,master,tmp);
		}
		else{
			throwto_4(sock_master,master_4,tmp);
		}
		free(tmp);
		free(cl.id);
	}
	free(tmp_ip);
	close(sock_master);
	return;
}

void *listener_c(void *arg){
	//creation socket d'écoute
	struct_for_listener_c *cl = (struct_for_listener_c *) arg;

	socklen_t addrlen;
	struct sockaddr_in6 my_addr;
	struct sockaddr_in my_addr_4;
	//structure qui receptionnera l'adresse du client (en ipv4 ou ipv6)
	if(cl->t_ip == 6){
		my_addr.sin6_family = AF_INET6;
		my_addr.sin6_port   = htons(cl->port);
		addrlen             = sizeof(struct sockaddr_in6);
		inet_pton(AF_INET6, cl->ip, &my_addr.sin6_addr);
		bind(cl->socket_listener,
				 (struct sockaddr *) &my_addr,
				 addrlen);
	}
	else{
		my_addr_4.sin_family = AF_INET;
		my_addr_4.sin_port   = htons(cl->port);
		addrlen              = sizeof(struct sockaddr_in);
		inet_pton(AF_INET, cl->ip, &my_addr_4.sin_addr);
		bind(cl->socket_listener,
				 (struct sockaddr *) &my_addr_4,
				 addrlen);
	}
	//la socket est bind avant de recevoir
	char buf[1024];
	memset(buf,'\0',1024);

	//mise en place du mutex pour la terminaison du keep alive
	pthread_mutex_t mutex_ka;
	pthread_mutex_init(&mutex_ka,NULL);
	pthread_mutex_lock(&mutex_ka);

	pthread_t thread_ka;

	//mise en place du thread de timeout
	pthread_mutex_t mutex_to;
	pthread_mutex_init(&mutex_to,NULL);

	struct_for_to to;
	to.socket = cl->socket_listener;
	to.mutex  = &mutex_to;

	pthread_t thread_to;
	pthread_create(&thread_to,
				   NULL,
				   timeout,
				   &to);

	//boucle sur la reception, cette derniere s'arrete
	//lorsque le thread principal ou le thread time out 
	//shutdown la socket_listener
	while(recv(cl->socket_listener, buf,1024,0) != 0){
			//creation de la structure pour la fonction action
			printf("in come:%s\n",buf);
			if(buf[0]=='E'){
				int r_mu = pthread_mutex_trylock(&mutex_to);
				if (r_mu != 0){
					break;
				}
				printf("master complet pour %s\n",cl->smbl);
				break;
			}
			else if(buf[0]=='S'){
				printf("Master Stop\nUtiliser \"quit\" pour terminer.\n");
				break;
			}
			else if(buf[0]=='Q'){
				//réception d'une requête
				char *rep;
				rep = cl->fct(&buf[2],cl->id);
				int sock_master;
				if(cl->t_ip == 6){
					sock_master = socket(AF_INET6,
										 SOCK_DGRAM,
										 IPPROTO_UDP);
					throwto(sock_master,cl->master,rep);
				}
				else{
					sock_master = socket(AF_INET,
										 SOCK_DGRAM,
										 IPPROTO_UDP);
					throwto_4(sock_master,cl->master_4,rep);
				}
				free(rep);
			}
			else if(buf[0]=='A'){
				//acceptation par le maitre
				//récupération de l'id
				int r_mu = pthread_mutex_trylock(&mutex_to);
				if (r_mu != 0){
					break;
				}
				int len = strlen(buf)-2;
				char *id = malloc(len);
				for(int i=0;i<len;i++){
					id[i]=buf[i+2];
				}
				cl->id = id;
				//démarrage du keep alive
				struct_for_ka ka;
				ka.id = id;
				ka.t_ip = cl->t_ip;
				ka.mutex = &mutex_ka;
				pthread_create(&thread_ka,
								 NULL,
								 keep_alive,
								 &ka);
				pthread_detach(thread_ka);
			}
			memset(buf,'\0',1024);
	}
	cl->flag = 0;
	pthread_cancel(cl->thread_quit_loop);
	close(cl->socket_listener);
	pthread_mutex_unlock(&mutex_ka);
	pthread_join(thread_to,NULL);
}

void *quit_loop(void *arg){
//petite boucle permettant de quitter proprement
	char buf[100];
	printf("en attente :(\"quit\" pour quitter)\n");
	memset(&buf,'\0',100);
	while(fgets(buf,100,stdin)!=NULL){
		if (!(strncmp(buf,"quit",4))){
			break;
		}
		printf("en attente :(\"quit\" pour quitter)\n");
		memset(&buf,'\0',100);
	}
}

void *timeout(void *arg){
	sleep(5);
	struct_for_to *to = (struct_for_to *)arg;
	int r_mu = pthread_mutex_trylock(to->mutex);
	if(r_mu == 0){
		printf("Pas de reponse, ferme par timeout\n");
		shutdown(to->socket,2);
	}
}

void *keep_alive(void *arg){
	struct_for_ka *ka = (struct_for_ka *)arg;
	char *m = malloc(strlen(ka->id)+2);
	memset(m,'\0',strlen(ka->id)+2);
	strcat(m,"K ");
	strcat(m,ka->id);
	int sock_master;
	struct sockaddr_in6 master;
	struct sockaddr_in master_4;
	//crée une socket vers le maitre
	if(ka->t_ip == 6){
		sock_master = socket(AF_INET6,
											 SOCK_DGRAM,
											 IPPROTO_UDP);
		inet_pton(AF_INET6, "::1", &master.sin6_addr);
		master.sin6_family = AF_INET6;
		master.sin6_port   = htons(25000);
	}
	else{
		sock_master = socket(AF_INET,
												 SOCK_DGRAM,
												 IPPROTO_UDP);
		inet_pton(AF_INET, "127.0.0.1", &master_4.sin_addr);
		master_4.sin_family = AF_INET;
		master_4.sin_port   = htons(25001);
	}
	//boucle infinit permettant d'envoyer périodiquement 
	//un message K
	while(sleep(10)==0){
		//vérification de l'état de l'esclave
		int r_mu = pthread_mutex_trylock(ka->mutex);
		if(r_mu==0){
			break;
		}
		if(ka->t_ip == 6){
			throwto(sock_master,master,m);
		}
		else{
			throwto_4(sock_master,master_4,m);
		}
	}
	free(m);
	pthread_mutex_destroy(ka->mutex);
}

void throwto(int socket, struct sockaddr_in6 target, const char *message){
	socklen_t addrlen = sizeof(struct sockaddr_in6);
	if(sendto(socket,
						message,
						strlen(message),0,
						(struct sockaddr *)&target,
						addrlen) == -1){printf("err sendto\n");}
	return;
}

void throwto_4(int socket, struct sockaddr_in target, const char *message){
	socklen_t addrlen = sizeof(struct sockaddr_in);
	if(sendto(socket,
						message,
						strlen(message),0,
						(struct sockaddr *)&target,
						addrlen) == -1){printf("err sendto\n");}
	return;
}
