/*
 * funcionesUtilesTeam.c
 *
 *  Created on: 4 may. 2020
 *      Author: utnso
 */

#include "funcionesUtilesTeam.h"

pthread_mutex_t mutex_id_entrenadores = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_entrenador = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_hay_pokemones = PTHREAD_MUTEX_INITIALIZER;

/*
  ============================================================================
 	 	 	 	 	 	 	 	 HITO 2
  ============================================================================
*/

void ponerEntrenadoresEnLista(t_config* config) {

	inicializarListasDeEstados();

	entrenadores = list_create(); //Creamos la lista de entrenadores

	t_coordenadas* coords = malloc(sizeof(t_coordenadas));
	char** coordenadasEntrenadores = config_get_array_value(config,
			"POSICIONES_ENTRENADORES");

	//t_nombrePokemon* pokemonesQuePosee = malloc(sizeof(t_nombrePokemon));
	char** pokemonesDeEntrenadores = config_get_array_value(config,
			"POKEMON_ENTRENADORES");

	//t_nombrePokemon* pokemonesQueQuiere = malloc(sizeof(t_nombrePokemon));
	char** pokemonesObjetivoDeEntrenadores = config_get_array_value(config,
			"OBJETIVOS_ENTRENADORES");

	int i = 0, j = 0;

	t_list* listaDePokemonesDeEntrenadores = organizarPokemones(pokemonesDeEntrenadores);
	t_list* listaDePokemonesObjetivoDeEntrenadores = organizarPokemones(pokemonesObjetivoDeEntrenadores);

	while (coordenadasEntrenadores[i] != NULL) {

		coords->posX = atoi(&coordenadasEntrenadores[i][0]);
		coords->posY = atoi(&coordenadasEntrenadores[i][2]);

		t_list* pokemonesQueTiene = (t_list*) list_get(listaDePokemonesDeEntrenadores, j);
		t_list* pokemonesQueDesea = (t_list*) list_get(listaDePokemonesObjetivoDeEntrenadores, j);

		uint32_t id_entrenador = generar_id();

		t_entrenador* entrenador = crear_entrenador(id_entrenador, coords, pokemonesQueTiene, pokemonesQueDesea, list_size(pokemonesQueTiene), NEW);

		list_add(entrenadores, entrenador);
		list_add(listaNuevos, entrenador);

		j++;
		i++;

	}

	hacerObjetivoTeam(listaDePokemonesDeEntrenadores, listaDePokemonesObjetivoDeEntrenadores);

	list_destroy(listaDePokemonesDeEntrenadores);
	list_destroy(listaDePokemonesObjetivoDeEntrenadores);
}

void crearHilosEntrenadores() {

	hilosEntrenadores = list_create();
	sem_entrenadores_ejecutar = list_create();

	int cantidadEntrenadores = list_size(entrenadores);

	pthread_t pthread_id[cantidadEntrenadores];

	for (int i = 0; i < cantidadEntrenadores; i++) {

		t_entrenador* entrenador = (t_entrenador*) list_get(entrenadores, i);

		sem_t semaforoDelEntrenador;

		sem_init(&semaforoDelEntrenador, 0, 0);

		list_add(sem_entrenadores_ejecutar, (void*) &semaforoDelEntrenador);

		pthread_create(&pthread_id[i], NULL, (void*) ejecutarEntrenador, entrenador);

		pthread_detach(pthread_id[i]);


		list_add(hilosEntrenadores, &pthread_id[i]);
	}

}

t_entrenador* crear_entrenador(uint32_t id_entrenador, t_coordenadas* coordenadas, t_list* pokemonesQuePosee, t_list* pokemonesQueQuiere, uint32_t cantidad_pokemons, status_code estado) {
	t_entrenador* entrenador = malloc(sizeof(t_entrenador));

	entrenador->id_entrenador = id_entrenador;
	entrenador->coordenadas = coordenadas;
	entrenador->pokemonesQuePosee = pokemonesQuePosee;
	entrenador->pokemonesQueQuiere = pokemonesQueQuiere;
	entrenador->cantidad_pokemons = cantidad_pokemons;
	entrenador->estado = estado;

	return entrenador;
}

t_list* organizarPokemones(char** listaPokemones) { //tanto para pokemonesObjetivoDeEntrenadores como para pokemonesDeEntrenadores

	int j = 0, w = 0;

	t_list* listaDePokemonesDeEntrenadores = list_create();

	while (listaPokemones[j] != NULL) { //recorro los pokemones de cada entrenador separado por coma
		char pipe = '|';
		char**pokemonesDeUnEntrenador = string_split(listaPokemones[j], &pipe); //separo cada pokemon de un mismo entrenador separado por |

		t_list* listaDePokemones = list_create();

		while (pokemonesDeUnEntrenador[w] != NULL) { //recorro todos y voy creando cada pokemon

			t_nombrePokemon* pokemon = crear_pokemon(pokemonesDeUnEntrenador[w]);

			list_add(listaDePokemones, pokemon);

			w++;
		}

		list_add(listaDePokemonesDeEntrenadores, listaDePokemones);

		j++;
		w=0;
	}

	return listaDePokemonesDeEntrenadores;

}

t_nombrePokemon* crear_pokemon(char* pokemon) {

	t_nombrePokemon* nuevoPokemon = malloc(sizeof(t_nombrePokemon));

	nuevoPokemon->nombre_lenght = strlen(pokemon) + 1;
	nuevoPokemon->nombre = pokemon;

	return nuevoPokemon;

}

uint32_t generar_id() {
	pthread_mutex_lock(&mutex_id_entrenadores);
	uint32_t id_generado = ID_ENTRENADORES++;
	pthread_mutex_unlock(&mutex_id_entrenadores);

	return id_generado;
}

void hacerObjetivoTeam(t_list* listaPokemonesTieneEntrenadores, t_list* listaPokemonesDeseaEntrenadores){ //Siempre Despues de Usar estas Listas

	 t_list* listaGrande = list_create();
	 t_list* listaMini = list_create();
	 objetivoTeam = list_create();

	 listaGrande = aplanarDobleLista(listaPokemonesDeseaEntrenadores);
	 listaMini = aplanarDobleLista(listaPokemonesTieneEntrenadores);

	 diferenciaYCargarLista(listaGrande, listaMini, objetivoTeam);

	 list_destroy(listaGrande);
	 list_destroy(listaMini);
}

t_list* aplanarDobleLista(t_list* lista){

	t_list* listaAplanada = list_create();

	int tamanioListaSuprema = list_size(lista);

		for(int b=0; b<tamanioListaSuprema ;b++){

			 int tamanioSubLista = list_size(list_get(lista, b));

			 for(int a=0; a<tamanioSubLista; a++){

				 list_add(listaAplanada, list_get(list_get(lista, b), a));
			 }
		}

	return listaAplanada;
}

void ejecutarEntrenador(t_entrenador* entrenador){

	sem_t* semaforoDelEntrenador = (sem_t*) list_get(sem_entrenadores_ejecutar, entrenador->id_entrenador);
	sem_wait(semaforoDelEntrenador);

	sleep(retardoCPU);

	moverAlEntrenadorHastaUnPokemon(entrenador->id_entrenador); //Lo mueve 1 posicion

	if(llegoAlObjetivo(entrenador)){
		evaluarEstadoPrevioAAtrapar(entrenador);
		//TODO terminar (catch)
	}
}

int llegoAlObjetivo(t_entrenador* entrenador){
	uint32_t posicionXEntrenador = entrenador->coordenadas->posX;
	uint32_t posicionYEntrenador = entrenador->coordenadas->posY;

	uint32_t posicionXPokemon = entrenador->pokemonInstantaneo->coordenadas->posX;
	uint32_t posicionYPokemon = entrenador->pokemonInstantaneo->coordenadas->posY;

	if(posicionXEntrenador == posicionXPokemon && posicionYEntrenador == posicionYPokemon){
		return true;
	} else{
		return false;
	}

}

void moverAlEntrenadorHastaUnPokemon(uint32_t idEntrenador){

	t_entrenador* entrenador = list_get(entrenadores, idEntrenador);

	uint32_t posicionXEntrenador = entrenador->coordenadas->posX;
	uint32_t posicionYEntrenador = entrenador->coordenadas->posY;

	uint32_t posicionXPokemon = entrenador->pokemonInstantaneo->coordenadas->posX;
	uint32_t posicionYPokemon = entrenador->pokemonInstantaneo->coordenadas->posY;

	uint32_t distanciaEnX = posicionXPokemon- posicionXEntrenador;
	uint32_t distanciaEnY = posicionYPokemon- posicionYEntrenador;

	if(posicionXEntrenador!= posicionXPokemon){

		if(distanciaEnX>0){
			entrenador->coordenadas->posX = posicionXEntrenador++;
		}else if(distanciaEnX<0){
			entrenador->coordenadas->posX = posicionXEntrenador--;
		}

	}else if(posicionYEntrenador!= posicionYPokemon){
		if(distanciaEnY>0){
			entrenador->coordenadas->posY = posicionYEntrenador++;
		}else if(distanciaEnX<0){
			entrenador->coordenadas->posY = posicionYEntrenador--;
		}
	}

}

void evaluarEstadoPrevioAAtrapar(t_entrenador* entrenador){
	//enviarMensajeCatch(entrenador->pokemonInstantaneo);

// 1. Tiene que pasar a blocked para que planifique a otros.
// 2. Si no se establece la conexion -> direc va a atraparlo.
// 3. Me dijo el chabon que la planificacion es por hilos.

//	if(entrenador->estado == BLOCKED){
//		list_add(listaBloqueadosEsperandoMensaje, entrenador);
//		//break; //Espera el mensaje caught correspondiente --> TODO semaforo esperando caught
//	} else{
//		atraparPokemon(entrenador);
//	}
}

t_entrenador* entrenadorMasCercano(t_newPokemon* pokemon){
	t_entrenador* entrenadorTemporal;
	t_entrenador* entrenadorMasCercanoBlocked;

	int distanciaTemporal;
	int menorDistanciaBlocked;

	t_list* entrenadores_bloqueados = listaBloqueadosEsperandoPokemones;

	if(!list_is_empty(entrenadores_bloqueados)){
		entrenadorMasCercanoBlocked = list_get(entrenadores_bloqueados, 0);
		menorDistanciaBlocked = distanciaA(entrenadorMasCercanoBlocked->coordenadas, pokemon->coordenadas);

		for(int i=0; i < entrenadores_bloqueados->elements_count; i++){

			if(menorDistanciaBlocked ==0){
				break;
			}

			entrenadorTemporal = list_get(entrenadores_bloqueados, i);
			distanciaTemporal = distanciaA(entrenadorTemporal->coordenadas, pokemon->coordenadas);

			if(distanciaTemporal < menorDistanciaBlocked){
				entrenadorMasCercanoBlocked = entrenadorTemporal;
				menorDistanciaBlocked = distanciaTemporal;
			}
		}
	}

	t_list* entrenadores_new = listaNuevos;
	t_entrenador* entrenadorMasCercanoNew;
	int menorDistanciaNew;


	if(!list_is_empty(entrenadores_new)){
		entrenadorMasCercanoNew = list_get(entrenadores_new, 0);
		menorDistanciaNew = distanciaA(entrenadorMasCercanoNew->coordenadas, pokemon->coordenadas);


		for(int i = 1; i < entrenadores_new->elements_count; i++){

			if(menorDistanciaNew == 0){
				break;
			}

			entrenadorTemporal = list_get(entrenadores_new, i);
			distanciaTemporal = distanciaA(entrenadorTemporal->coordenadas, pokemon->coordenadas);

			if(distanciaTemporal < menorDistanciaNew){
				entrenadorMasCercanoNew = entrenadorTemporal;
				menorDistanciaNew = distanciaTemporal;
			}

		}
	}

	if(menorDistanciaNew <= menorDistanciaBlocked ){
		entrenadorMasCercanoNew->estado = READY;
		list_add(listaReady, entrenadorMasCercanoNew);
		entrenadorMasCercanoNew->pokemonInstantaneo = pokemon;
		return entrenadorMasCercanoNew;

	} else{
		entrenadorMasCercanoNew->estado = READY;
		list_add(listaReady, entrenadorMasCercanoNew);
		entrenadorMasCercanoBlocked->pokemonInstantaneo = pokemon;
		return entrenadorMasCercanoBlocked;
	}

}

void buscarPokemon(t_newPokemon* pokemon){  //Busca al entrenador más cercano y pone a planificar (para que ejecute, es decir, para que busque al pokemon en cuestión)

	t_entrenador* entrenador = entrenadorMasCercano(pokemon);

	planificarSegun();

	//pthread_mutex_unlock(&mutex_entrenador); 		//a un entrenador no se le asignen más de un pokemon al haber un appeard
	//pthread_mutex_unlock(&mutex_entrenador_hilo); //signal al hilo del entrenador que va a ejecutar

}

void moverAlEntrenadorHastaOtroEntrenador(uint32_t idEntrenador1, uint32_t idEntrenador2){

	t_entrenador* entrenador1 = list_get(entrenadores, idEntrenador1);
	t_entrenador* entrenador2 = list_get(entrenadores, idEntrenador2);

	uint32_t posicionXEntrenador1 = entrenador1->coordenadas->posX;
	uint32_t posicionYEntrenador1 = entrenador1->coordenadas->posY;

	uint32_t posicionXEntrenador2 = entrenador2->coordenadas->posX;
	uint32_t posicionYEntrenador2 = entrenador2->coordenadas->posY;

	uint32_t distanciaEnX = posicionXEntrenador2 - posicionXEntrenador1;
	uint32_t distanciaEnY = posicionYEntrenador2 - posicionYEntrenador1;

	if(posicionXEntrenador1 !=  posicionXEntrenador2){

		if(distanciaEnX>0){
			entrenador1->coordenadas->posX = posicionXEntrenador1++;
		}else if(distanciaEnX<0){
			entrenador1->coordenadas->posX = posicionXEntrenador1--;
		}

	}else if(posicionYEntrenador1 != posicionYEntrenador2){
		if(distanciaEnY>0){
			entrenador1->coordenadas->posY = posicionYEntrenador1++;
		}else if(distanciaEnX<0){
			entrenador1->coordenadas->posY = posicionYEntrenador1--;
		}
	}

}

void intercambiarPokemones(uint32_t idEntrenador1, uint32_t idEntrenador2){
	t_entrenador* entrenador1 = list_get(entrenadores, idEntrenador1);
	t_entrenador* entrenador2 = list_get(entrenadores, idEntrenador2);

	dameTuPokemon(entrenador1,entrenador2);
	dameTuPokemon(entrenador2, entrenador1);

}

void dameTuPokemon(t_entrenador* entrenador1, t_entrenador* entrenador2){ //ya se que E2 tiene uno que quiere E1

	t_list* listaQuiere1 = list_create(); //TODO destruirlas bien
	t_list* listaPosee1 = list_create();
	t_list* listaQuiere2 = list_create();
	t_list* listaPosee2 = list_create();

	t_list* leFaltanParaObj1 = list_create();
	t_list* tienePeroNoQuiere2 = list_create();
	t_list* pokemonesDe2QueQuiere1 = list_create();

	listaQuiere1 = entrenador1->pokemonesQueQuiere;
	listaPosee1 = entrenador1->pokemonesQuePosee;

	diferenciaYCargarLista(listaQuiere1, listaPosee1, leFaltanParaObj1);

	listaQuiere2 = entrenador2->pokemonesQueQuiere;
	listaPosee2 = entrenador2->pokemonesQuePosee;

	diferenciaYCargarLista(listaPosee2, listaQuiere2, tienePeroNoQuiere2);

	diferenciaYCargarLista(leFaltanParaObj1, tienePeroNoQuiere2, pokemonesDe2QueQuiere1);

	if(list_is_empty(pokemonesDe2QueQuiere1)){
		//	list_add(entrenador1->pokemonesQuePosee, list_find(entrenador2->pokemonesQuePosee, bool(*closure)(void*))); //sea igual al primero de la lista de tiene pero no quiere 2
		//	list_remove_by_condition(entrenador2->pokemonesQuePosee, )); //TODO funcion interna con sonIguales
	} else{
		//  list_add(entrenador1->pokemonesQuePosee, list_find(entrenador2->pokemonesQuePosee, bool(*closure)(void*))); //sea igual al primero de la lista pokemonesDe2QueQuiere1
		//	list_remove_by_condition(entrenador2->pokemonesQuePosee, )); //TODO funcion interna con sonIguales
	}


	list_destroy(listaQuiere1);
	list_destroy(listaPosee1);
	list_destroy(listaQuiere2);
	list_destroy(listaPosee2);
	list_destroy(leFaltanParaObj1);
	list_destroy(tienePeroNoQuiere2);
	list_destroy(pokemonesDe2QueQuiere1); //TODO destruir elementos tambien?
}

t_entrenador* elegirConQuienIntercambiar(t_entrenador* entrenador){ //TODO probar

	t_list* listaQuiere1 = list_create();//TODO ELIMINAR listas
	t_list* listaPosee1 = list_create();

	listaQuiere1 = entrenador->pokemonesQueQuiere;
	listaPosee1 = entrenador->pokemonesQuePosee;

	t_list* leFaltanParaObj1 = list_create();

	diferenciaYCargarLista(listaQuiere1, listaPosee1, leFaltanParaObj1);

	t_list* listaQuiere2 = list_create();
	t_list* listaPosee2 = list_create();

	t_list* tienePeroNoQuiere2 = list_create();
	t_list* pokemonesDe2QueQuiere1 = list_create();

	t_list* sublistasPosiblesProveedoresDePokemon = list_create(); // TODO destruirla

	int tamanioDeadlock = list_size(listaBloqueadosDeadlock);

	for(int a=0; a< tamanioDeadlock; a++){

		t_entrenador* entrenador2 = list_get(listaBloqueadosDeadlock, a);

		listaQuiere2 = entrenador2->pokemonesQueQuiere;
		listaPosee2 = entrenador2->pokemonesQuePosee;

		diferenciaYCargarLista(listaPosee2, listaQuiere2, tienePeroNoQuiere2);

		diferenciaYCargarLista(leFaltanParaObj1, tienePeroNoQuiere2, pokemonesDe2QueQuiere1);

		if(!list_is_empty(pokemonesDe2QueQuiere1)){
			if(tengoAlgunPokemonQueQuiere2(entrenador, entrenador2)){
				return entrenador2;
			}

			list_add(sublistasPosiblesProveedoresDePokemon, entrenador2);
		}
		//TODO hacer clean a la lista
	}

	return list_get(sublistasPosiblesProveedoresDePokemon, 0);
}

int tengoAlgunPokemonQueQuiere2(t_entrenador* entrenador1,t_entrenador* entrenador2){

	t_list* listaQuiere1 = list_create();//TODO ELIMINAR listas
	t_list* listaPosee1 = list_create();

	listaQuiere1 = entrenador1;
	listaPosee1 = entrenador1->pokemonesQuePosee;

	t_list* tienePeroNoQuiere1 = list_create();

	diferenciaYCargarLista(listaPosee1, listaQuiere1, tienePeroNoQuiere1);

	t_list* listaQuiere2 = list_create();
	t_list* listaPosee2 = list_create();



	listaQuiere2 = entrenador2->pokemonesQueQuiere;
	listaPosee2 = entrenador2->pokemonesQuePosee;

	t_list* leFaltanParaObj2 = list_create();

	diferenciaYCargarLista(listaQuiere2, listaPosee2, leFaltanParaObj2);


	int tamanioFaltaParaObj2 = list_size(leFaltanParaObj2);
	int tamanioTienePeroNoQuiere1 = list_size(tienePeroNoQuiere1);

	for(int a=0; a< tamanioFaltaParaObj2; a++){

		for(int b=0; b<tamanioTienePeroNoQuiere1; b++){

			if(sonIguales(list_get(leFaltanParaObj2, a), list_get(tienePeroNoQuiere1, b))){
				return true;
			}
		}

	}

	return false;
}
