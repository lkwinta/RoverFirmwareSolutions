#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef UART_QUEUE_H_
#define UART_QUEUE_H_

/* Small and simple implementation of linked-list based
 * queue which will be used to store byte's to be processed*/

/* Enum type for basic exception handling*/
typedef enum {
  QUEUE_OK, // returned when operation was successful
  QUEUE_FAILED_TO_MALLOC, // returned if malloc failed during enqueuing the element
  QUEUE_FULL_BYTE_DISCARDED, //returned if attempted to enqueue data to full queue
  QUEUE_EMPTY, //returned if tried to dequeue empty queue
  QUEUE_FAILED_TO_DISPOSE //returned when dispose function failed to dequeue element (possible memory leak)
} UART_QueueStatusTypeDef;

/* Single element of linked-queue. Each element
 * stores one byte of data and pointer to next
 * element in queue */

typedef struct UART_QueueElementTypeDef {
	uint8_t byte; //one byte of data
	struct UART_QueueElementTypeDef *pNext; //pointer to next element
} UART_QueueElementTypeDef;

/* The actual queue*/
typedef struct {
	/* Defines max size of the queue, it prevents from
	 * uncontrolled memory usage when something goes wrong
	 * with dequeuing the data or in main loop,
	 * all data which will be enqueued */
	uint16_t MAX_QUEUE_SIZE;

	/* Pointer to the first element of the queue*/
	UART_QueueElementTypeDef *pFirst;
	/* Pointer to the last element of the queue,
	 * storing it reduces time complexity needed to add next element to
	 * the queue*/
	UART_QueueElementTypeDef *pLast;

	/* Actual size of the queue*/
	uint32_t Size;
} UART_QueueTypeDef;

/*
 * @brief Initializes queue pointers with NULL, and sets its max size
 *
 * @param pQueue pointer to queue
 * @param max_size max length of the queue
 *
 * @retval QUEUE_STATUS
 * */
extern UART_QueueStatusTypeDef UART_Queue_Init(UART_QueueTypeDef* pQueue, uint32_t max_size);

/*
 * @brief Enqueues one byte to the queue
 *
 * @param pQueue pointer to queue
 * @param byte one byte of data to be enqueued
 *
 * @retval QUEUE_STATUS
 * */
extern UART_QueueStatusTypeDef UART_Queue_Enqueue(UART_QueueTypeDef* pQueue, uint8_t byte);

/*
 * @brief Dequeues one byte from the queue
 *
 * @param pQueue pointer to queue
 * @param pByte pointer to memory where dequeued value will be stored
 *
 * @retval QUEUE_STATUS
 * */
extern UART_QueueStatusTypeDef UART_Queue_Dequeue(UART_QueueTypeDef* pQueue, uint8_t* pByte);

/*
 * @brief Dequeues all remaining data from the queue, stored values are lost
 *
 * @param pQueue pointer to queue
 *
 * @retval QUEUE_STATUS
 * */
extern UART_QueueStatusTypeDef UART_Queue_Dispose(UART_QueueTypeDef* pQueue);

#endif
