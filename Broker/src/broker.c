/*
 ============================================================================
 Name        : Broker

 Author      : Fran and Co
 Description : Proceso Broker
 ============================================================================
 */

#include "broker.h"

void sig_handler(int signo)
{
    if (signo == SIGUSR1)
    	memory_dump();
}

int main(void) {

	signal(SIGUSR1, sig_handler);

	init_config();
	init_memory();
	init_logger();
	init_message_queues();
	init_suscriber_lists();
	int server_socket = init_server();

	printf("broker!\n");
	fflush(stdout);

	while(1) {
		int potential_client_socket = esperar_cliente(server_socket);
		if(potential_client_socket > 0) {
			int* client_socket = (int*) malloc(sizeof(int));
			*client_socket = potential_client_socket;
			log_new_connection(*client_socket);
			fflush(stdout);
			pthread_create(&thread,NULL,(void*)serve_client,client_socket);
			pthread_detach(thread);
		}
	}

}

void serve_client(int* client_socket)
{
	char* nombre_recibido = NULL;
	uint32_t size_message = 0;
	t_paquete* paquete_recibido = recibir_paquete(*client_socket, &nombre_recibido, &size_message);

	if (paquete_recibido->codigo_operacion == SUSCRIPCION) {
		process_suscription((t_suscripcion_msg*) (paquete_recibido->mensaje), *client_socket);
	} else {
		process_new_message(paquete_recibido->codigo_operacion, paquete_recibido->id_correlativo, paquete_recibido->mensaje, size_message, *client_socket);
	}

	free_paquete_recibido(nombre_recibido, paquete_recibido);
}

void process_suscription(t_suscripcion_msg* subscription_msg, int socket_subscriptor)
{
	t_subscriber* subscriber;

	t_list* suscriptores = SUSCRIPTORES_MENSAJES[subscription_msg->tipo_cola];
	t_queue* queue = COLAS_MENSAJES[subscription_msg->tipo_cola];
	pthread_mutex_t mutex = MUTEX_SUSCRIPTORES[subscription_msg->tipo_cola];
	uint32_t cantidad_mensajes = size_message_queue(queue); // Como maximo sera del size de la lista, este valor es modificado en responder_a_suscriptor_nuevo
	t_list* mensajes_encolados = list_create();

	if (isSubscriberListed(suscriptores, subscription_msg->id_proceso)) {
		t_subscriber* subscriber_listed = get_subscriber_by_id(suscriptores, subscription_msg->id_proceso);
		subscriber_listed->socket_subscriber = socket_subscriptor;
		subscriber_listed->activo = 1;
		reply_to_new_subscriber(subscription_msg->tipo_cola, queue, subscriber_listed, &cantidad_mensajes, mensajes_encolados);

		subscriber = subscriber_listed;
	} else {
		subscriber = malloc(sizeof(*subscriber));
		subscriber->id_subscriber = subscription_msg->id_proceso;
		subscriber->socket_subscriber = socket_subscriptor;
		subscriber->activo = 1;
		subscribe_process(suscriptores, subscriber, mutex);

		reply_to_new_subscriber(subscription_msg->tipo_cola, queue, subscriber, &cantidad_mensajes, mensajes_encolados);
	}

	log_new_subscriber(subscription_msg->id_proceso, subscription_msg->tipo_cola);
	receive_ack(mensajes_encolados, cantidad_mensajes, subscriber);

	if (VICTIM_SELECTION_ALGORITHM == LRU) {
		for (int i=0; i < cantidad_mensajes; i++) {
			t_enqueued_message* mensaje_encolado = list_get(mensajes_encolados, i);
			notify_message_used(mensaje_encolado->ID);
		}
	}

	list_destroy(mensajes_encolados);

	remove_subscriber_if_temporal(suscriptores, subscriber, subscription_msg->temporal, mutex);
}

void process_new_message(op_code cod_op, uint32_t id_correlative, void* received_message, uint32_t size_message, int socket_cliente)
{
	uint32_t id_message = generate_id();

	t_queue* queue = COLAS_MENSAJES[cod_op];
	t_list* subscribers = SUSCRIPTORES_MENSAJES[cod_op];
	pthread_mutex_t mutex = MUTEX_COLAS[cod_op];

	uint32_t net_size_message = size_message - sizeof(uint32_t)*3; // Se resta el tamanio del cod. op, id e id correlativo que son 3 uint32_t
	if (cod_op != CAUGHT_POKEMON) {
		net_size_message--; // TODO ojo esto
	}
	t_copy_args* args = malloc(sizeof(*args));
	args->queue = cod_op;
	args->id = id_message;
	args->data = received_message;
	args->data_size = net_size_message;
	void* allocated_memory = memory_alloc(net_size_message);
	args->alloc = allocated_memory;
	void* allocated_message = memory_copy(args);
	free(args);
	free(received_message);

	pthread_mutex_lock(&mutex_deleted_messages_ids);
	int ids_count = 0;
	t_list* ids_messages_deleted = get_victim_messages_ids(&ids_count);

	if (ids_count > 0) {
		pthread_mutex_lock(&mutex);
		remove_messages_by_id(queue, ids_messages_deleted, ids_count);
		pthread_mutex_unlock(&mutex);
		notify_all_victim_messages_deleted();
	}
	pthread_mutex_unlock(&mutex_deleted_messages_ids);

	t_enqueued_message* mensaje_encolado = push_message_queue(queue, id_message, id_correlative, allocated_message, mutex);

	enviar_id_respuesta(id_message, socket_cliente);
	t_list* suscriptores_informados = inform_subscribers(cod_op, allocated_message, id_message, id_correlative, subscribers, mutex);
	mensaje_encolado->subscribers_informed = suscriptores_informados;
	log_new_message(id_message, cod_op);

	receive_multiples_ack(cod_op, id_message, suscriptores_informados);
}

void remove_subscriber_if_temporal(t_list* subscribers, t_subscriber* subscriber, uint32_t temporal, pthread_mutex_t mutex)
{
	if (temporal > 0) {
		sleep(temporal);
		close(subscriber->socket_subscriber);
		unsubscribe_process(subscribers, subscriber, mutex);
		free(subscriber);
	}
}

uint32_t generate_id()
{
	pthread_mutex_lock(&mutex_id_counter);
	uint32_t id_generado = ++ID_COUNTER;
	pthread_mutex_unlock(&mutex_id_counter);

	return id_generado;
}

t_list* inform_subscribers(op_code codigo, void* mensaje, uint32_t id, uint32_t id_correlativo, t_list* suscriptores, pthread_mutex_t mutex)
{
	t_list* suscriptores_informados = list_create();
	pthread_mutex_lock(&mutex);
	for (int i=0; i < list_size(suscriptores); i++) {
		t_subscriber* suscriptor = list_get(suscriptores, i);

		if (suscriptor->activo == 1) {
			if (enviar_mensaje(codigo, id, id_correlativo, mensaje, suscriptor->socket_subscriber) > 0) {
				list_add(suscriptores_informados, (void*)suscriptor);
				log_message_to_subscriber(suscriptor->id_subscriber, id);
			} else {
				suscriptor->activo = 0;
			}
		}

	}
	pthread_mutex_unlock(&mutex);

	return suscriptores_informados;
}

void receive_multiples_ack(op_code codigo, uint32_t id, t_list* suscriptores_informados)
{
	t_list* mensajes_encolados = list_create();
	t_queue* queue = COLAS_MENSAJES[codigo];
	t_enqueued_message* enqueued_message = find_message_by_id(queue, id);
	list_add(mensajes_encolados, enqueued_message);

	for (int i=0; i < list_size(suscriptores_informados); i++) {
		t_subscriber* suscriptor = list_get(suscriptores_informados, i);
		receive_ack(mensajes_encolados, 1, suscriptor);
	}

	list_destroy(mensajes_encolados);
}

void reply_to_new_subscriber(op_code code, t_queue* message_queue, t_subscriber* subscriber, uint32_t* messages_count, t_list* enqueue_messages)
{
	printf("Cantidad de mensajes en cola: %d\n", *messages_count);
	printf("Socket suscriptor: %d\n", subscriber->socket_subscriber);
	fflush(stdout);

	t_list* paquetes_serializados = list_create();
	t_list* tamanio_paquetes = list_create();
	uint32_t tamanio_stream = 0;

	pthread_mutex_t mutex = MUTEX_COLAS[code];
	pthread_mutex_lock(&mutex);
	for (int i=0; i < *messages_count; i++) {
		uint32_t bytes;
		t_enqueued_message* mensaje_encolado = get_message_by_index(message_queue, i);

		if(mensaje_encolado->message != NULL && !isSubscriberListed(mensaje_encolado->subscribers_ack, subscriber->id_subscriber)) {
			void* a_enviar = serializar_paquete(code, mensaje_encolado->ID, mensaje_encolado->ID_correlativo, mensaje_encolado->message, &bytes);
			bytes += sizeof(bytes);

			list_add(paquetes_serializados, a_enviar);
			list_add(tamanio_paquetes, &bytes);
			tamanio_stream += bytes;
			list_add(enqueue_messages, (void*) mensaje_encolado);
		}

	}
	pthread_mutex_unlock(&mutex);

	*messages_count = list_size(paquetes_serializados);

	send_enqueued_messages(*messages_count, tamanio_stream, paquetes_serializados, tamanio_paquetes, enqueue_messages, subscriber);

	list_destroy(tamanio_paquetes);
	list_destroy_and_destroy_elements(paquetes_serializados, free);
}

void send_enqueued_messages(uint32_t cantidad_mensajes, uint32_t tamanio_stream, t_list* paquetes_serializados, t_list* tamanio_paquetes, t_list* mensajes_encolados, t_subscriber* subscriber)
{
	void* a_enviar;
	int bytes_a_enviar = sizeof(cantidad_mensajes) + sizeof(tamanio_stream);
	if (cantidad_mensajes > 0) {
		a_enviar = malloc(tamanio_stream + sizeof(cantidad_mensajes) + sizeof(tamanio_stream));
		int offset = 0;
		memcpy(a_enviar + offset, &cantidad_mensajes, sizeof(cantidad_mensajes));
		offset += sizeof(cantidad_mensajes);
		memcpy(a_enviar + offset, &tamanio_stream, sizeof(tamanio_stream));
		offset += sizeof(tamanio_stream);

		for (int i=0; i < cantidad_mensajes; i++ ) {
			uint32_t* tamanio_paquete = (uint32_t*) list_get(tamanio_paquetes, i);
			void* paquete = list_get(paquetes_serializados, i);
			memcpy(a_enviar + offset, paquete, *tamanio_paquete);
			offset += *tamanio_paquete;
		}
		bytes_a_enviar += tamanio_stream;

	} else {
		a_enviar = malloc(sizeof(cantidad_mensajes) + sizeof(tamanio_stream));
		memcpy(a_enviar, &cantidad_mensajes, sizeof(cantidad_mensajes));
		memcpy(a_enviar + sizeof(cantidad_mensajes), &tamanio_stream, sizeof(tamanio_stream));
	}

	if (send(subscriber->socket_subscriber, a_enviar, bytes_a_enviar, MSG_NOSIGNAL) > 0) { //TODO podria ser >= 0
	printf("Respondí a la suscripción!\n");
		if (cantidad_mensajes > 0) {
			printf("Envié mensajes encolados!\n");
			add_new_informed_subscriber_to_mq(mensajes_encolados, cantidad_mensajes, subscriber);
		}
	} else {
		printf("El suscriptor está inactivo!\n");
		subscriber->activo = 0;
	}

	free(a_enviar);
}

void receive_ack(t_list* mensajes_encolados, uint32_t cantidad_mensajes, t_subscriber* subscriber)
{
	uint32_t response_status = 0;
	int status = recv(subscriber->socket_subscriber, &response_status, sizeof(response_status), MSG_WAITALL);

	if(status > 0 && response_status == 200) {
		add_new_ack_suscriber_to_mq(mensajes_encolados, cantidad_mensajes, subscriber);
	}
}

int init_server()
{
	char* IP = config_get_string_value(CONFIG,"IP_BROKER");
	char* PUERTO = config_get_string_value(CONFIG,"PUERTO_BROKER");
	return iniciar_servidor(IP, PUERTO);
}

t_selection_algorithm choose_partition_algorithm()
{
	char* partition_algorithm = config_get_string_value(CONFIG,"ALGORITMO_PARTICION_LIBRE");
	if (strcmp(partition_algorithm, "FF") == 0) {
		return FIRST_FIT;
	} else if (strcmp(partition_algorithm, "BF") == 0) {
		return BEST_FIT;
	}
	return NONE;
}

t_selection_algorithm choose_victim_algorithm()
{
	char* victim_algorithm = config_get_string_value(CONFIG,"ALGORITMO_REEMPLAZO");
	if (strcmp(victim_algorithm, "FIFO") == 0) {
		return FIFO;
	} else if (strcmp(victim_algorithm, "LRU") == 0) {
		return LRU;
	}
	return NONE;
}

void init_memory()
{
	int size_memory = config_get_int_value(CONFIG,"TAMANO_MEMORIA");

	t_memory_algorithm memory_alg;
	t_selection_algorithm victim_alg;
	t_selection_algorithm partition_alg;

	char* mem_algorithm = config_get_string_value(CONFIG,"ALGORITMO_MEMORIA");
	if (strcmp(mem_algorithm, "PARTICIONES") == 0) {
		memory_alg = DYNAMIC_PARTITIONS;
		victim_alg = choose_victim_algorithm();
		partition_alg = choose_partition_algorithm();
	} else if (strcmp(mem_algorithm, "BS") == 0) {
		memory_alg = BUDDY_SYSTEM;
		victim_alg = choose_victim_algorithm();
		partition_alg = NONE;
	}

	int min_part_size = config_get_int_value(CONFIG,"TAMANO_MINIMO_PARTICION");
	int freq_compact  = config_get_int_value(CONFIG,"FRECUENCIA_COMPACTACION");
	char* dump_path = config_get_string_value(CONFIG,"DUMP_FILE");

	load_memory(size_memory, min_part_size, freq_compact, memory_alg, victim_alg, partition_alg, dump_path);
}

void init_message_queues()
{
	NEW_POKEMON_QUEUE = create_message_queue();
	APPEARED_POKEMON_QUEUE = create_message_queue();
	CATCH_POKEMON_QUEUE = create_message_queue();
	CAUGHT_POKEMON_QUEUE = create_message_queue();
	GET_POKEMON_QUEUE = create_message_queue();
	LOCALIZED_POKEMON_QUEUE = create_message_queue();

	COLAS_MENSAJES[1] = NEW_POKEMON_QUEUE;
	COLAS_MENSAJES[2] = APPEARED_POKEMON_QUEUE;
	COLAS_MENSAJES[3] = CATCH_POKEMON_QUEUE;
	COLAS_MENSAJES[4] = CAUGHT_POKEMON_QUEUE;
	COLAS_MENSAJES[5] = GET_POKEMON_QUEUE;
	COLAS_MENSAJES[6] = LOCALIZED_POKEMON_QUEUE;

	MUTEX_COLAS[1] = mutex_new_queue;
	MUTEX_COLAS[2] = mutex_appeared_queue;
	MUTEX_COLAS[3] = mutex_get_queue;
	MUTEX_COLAS[4] = mutex_localized_queue;
	MUTEX_COLAS[5] = mutex_catch_queue;
	MUTEX_COLAS[6] = mutex_caught_queue;
}

void init_suscriber_lists()
{
	NEW_POKEMON_SUBSCRIBERS = list_create();
	APPEARED_POKEMON_SUBSCRIBERS = list_create();
	CATCH_POKEMON_SUBSCRIBERS = list_create();
	CAUGHT_POKEMON_SUBSCRIBERS = list_create();
	GET_POKEMON_SUBSCRIBERS = list_create();
	LOCALIZED_POKEMON_SUBSCRIBERS = list_create();

	SUSCRIPTORES_MENSAJES[1] = NEW_POKEMON_SUBSCRIBERS;
	SUSCRIPTORES_MENSAJES[2] = APPEARED_POKEMON_SUBSCRIBERS;
	SUSCRIPTORES_MENSAJES[3] = CATCH_POKEMON_SUBSCRIBERS;
	SUSCRIPTORES_MENSAJES[4] = CAUGHT_POKEMON_SUBSCRIBERS;
	SUSCRIPTORES_MENSAJES[5] = GET_POKEMON_SUBSCRIBERS;
	SUSCRIPTORES_MENSAJES[6] = LOCALIZED_POKEMON_SUBSCRIBERS;

	MUTEX_SUSCRIPTORES[1] = mutex_new_susc;
	MUTEX_SUSCRIPTORES[2] = mutex_appeared_susc;
	MUTEX_SUSCRIPTORES[3] = mutex_get_susc;
	MUTEX_SUSCRIPTORES[4] = mutex_localized_susc;
	MUTEX_SUSCRIPTORES[5] = mutex_catch_susc;
	MUTEX_SUSCRIPTORES[6] = mutex_caught_susc;
}

void init_logger()
{
	char* broker_log = config_get_string_value(CONFIG,"LOG_FILE");
	LOGGER = log_create(broker_log, BROKER_NAME, false, LOG_LEVEL_INFO);
}

void init_config()
{
	CONFIG = config_create(BROKER_CONFIG);
}

void destroy_all_mutex()
{
	pthread_mutex_destroy(&mutex_id_counter);
	pthread_mutex_destroy(&mutex_new_queue);
	pthread_mutex_destroy(&mutex_appeared_queue);
	pthread_mutex_destroy(&mutex_get_queue);
	pthread_mutex_destroy(&mutex_localized_queue);
	pthread_mutex_destroy(&mutex_catch_queue);
	pthread_mutex_destroy(&mutex_caught_queue);
	pthread_mutex_destroy(&mutex_new_susc);
	pthread_mutex_destroy(&mutex_appeared_susc);
	pthread_mutex_destroy(&mutex_get_susc);
	pthread_mutex_destroy(&mutex_localized_susc);
	pthread_mutex_destroy(&mutex_catch_susc);
	pthread_mutex_destroy(&mutex_caught_susc);
}

void terminar_programa(int socket_servidor, t_log* logger)
{
	destroy_all_mutex();
	liberar_conexion(socket_servidor);
	log_destroy(logger);
	config_destroy(CONFIG);
}

