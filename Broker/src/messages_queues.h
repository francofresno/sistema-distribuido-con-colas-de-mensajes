/*
 ============================================================================
 Name        : Messages Queues
 Author      : Fran and Co
 Description : Header Funciones de creación de colas que en realidad no son colas
 ============================================================================
 */

#ifndef MESSAGES_QUEUES_H_
#define MESSAGES_QUEUES_H_

#include<stdlib.h>
#include<stdint.h>
#include<pthread.h>

#include<commons/collections/queue.h>

#include "logger.h"

typedef struct
{
	uint32_t ID;
	uint32_t ID_correlativo;
	t_list* suscribers_informed;
	t_list* suscribers_ack;
	void* message;
} t_enqueued_message;

typedef struct
{
	uint32_t id_suscriptor;
	int socket_suscriptor;
	int activo;
} t_subscriber;

t_queue* create_message_queue();
t_enqueued_message* push_message_queue(t_queue* queue, uint32_t ID, uint32_t ID_correlativo, void* message, pthread_mutex_t mutex);
t_enqueued_message* pop_message_queue(t_queue* queue, pthread_mutex_t mutex);
t_enqueued_message* get_message_by_index(t_queue* queue, int index);
void inform_message_sent_to(t_enqueued_message* data, t_subscriber* subscriber);
void inform_message_ack_from(t_enqueued_message* data, t_subscriber* subscriber);
t_enqueued_message* find_message_by_id(t_queue* queue, uint32_t id);
t_enqueued_message* find_message_by_id_correlativo(t_queue* queue, uint32_t id);
void remove_message_by_id(t_queue* queue, uint32_t id);
void remove_message_by_id_correlativo(t_queue* queue, uint32_t id);
void element_destroyer_mq(void* data);
int size_message_queue(t_queue* queue);
int is_empty_message_queue(t_queue* queue);
void free_message_queue(t_queue* queue);
void add_new_informed_subscriber_to_mq(t_list* messages_in_queue, uint32_t number_of_messages, t_subscriber* subscriber, t_log* logger);
void add_new_ack_suscriber_to_mq(t_list* messages_in_queue, uint32_t number_of_messages, t_subscriber* subscriber, t_log* logger);

void subscribe_process(t_list* subscribers, t_subscriber* subscriber, pthread_mutex_t mutex);
void unsubscribe_process(t_list* subscribers, t_subscriber* subscriber, pthread_mutex_t mutex);
/*
 * @NAME: get_index_of_subscriber
 * @RETURN: -1 in case of error
 */
int get_index_of_subscriber(t_list* subscribers, uint32_t id_subscriber);
t_subscriber* get_subscriber_by_id(t_list* subscribers, uint32_t id_subscriber);
int isSubscriberListed(t_list* subscribers, uint32_t id_subscriber);
void free_subscribers_list(t_list* subscribers);

#endif /* MESSAGES_QUEUES_H_ */
