/*UARTCommunication.h*/
#include "stm32g4xx_hal.h"
#include <stdlib.h>
#include <stddef.h>

#include "UART_Queue.h"

/*
 * ALGORITHM
 * 1. UART_Init() calls first HAL_UART_Receive_IT()
 * 2. When data is ready to read HAL_UART_RxCpltCallback() is called, it should call my UART_Communication_Register_Callback function
 * 			- It places read byte in queue to be processed by main loop in UART_Communication_Update()
 * 			- It calls next HAL_UART_Receive_IT() which will read next incoming byte and prolong read cycle
 * 3. UART_Communication_Update() will take first byte from queue interpret it
 * 		- it will create UART_FrameTypeDef object based on ID
 * 		- with each byte UART_RequestStateTypeDef is updated,
 * 		- when UART_FrameTypeDef is complete (UART_FrameTypeDef.State = REQUEST_COMPLETE) its callback will be called with required parameters
 *
 *
 * */
#ifndef UART_COMMUNICATION_H_
#define UART_COMMUNICATION_H_

/*define simple bool type (just for better code readability)*/
#define true 1
#define false 0
typedef uint8_t bool;

/*
 * Return type of all functions
 * (simple error checking)
 * */
typedef enum {
	COMMUNICATION_OK, //Everything fine
	COMMUNICATION_MALLOC_ERROR, // allocating memory went wrong
	COMMUNICATION_HAL_ERROR, //something went wrong with HAL calls
	COMMUNICATION_CALLBACK_NOT_FOUND, //callback was not found
	COMMUNICATION_NULL_ERROR, //pointer passed as an argument was null
	COMMUNICATION_QUEUE_FAILED, // something went wrong with enqueue() dequeue()
	COMMUNICATION_UNKNOWN_DATA //unknown data processed in Upddate()
} UART_CommunicationStatusTypeDef;

/*
 * Structure that will hold registered callback in memory
 * Because length of the payload is specified in the frame it is
 * unnecessary to save it in memory
 * */
typedef struct {
	/*ID of the frame*/
	uint8_t ID;
	/*Pointer to callback function*/
	void (*pCallback)(uint8_t len, uint8_t* payload);
} UART_CallbackTypeDef;

/*
 * Enum which defines state of the current frame data collection
 * */
typedef enum {
	//Frame not started yet
	REQUEST_EMPTY = 0,
	//next byte will be frame ID
	WAITING_FOR_ID = 1,
	//Next byte will be interpreted as payload length
	WAITING_FOR_LEN = 2,
	//Next bytes received will be stored as payload
	WAITING_FOR_PAYLOAD = 3,
	//Request ready to call callback
	REQUEST_COMPLETE = 4
} UART_RequestStateTypeDef;

/*
 * This struct will store frame that is currently being created,
 * it will be filed with each next byte coming
 * */
typedef struct {
	/*Represents in which o*/
	UART_RequestStateTypeDef State;

	/*Frame ID*/
	uint8_t ID;
	/*Size (in bytes) of the payload*/
	uint8_t FinalLength;
	/*Stores number of bytes currently read*/
	uint8_t CurrentLength;

	/*pointer to array to store payloads*/
	uint8_t* pPayload;

	/*Function pointer for frame callback*/
	void (*pCallback)(uint8_t len, uint8_t* payload);

} UART_FrameTypeDef;

/*
 * Structure that handles all variables required for correct communication
 *
 * */
typedef struct {
	//Handle for HAL's UART struct
	UART_HandleTypeDef* HAL_UART_Handle;

	//Last received byte
	uint8_t ReceivedByte;
	//Symbol of frame start
	uint8_t FrameStartByte;

	//Flag if transmission has been already started
	bool Transsmision;

	//Pointer to callbacks array
	UART_CallbackTypeDef* pRegisteredCallbacks;
	//Count of already registered callbacks
	uint8_t RegisteredCallbacksCount;

	//queue used to store received bytes
	UART_QueueTypeDef ReadBytesQueue;
	//queue used to store data to be sent
	UART_QueueTypeDef WriteBytesQueue;

	//current frame to be decode received bytes
	UART_FrameTypeDef CurrentFrame;
} UART_CommunicationTypeDef;

/*
 * @brief Function that will initialize all library components
 *
 * @param pCommunication pointer to UART_Communication handle
 * @param huart pointer to HAL UART structure representing uart port we want to use
 * @param frame_start defines what is the frame start byte
 * @param queue_size define max size in bytes for read and write bytes queues
 *
 * @retval UART_CommunicationStatusTypeDef status if function was executed successfully
 * */
extern UART_CommunicationStatusTypeDef UART_Communication_Init(UART_CommunicationTypeDef* pCommunication, UART_HandleTypeDef* huart, uint8_t frame_start, uint32_t queue_size);

/*
 * @brief Function that registers possible frames and saves them in registered_frames array
 *
 * @param pCommunication pointer to UART_Communication handle
 * @param ID ID of the frame to register
 * @param pCallback pointer to callback function
 *
 * @retval UART_CommunicationStatusTypeDef status if function was executed successfully
 * */
extern UART_CommunicationStatusTypeDef UART_Communication_Register_Callback(UART_CommunicationTypeDef* pCommunication, uint8_t ID, void (*pCallback)(uint8_t len, uint8_t* payload));

/*
 * @brief Function that will be called in main loop, fills in current frame struct and calls callbacks
 *
 * @param pCommunication pointer to UART_Communication handle
 *
 * @retval UART_CommunicationStatusTypeDef status if function was executed successfully
 * */
extern UART_CommunicationStatusTypeDef UART_Communication_Update(UART_CommunicationTypeDef* pCommunication);

/*
 * @brief Dequeues any byte left in queue, cleans registered callbacks
 *
 * @param pCommunication pointer to UART_Communication handle
 *
 * @retval UART_CommunicationStatusTypeDef status if function was executed successfully
 * */
extern UART_CommunicationStatusTypeDef UART_Communication_Clean(UART_CommunicationTypeDef* pCommunication);

/*
 * @brief callback that should be called in HAL interrupt handler
 *
 * @param pCommunication pointer to UART_Communication handle
 *
 * @retval UART_CommunicationStatusTypeDef status if function was executed successfully
 * */
extern UART_CommunicationStatusTypeDef UART_Communication_Transmit_Interrupt_Callback(UART_CommunicationTypeDef* pCommunication);

/*
 * @brief callback that should be called in HAL interrupt handler
 *
 * @param pCommunication pointer to UART_Communication handle
 *
 * @retval UART_CommunicationStatusTypeDef status if function was executed successfully
 * */
extern UART_CommunicationStatusTypeDef UART_Communication_Receive_Interrupt_Callback(UART_CommunicationTypeDef* pCommunication);

/*
 * @brief function that sends character via UART, should be called in __io_putchar from syscall.c
 *
 * @param pCommunication pointer to UART_Communication handle
 * @param ch character that will be sent via UART
 *
 * @retval UART_CommunicationStatusTypeDef status if function was executed successfully
 * */
extern UART_CommunicationStatusTypeDef UART_Communication__io_put_char(UART_CommunicationTypeDef* pCommunication, int ch);

/*
 * @brief Searches all registered frames by their ID, if frame with specified ID not found returns -1
 *
 * @param pCommunication pointer to UART_Communication handle
 * @param ID frame ID that will be executed
 * @param index of the found frame
 *
 * @retval UART_CommunicationStatusTypeDef status if function was executed successfully
 * */
extern UART_CommunicationStatusTypeDef __find_callback(UART_CommunicationTypeDef* pCommunication, uint8_t ID, int* frame_index);

/*
 * @brief Clears UART_Frame structure to default values
 *
 * @param frame pointer to the frame we want to initialize
 *
 * @retval UART_CommunicationStatusTypeDef status if function was executed successfully
 * */
extern UART_CommunicationStatusTypeDef __uart_frame_init(UART_FrameTypeDef* frame);
#endif
