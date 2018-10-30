/*
 * =====================================================================================
 *
 *       Filename:  master.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  27/04/2018 15:44:33
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
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>

#include "common.h"
#include "master.h"

int main(int argc, char **argv){
	//creation des sockets
	int socket_list_6, socket_list_4;
	socket_list_6 = socket(AF_INET6,
						   SOCK_DGRAM,
						   IPPROTO_UDP);
	socket_list_4 = socket(AF_INET,
						   SOCK_DGRAM,
						   IPPROTO_UDP);

	//preparation de la structure principale
	struct_for_listener cl;
	cl.ip = malloc(3);
	strncpy(cl.ip,"::1",3);

	cl.ip_4 = malloc(9);
	strncpy(cl.ip_4,"127.0.0.1",9);

	cl.port            = 25000;
	cl.port_4          = 25001;

	cl.socket_listener = socket_list_6;
	cl.socket_list_4   = socket_list_4;
	cl.last_id         = 0;
	cl.fct_f           = NULL;

	pthread_mutex_init(&cl.mutex,NULL);

	//creation et demarrage des threads
	pthread_t thread_listener;
	pthread_t thread_listener_4;
	pthread_create(&thread_listener,
				   NULL,
				   listener,
				   &cl);
	pthread_create(&thread_listener_4,
				   NULL,
				   listener_4,
				   &cl);

	struct_for_janitor cj;
	cj.cl = &cl;
	pthread_mutex_init(&cj.mutex,NULL);

	pthread_t thread_janitor;
	pthread_create(&thread_janitor,
								 NULL,
								 janitor,
								 &cj);

	printf("En attente de vos commandes\n");
	chat_loop(&cl);

	//procédure d'arrêt
	shutdown(socket_list_6,SHUT_RDWR);
	shutdown(socket_list_4,SHUT_RDWR);
	pthread_join(thread_listener,NULL);
	pthread_mutex_lock(&cj.mutex);
	pthread_cancel(thread_janitor);
	pthread_join(thread_janitor,NULL);
	pthread_mutex_destroy(&cj.mutex);
	clean(&cl);
	return 0;
}

void chat_loop(struct_for_listener *cl){
	char buf[100];
	while(1){
		memset(&buf,'\0',100);
		fgets(buf,100,stdin);
		if(!(strncmp(buf,"quit",4))){
			break;
		}
		else if(!(strncmp(buf,"etat",4))){
			show_state(cl->fct_f);
			continue;
		}
		else if(!(strncmp(buf,"help",4))){
			printf("help :\n\"etat\" : affiche les fonctions connues et leurs esclaves associés.\n\"send smbl(arg1,...,argn)\" : envoyer une requête\n\"quit\" : pour quitter\n");
			continue;
		}
		else if(!(strncmp(buf,"send",4))){
			char *tmp,*mess;
			server *s;
			//validation de la transaction
			tmp = validation_transaction(&buf[5]);
			if(tmp == NULL){
				printf("Fail: arguments invalides\n");
				continue;
			}
			//recherche de la fonction
			int sbl_ok = smbl_in_fml(tmp,cl->fct_f);
			if(!sbl_ok){
				printf("Fail : Fonction inconnue\n");
				free(tmp);
				continue;
			}
			sbl_ok--;
			//récupération d'un esclave libre
			s = free_slave_in_smbl(cl->fct_f,sbl_ok);
			if(s==NULL){
				printf("Fail : Aucun serveur disponible\n");
				free(tmp);
				continue;
			}
			//envoie de la requête
			int tp = strlen(tmp);
			tmp[tp]=' ';
			mess = malloc(strlen(tmp)+2);
			strcat(mess,"Q ");
			strcat(mess,&tmp[tp+1]);
			s->request = mess;
			int sock;
			if(s->t_ip == 6){
				sock = socket(AF_INET6,
											SOCK_DGRAM,
											IPPROTO_UDP);
				throwto(sock,s->saddr,mess);
			}
			else{
				sock = socket(AF_INET,
											SOCK_DGRAM,
											IPPROTO_UDP);
				throwto_4(sock,s->saddr_4,mess);
			}
			close(sock);
		}
		else{
			printf("commande non reconnue.\n");
		}
	}
}

server *free_slave_in_smbl(fct_family *ff, int t){
	for(int i=0;i<t;i++){ff = ff->next;}
	for(int i=0;i<RDDC_MAX;i++){
		if(ff->slaves[i].id != -1 &&\
			 ff->slaves[i].request == NULL  &&\
			 still_alive(ff->slaves[i].alive)){
			return &ff->slaves[i];
		}
	}
	return NULL;
}

char *validation_transaction(char *str){
	int i,j,vld=2;
	char *rtr;
	for(i=0;str[i]!='(';i++){}
	i++;
	while(1){
		//si on trouve dans les arguments autre chose qu'un digit
		//ou plus d'un point on considère l'argument comme faux.
		if(str[i]=='.'){vld--;}
		else if(str[i]==')'){break;}
		else if(str[i]==','){vld=2;i++;break;}
		else if(str[i]<48 && str[i]>57){
			return NULL;
		}
		if(!vld){return NULL;}
		i++;
	}
	rtr = malloc(i+2);
	strcat(rtr,str);
	
	for(j=0;j<i+2;j++){
		if(rtr[j]=='('){rtr[j]='\0';}
	}
	return rtr;
}

void *listener(void *arg){
	//creation socket d'écoute
	struct_for_listener *cl = (struct_for_listener *) arg;

	socklen_t addrlen;
	struct sockaddr_in6 my_addr;
	//structure qui receptionnera l'adresse du client
	
	my_addr.sin6_family = AF_INET6;
	my_addr.sin6_port   = htons(cl->port);
	my_addr.sin6_addr   = in6addr_any;
	addrlen             = sizeof(struct sockaddr_in6);

	//la socket est bind avant de recevoir
	bind(cl->socket_listener,
							 (struct sockaddr *) &my_addr,
							 addrlen);

	char buf[1024];
	memset(buf,'\0',1024);

	//boucle sur la reception, cette derniere s'arrete
	//lorsque le thread principal shutdown la socket
	
		while(recv(cl->socket_listener,
								 buf,1024,0) != 0){
			//creation de la structure pour la fonction action
			action(buf,cl);
			memset(buf,'\0',1024);
		}
}

void *listener_4(void *arg){
	//creation socket d'écoute
	struct_for_listener *cl = (struct_for_listener *) arg;

	socklen_t addrlen;
	struct sockaddr_in my_addr;
	//structure qui receptionnera l'adresse du client
	
	my_addr.sin_family      = AF_INET;
	my_addr.sin_port        = htons(cl->port_4);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addrlen                 = sizeof(struct sockaddr_in);

	//la socket est bind avant de recevoir
	bind(cl->socket_list_4,
			 (struct sockaddr *) &my_addr,
			 addrlen);

	char buf[1024];
	memset(buf,'\0',1024);

	//boucle sur la reception, cette derniere s'arrete
	//lorsque le thread principal shutdown la socket
	
		while(recv(cl->socket_list_4,
								 buf,1024,0) != 0){
			//creation de la structure pour la fonction action
			action(buf,cl);
			memset(buf,'\0',1024);
		}
}

//si le symbole smbl est connue retourne la position dans
//la liste chainée (1 à n)
//sinon 0
int smbl_in_fml(char *smbl, fct_family *ff){
	int i = 0;
	int j = 0;
	while(ff != NULL){
		i++;
		if(strncmp(smbl,ff->symbole,strlen(smbl)) == 0){
			j = 1;
			break;
		}
		ff = ff->next;
	}
	return j?i:0;
}

//ajoute une fonction à la collection
void add_fct(char *smbl, struct_for_listener *cl){
	if(cl->fct_f == NULL){
		cl->fct_f = malloc(sizeof(fct_family));
		cl->fct_f->next = NULL;
		cl->fct_f->symbole = smbl;
		for(int i=0;i<RDDC_MAX;i++){
			cl->fct_f->slaves[i].id = -1;
		}
	}
	else{
		fct_family *tmp;
		tmp = cl->fct_f;
		while(tmp->next != NULL){
			tmp = tmp->next;
		}
		tmp->next = malloc(sizeof(fct_family));
		tmp->next->next = NULL;
		tmp->next->symbole = smbl;
		for(int i=0;i<RDDC_MAX;i++){
			tmp->next->slaves[i].id = -1;
		}
	}
	return;
}

//associe un nouveau serveur si possible renvoie 1
//sinon 0
int add_slave(server s,fct_family *ff,int *l_id){
	for(int i=0;i<RDDC_MAX;i++){
		if(ff->slaves[i].id == -1){
			ff->slaves[i].id      = s.id;
			ff->slaves[i].ip      = s.ip;
			ff->slaves[i].port    = s.port;
			ff->slaves[i].request = NULL;
			ff->slaves[i].smbl    = ff->symbole;
			ff->slaves[i].alive   = time(NULL);
			ff->slaves[i].t_ip    = s.t_ip;
			if(s.t_ip == 6){
				ff->slaves[i].saddr = s.saddr;
			}
			else{
				ff->slaves[i].saddr_4 = s.saddr_4;
			}
			return 1;
		}
	}
	return 0;
}

//affiche l'état du maitre et esclave
void show_state(fct_family *ff){
	while(ff != NULL){
		printf("%s :\n",ff->symbole);
		for(int i=0;i<RDDC_MAX;i++){
			if(ff->slaves[i].id != -1 && ff->slaves[i].alive){
				printf("%s:%s",ff->slaves[i].ip,ff->slaves[i].port);
				if(ff->slaves[i].request != NULL){
					printf(" - %s",ff->slaves[i].request);
				}
				printf("\n");
			}
		}
		ff = ff->next;
	}
	return;
}

//agit en fonction du message arrivant
void action(const char *buf,struct_for_listener *cl){
	int i = 0;
	while(buf[i]!='\0'){
		i++;
	}
	char *tmp = malloc(i);
	char *sav = tmp;
	strncpy(tmp,buf,i);

	pthread_mutex_lock(&cl->mutex);
	if(tmp[0]=='C'){
		//on analyse le message entrant pour récupérer l'ip et le port et le symbole
		//ces derniers seront ajoutés à un server temporaire
		server s;
		s.t_ip = 6;
		tmp = &tmp[2];
		for(i=0;tmp[i]!=' ';i++){}
		char *smbl = malloc(i);
		strncpy(smbl,tmp,i);
		tmp = &tmp[++i];
		//on détermine le protocole
		for(i=0;tmp[i]!=' ';i++){
			if(tmp[i]=='.'){s.t_ip = 4;}
		}
		char *ip = malloc(i);
		strncpy(ip,tmp,i);
		tmp = &tmp[++i];
		for(i=0;i<strlen(tmp);i++){}
		char *port = malloc(i);
		strncpy(port,tmp,i);
		s.ip   = ip;
		s.port = port;
		s.id   = cl->last_id;
		cl->last_id++;
		//on crée l'adresse en fonction du type de protocole
		if(s.t_ip == 6){
			inet_pton(AF_INET6, s.ip, &s.saddr.sin6_addr);
			s.saddr.sin6_family = AF_INET6;
			s.saddr.sin6_port   = htons(atoi(s.port));
		}
		else{
			inet_pton(AF_INET, s.ip, &s.saddr_4.sin_addr);
			s.saddr_4.sin_family = AF_INET;
			s.saddr_4.sin_port   = htons(atoi(s.port));
		}
		int j;
		//on regarde si le symbole est connue
		//sinon on ajoute cette famille à celles connues
		//puis l'esclave à cette dernière
		//si on ne peut pas ajouter l'esclave un message E est renvoyé
		if(!(j=smbl_in_fml(smbl,cl->fct_f))){
			add_fct(smbl,cl);
			j=smbl_in_fml(smbl,cl->fct_f);
		}
		fct_family *ff_tmp = cl->fct_f;
		for(i=1;i<j;i++){
			ff_tmp = ff_tmp->next;
		}
		char mess[12];
		if(add_slave(s, ff_tmp, &cl->last_id)){
			sprintf(mess,"A %d",s.id);
			printf("Connection de %s (%s:%s)\n",smbl,s.ip,s.port);
		}
		else{
			printf("Liste pleine, refus de %s (%s:%s)\n",smbl,s.ip,s.port);
			memset(mess,'\0',12);
			mess[0]='E';
		}
		int tmp_sock;
		if(s.t_ip == 6){
			tmp_sock = socket(AF_INET6,SOCK_DGRAM,IPPROTO_UDP);
			throwto(tmp_sock,s.saddr,mess);
		}
		else{
			tmp_sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
			throwto_4(tmp_sock,s.saddr_4,mess);
		}
		free(sav);
		close(tmp_sock);

	}
	else if(buf[0]=='D'){
		int id = atoi(&buf[2]);
		server *s = find_serv_by_id(cl->fct_f,id);
		fprintf(stdout,"Deconnection de ",id);
		if(s!=NULL){
			fprintf(stdout,"(%s:%s)\n",s->ip,s->port);
			free(s->ip);
			free(s->port);
			s->id = -1;
			s->t_ip = 0;
			s->alive = 0;
		}
	}

	else if(buf[0]=='R' || buf[0]=='I'){
		//on récupère la réponse d'un esclave et on affiche le résultat
		int i;
		for(i=2;buf[i]!=' ';i++){}
		char *tmp = malloc(i-2);
		for(int j=0;j< i-2;j++){
			tmp[j]=buf[2+j];
		}
		int id = atoi(tmp);
		server *s = find_serv_by_id(cl->fct_f,id);
		if(buf[0]=='R'){
			printf("reponse pour : %s",s->smbl);
			printf("(%s",&s->request[2]);
			printf("%s\n",&buf[i+1]);
		}
		else{
			printf("reponse pour : %s",s->smbl);
			printf("(%s",&s->request[2]);
			printf("%s\n",&buf[i+1]);
		}
		free(s->request);
		s->request = NULL;
	}

	else if(buf[0]=='K'){
		//si l'esclave est connu on met à jour son champ alive
		int id = atoi(&buf[2]);
		server *s = find_serv_by_id(cl->fct_f,id);
		if(s!=NULL){
			s->alive = time(NULL);
		}
	}
	pthread_mutex_unlock(&cl->mutex);
	return;
}

server *find_serv_by_id(fct_family *ff, int id){
	while(ff!=NULL){
		for(int i=0;i<RDDC_MAX;i++){
			if(ff->slaves[i].id == id){
				return &ff->slaves[i];
			}
		}
		ff=ff->next;
	}
	return NULL;
}

void *janitor(void *arg){
	struct_for_janitor *cj = (struct_for_janitor *)arg;
	struct_for_listener *cl = cj->cl;
	fct_family *ff;
	while(sleep(15)==0){
		//vérification de l'état du master (terminaison ou non)
		pthread_mutex_lock(&cj->mutex);
		pthread_mutex_lock(&cl->mutex);
		ff = cl->fct_f;
		while(ff != NULL){
			for(int i=0;i<RDDC_MAX;i++){
				//si l'esclave est mort
				if(ff->slaves[i].id != -1 &&\
					 !still_alive(ff->slaves[i].alive)){
					//on vérifie si il avait une requête en cours
					if(ff->slaves[i].request != NULL){
						//si oui on essaye de la transmettre à un autre esclaves
						int sbl = smbl_in_fml(ff->slaves[i].smbl,cl->fct_f);
						sbl--;
						server *s = free_slave_in_smbl(cl->fct_f,sbl);
						if(s==NULL){
						//si impossible elle est abandonnée
							printf("Abort : %s",ff->slaves[i].smbl);
							printf("(");
							printf("%s\n",&ff->slaves[i].request[2]);
							free(ff->slaves[i].request);
							free(ff->slaves[i].ip);
							free(ff->slaves[i].port);
							ff->slaves[i].id = -1;
							ff->slaves[i].t_ip = 0;
							continue;
						}
						//transmission de la requête inachevée
						int sock;
						s->request = ff->slaves[i].request;
						ff->slaves[i].request = NULL;
						if(s->t_ip == 6){
							sock = socket(AF_INET6,
														SOCK_DGRAM,
														IPPROTO_UDP);
							throwto(sock,s->saddr,s->request);
						}
						else{
							sock = socket(AF_INET,
														SOCK_DGRAM,
														IPPROTO_UDP);
							throwto_4(sock,s->saddr_4,s->request);
						}
						close(sock);
					}
					free(ff->slaves[i].ip);
					free(ff->slaves[i].port);
					ff->slaves[i].id = -1;
					ff->slaves[i].t_ip = 0;
				}
			}
			ff = ff->next;
		}
		pthread_mutex_unlock(&cl->mutex);
		pthread_mutex_unlock(&cj->mutex);
	}
}

void clean(struct_for_listener *cl){
	//free de la mémoire allouée
	free(cl->ip);
	free(cl->ip_4);
	fct_family *tmp;
	pthread_mutex_destroy(&cl->mutex);
	int i;
	while(cl->fct_f != NULL){
		for(i=0;i<RDDC_MAX;i++){
			//prévient les esclaves de l'arrêt du master
			if(cl->fct_f->slaves[i].id != -1){
				int tmp_sock;
				if(cl->fct_f->slaves[i].t_ip == 6){
					tmp_sock = socket(AF_INET6,
														 SOCK_DGRAM,
														 IPPROTO_UDP);
					throwto(tmp_sock,cl->fct_f->slaves[i].saddr,"S");
				}
				else{
					tmp_sock = socket(AF_INET,
													 SOCK_DGRAM,
													 IPPROTO_UDP);
					throwto_4(tmp_sock,cl->fct_f->slaves[i].saddr_4,"S");
				}
				close(tmp_sock);
			}
			free(cl->fct_f->slaves[i].ip);
			free(cl->fct_f->slaves[i].port);
			if(cl->fct_f->slaves[i].request!=NULL){
				free(cl->fct_f->slaves[i].request);
			}
		}
		free(cl->fct_f->symbole);
		tmp = cl->fct_f;
		cl->fct_f = cl->fct_f->next;
		free(tmp);
	}
}

int still_alive(time_t last_call){
	return (time(NULL)-last_call)>15?0:1;
}
