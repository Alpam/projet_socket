# projet_socket 

# Auteurs : 
Paul Robin
Geoffrey Bisch

#Instructions :

lancer le master avec ./master
lancer les esclaves avec ./slave_x ip port

exemple : ./slave_mod localhost 1234

adresse supporter : ipv4, ipv6 (localhost donne l'adresse 127.0.0.1)

#Correctif Apporté :

Déconnection mieux gérer pour le maitre et les esclaves.
Une fois la procédure d'arrêt lancée les esclaves s'arrêtent sans attendre la fin
d'une boucle de leur Keep a Live, ce dernier est simplement shut down.
De même pour le master avec le Janitor.
Correctif du rapport : il y noté que les chiffres négatifs ne sont pas gérés alors 
que si ...