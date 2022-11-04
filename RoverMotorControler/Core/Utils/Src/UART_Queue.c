/*
 * UART_Queue.c
 *
 *  Created on: Oct 26, 2022
 *      Author: Lukasz
 */

#include "UART_Queue.h"

UART_QueueStatusTypeDef UART_Queue_Init(UART_QueueTypeDef* pQueue, uint32_t max_size){
	/*Just set default values to the queue*/
	pQueue->MAX_QUEUE_SIZE = max_size;
	pQueue->pFirst = NULL;
	pQueue->pLast = NULL;
	pQueue->Size = 0;

	return QUEUE_OK;
}

UART_QueueStatusTypeDef UART_Queue_Enqueue(UART_QueueTypeDef* pQueue, uint8_t byte) {
	/*if queue is full just return status message*/
	if (pQueue->Size >= pQueue->MAX_QUEUE_SIZE)
		return QUEUE_FULL_BYTE_DISCARDED;

	/*Allocate memory for new element*/
	UART_QueueElementTypeDef* elementToEnqueue = (UART_QueueElementTypeDef*)malloc(sizeof(UART_QueueElementTypeDef));

	/*If pointer is null it means that malloc failed, return message*/
	if (elementToEnqueue == NULL)
	  	return QUEUE_FAILED_TO_MALLOC;

	/*Initialize new element*/
	elementToEnqueue->byte = byte;
  	elementToEnqueue->pNext = NULL;

  	if (pQueue->Size == 0) {
  		/*If queue size is 0, pFirst and pLast pointers
  		 * are set to the pointer of the new element */
  		pQueue->pFirst = elementToEnqueue;
  		pQueue->pLast = elementToEnqueue;
  	} else {
  		/* pNext pointer of penultimate element is set to new element,
  		 * pLast pointer is updated */
  		pQueue->pLast->pNext = elementToEnqueue;
  		pQueue->pLast = elementToEnqueue;
  	}

  	/*Increase size of the queue*/
  	pQueue->Size++;

  	/*everything went well, we can return OK message*/
	return QUEUE_OK;
}

UART_QueueStatusTypeDef UART_Queue_Dequeue(UART_QueueTypeDef* pQueue, uint8_t* pByte){
	/*can't dequeue from empty list*/
	if (pQueue->Size <= 0)
		return QUEUE_EMPTY;

	/*Store pointer of the first element so it is not lost*/
	UART_QueueElementTypeDef* elementToDequeue = pQueue->pFirst;
	/*Read the value of the first element*/
	uint8_t value = elementToDequeue->byte;

	/*Update pFirst pointer to be pointing to second element*/
	pQueue->pFirst = elementToDequeue->pNext;

	/*Decrease size of the queue*/
	pQueue->Size--;

	/*if pByte is NULL data is discarded*/
	if(pByte != NULL)
		(*pByte) = value;

	// remove first element
	free(elementToDequeue);

	/*everything went well, we can return OK message*/
	return QUEUE_OK;
}

UART_QueueStatusTypeDef UART_Queue_Dispose(UART_QueueTypeDef* pQueue){
	/*Dequeues elements while queue is not empty*/
	while (pQueue->Size > 0) {
		/*check if we managed to dequeued element successfully*/
		if(UART_Queue_Dequeue(pQueue, NULL) != QUEUE_OK)
			return QUEUE_FAILED_TO_DISPOSE; //fatal error
	}

	/*everything went well, we can return OK message*/
	return QUEUE_OK;
}

