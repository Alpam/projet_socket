/*
 * =====================================================================================
 *
 *       Filename:  master.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  27/04/2018 15:54:56
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Paul Robin (), paul.robin@etu.unistra.fr
 *
 * =====================================================================================
 */
#ifndef __MASTER_H
#define __MASTER_H

#define RDDC_MAX 4


/*
 * structure représentant les esclaves
 */
typedef struct{
	char                *ip;
	char                *port;
	char                *request;
	char                *smbl;
	time_t               alive;
	int                  id;
	int                  t_ip;
	struct sockaddr_in6  saddr;
	struct sockaddr_in   saddr_4;
}server;

/*
 * liste chainée regroupant les esclaves par symbole
 * la redondance maximale est définie par RDDC_MAX
 */
typedef struct fct_family{
	struct fct_family *next;
	char              *symbole;
	server             slaves[RDDC_MAX];
}fct_family;

/*
 * structure transmise aux threads gérant une socket d'écoute
 * elle porte les informations essentielles pour la transmission
 * de messages.
 * elle est également commune à tous les threads leurs permettant
 * de communiquer entre eux.
 */
typedef struct{
	char                *ip;
	char                *ip_4;
	int                  port;
	int                  port_4;
	int                  socket_listener;
	int                  socket_list_4;
	int                  last_id;
	fct_family          *fct_f;
	pthread_mutex_t      mutex;
}struct_for_listener;

/*
 * structure transmise au thread janitor, elle porte les
 * informations qui lui sont nécessaires.
 */
typedef struct{
	pthread_mutex_t     mutex;
	struct_for_listener *cl;
}struct_for_janitor;

/*
 * fonctions pour les threads gérant une socket d'écoute
 * ipv4 ou ipv6
 */
void *listener(void *arg);
void *listener_4(void *arg);

/*
 * fonction pour le thread gérant le janitor
 */
void *janitor(void *arg);

/*
 * fonction regroupant les différentes actions à mener endif
 * fonction des messages entrant
 */
void action(const char *buf,struct_for_listener *cl);

/*
 * fonction vérifiant si un symbole est connu.
 * si oui renvoit ça position dans la liste chainée (1 à n)
 * si non retourne 0.
 */
int smbl_in_fml(char *smbl, fct_family *ff);

/*
 * ajoute une fonction à la liste des fonctions connues
 */
void add_fct(char *smbl, struct_for_listener *cl);

/*
 * ajoute un esclave associer à une fonction.
 * si c'est possible renvoit 1
 * si il n'y a plus de place renvoit 0
 */
int add_slave(server s,fct_family *ff,int *l_id);

/*
 * affiche l'état du master
 * les fonctions et les esclaves associés et l'état de ces derniers
 */
void show_state(fct_family *ff);

/*
 * vérifie qu'une requête est valide
 * contient des entiers ou flottants positifs
 */
char *validation_transaction(char* str);

/*
 * trouve et renvoie un esclave libre dans un famille donnée
 * renvoie NULL sinon
 */
server *free_slave_in_smbl(fct_family *ff,int t);

/*
 * trouve et renvoie un eslave définit par son id
 * renvoie NULL sinon
 */
server *find_serv_by_id(fct_family *ff, int id);

/*
 * fonction pour discuter avec le master via l'entrée standard
 */
void chat_loop(struct_for_listener *cl);

/*
 * clean la struct_for_listener et envoit le message S aux
 * esclaves.
 */
void clean(struct_for_listener *cl);

/*
 * vérifie qu'un eslave donne toujours signe de vie
 */
int still_alive(time_t last_call);
#endif

