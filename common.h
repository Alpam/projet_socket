/*
 * =====================================================================================
 *
 *       Filename:  common.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  29/04/2018 09:15:32
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Paul Robin (), paul.robin@etu.unistra.fr
 *				   Geoffrey Bisch (), geoffrey.bisch@etu.unistra.fr *   Organization:  
 *
 * =====================================================================================
 */
#ifndef __COMMON_H
#define __COMMON_H

/*
 * structure portant toute les informations nécessaires au 
 * thread gérant la socket d'écoute.
 */
typedef struct{
	char               *(*fct)(char *,char *);
	const char         *ip;
	char               *id;
	const char         *smbl;
	int                 t_ip;
	int                 port;
	int                 socket_listener;
	struct sockaddr_in6 master;
	struct sockaddr_in  master_4;
}struct_for_listener_c;

/*
 * structure portant toutes les informations pour le thread
 * gérant le keep alive
 */
typedef struct{
	char *id;
	int   t_ip;
	pthread_mutex_t *mutex;
}struct_for_ka;

/*
 * fonction principale d'un esclave, lance les threads keep alive
 * et de la socket d'écoute.
 * détermine également le type de protocole utilisé
 */
void core_slave(const char* symbole, const char* ip, const char *port,char *(*mission)(char *,char *));

/*
 * fonction pour le thread gérant la socket d'écoute
 */
void *listener_c(void *arg);

/*
 * fonction pour le thread gérant le keep alive
 */
void *keep_alive(void *arg);

/*
 * fonction permettant d'envoyer un message vers une adresse
 * ipv6
 */
void throwto(int socket, struct sockaddr_in6 target, const char *message);

/*
 * fonction permettant d'envoyer un message vers une adresse
 * ipv4
 */
void throwto_4(int socket, struct sockaddr_in target, const char *message);

#endif
