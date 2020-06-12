/*
 ============================================================================
 Name        : Broker
 Author      : Fran and Co
 Description : Buddy System Algorithm Header
 ============================================================================
 */

#ifndef BUDDY_SYSTEM_H_
#define BUDDY_SYSTEM_H_

#include "memory_commons.h"

void buddy_init();
void* buddy_alloc(int size);

#endif /* BUDDY_SYSTEM_H_ */
