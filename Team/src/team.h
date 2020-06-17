/*
 ============================================================================
 Name        : Team
 Author      : Fran and Co
 Description : Proceso Team Header
 ============================================================================
*/

#ifndef TEAM_H_
#define TEAM_H_

#include "funcionesUtilesTeam.h"

// IP y PUERTO de team para iniciar servidor

#define TEAM_LOG "team.log"
#define TEAM_NAME "team"
#define TEAM_CONFIG "team.config"

const static struct {
	op_code codigoOperacion;
	const char* str;
} conversionCodigoOp[] = {

		{CATCH_POKEMON, "CATCH_POKEMON"},
		{GET_POKEMON, "GET_POKEMON"},

};

// Variables globales
char* IP_TEAM;
char* PUERTO_TEAM;
char* ipBroker;
char* puertoBroker;
int ID_TEAM;
int TIEMPO_RECONEXION;
pthread_t thread;

// Listas
t_list* atrapados;
t_list* pendientes;
t_list* id_mensajeGet;
t_list* id_mensajeCatch;

// Mutexs
extern pthread_mutex_t mutex_send;

void quedarseALaEscucha(int*);

t_log* iniciar_logger(void);

t_config* leer_config(void);

void inicializarConfig(t_config*);

void inicializarSemaforosYMutex();

void suscribirseAppeared();

void suscribirseCaught();

void suscribirseLocalized();

void suscribirseAColas();

void suscribirseA(op_code);

void serve_client(int*);

void process_request(int, uint32_t, void*, int);

op_code stringACodigoOperacion(const char*);

void enviarMensajeGetABroker();

t_list* eliminarRepetidos();

void enviarMensajeGet(t_nombrePokemon*);

void enviarMensajeCatch(t_newPokemon*);

void inicializarListas();

void esperarIdGet(int);

void esperarIdCatch(int);

void requiere(t_nombrePokemon*, t_coordenadas*);

void diferencia();

//void terminar_programa(int socket, t_log* logger, t_config* config);

#endif /* TEAM_H_ */
