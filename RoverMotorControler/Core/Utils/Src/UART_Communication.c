/*
 * UARTCommunication.c
 *
 *  Created on: Oct 25, 2022
 *      Author: Lukasz
 */
#include "UART_Communication.h"


UART_CommunicationStatusTypeDef UART_Communication_Init(UART_CommunicationTypeDef* pCommunication, UART_HandleTypeDef* huart, uint8_t frame_start, uint32_t queue_size){
	if(pCommunication == NULL)
			return COMMUNICATION_NULL_ERROR;

	/*Initialize fields of the UART_Communication structure*/
	pCommunication->HAL_UART_Handle = huart;

	pCommunication->ReceivedByte = 0;
	pCommunication->FrameStartByte = frame_start;

	pCommunication->Transsmision = false;

	pCommunication->pRegisteredCallbacks = NULL;
	pCommunication->RegisteredCallbacksCount = 0;

	__uart_frame_init(&pCommunication->CurrentFrame);

	if(UART_Queue_Init(&pCommunication->ReadBytesQueue, queue_size) != QUEUE_OK){
		return COMMUNICATION_QUEUE_FAILED;
	};

	if(UART_Queue_Init(&pCommunication->WriteBytesQueue, queue_size) != QUEUE_OK){
		return COMMUNICATION_QUEUE_FAILED;
	};

	//We have to start interrupts "reading chain", from this point every successful read will start another
	if(HAL_UART_Receive_IT(huart, &pCommunication->ReceivedByte, 1) != HAL_OK){
		return COMMUNICATION_HAL_ERROR;
	}
	return COMMUNICATION_OK;
}

UART_CommunicationStatusTypeDef UART_Communication_Register_Callback(UART_CommunicationTypeDef* pCommunication, uint8_t ID, void (*pCallback)(uint8_t len, uint8_t* payload)){
	if(pCommunication == NULL)
			return COMMUNICATION_NULL_ERROR;

	//reallocate more memory for new callback
	pCommunication->pRegisteredCallbacks = realloc(pCommunication->pRegisteredCallbacks, sizeof(UART_CallbackTypeDef)*(pCommunication->RegisteredCallbacksCount+1));

	if(pCommunication->pRegisteredCallbacks == NULL){
		return COMMUNICATION_MALLOC_ERROR;
	}

	//assign callback and id
	pCommunication->pRegisteredCallbacks[pCommunication->RegisteredCallbacksCount].ID = ID;
	pCommunication->pRegisteredCallbacks[pCommunication->RegisteredCallbacksCount].pCallback = pCallback;

	//increase size of registered callbacks
	pCommunication->RegisteredCallbacksCount++;

	return COMMUNICATION_OK;
}

UART_CommunicationStatusTypeDef UART_Communication_Update(UART_CommunicationTypeDef* pCommunication){
	if(pCommunication == NULL)
		return COMMUNICATION_NULL_ERROR;

	uint8_t data;
	//try to dequeue one byte from queue
	if(UART_Queue_Dequeue(&pCommunication->ReadBytesQueue, &data) == QUEUE_OK){
		if(data == pCommunication->FrameStartByte){
			//if we have received frame start we cant update state of the current frame to next stage
			if(pCommunication->CurrentFrame.State != REQUEST_EMPTY){
				//clean up if something went wrong
				free(pCommunication->CurrentFrame.pPayload);
				__uart_frame_init(&pCommunication->CurrentFrame);
			}
			pCommunication->CurrentFrame.State = WAITING_FOR_ID;
		} else {
			switch (pCommunication->CurrentFrame.State){
				case WAITING_FOR_ID:
					//we have received ID of the frame
					pCommunication->CurrentFrame.ID = data;
					int index;
					//we can try to find suitable callback
					if(__find_callback(pCommunication, pCommunication->CurrentFrame.ID, &index) == COMMUNICATION_OK){
						//if we have found callback we can assign it in current structure
						pCommunication->CurrentFrame.pCallback = pCommunication->pRegisteredCallbacks[index].pCallback;
					}
					pCommunication->CurrentFrame.State++; //progress to next state
					break;
				case WAITING_FOR_LEN:
					//we have received length of the payload
					pCommunication->CurrentFrame.FinalLength = data;
					//we can allocate memory to store whole payload
					pCommunication->CurrentFrame.pPayload = malloc(sizeof(uint8_t)*pCommunication->CurrentFrame.FinalLength);
					if(pCommunication->CurrentFrame.pPayload == NULL){
						return COMMUNICATION_MALLOC_ERROR;
					}
					pCommunication->CurrentFrame.State++; //progress to next state
					break;
				case WAITING_FOR_PAYLOAD:
					//we have received one byte of payload
					pCommunication->CurrentFrame.pPayload[pCommunication->CurrentFrame.CurrentLength] = data;
					pCommunication->CurrentFrame.CurrentLength++;

					//we can progress to next stage only if we have received whole payload
					if(pCommunication->CurrentFrame.CurrentLength == pCommunication->CurrentFrame.FinalLength)
						pCommunication->CurrentFrame.State++; // progress to next state
					break;
				default:
					//something went wrong, probably bad data
					return COMMUNICATION_UNKNOWN_DATA;
					break;
			}
		}
	};

	//check if current frame is complete
	if(pCommunication->CurrentFrame.State == REQUEST_COMPLETE){
		//if true, we have to check if we have found suitable callback for the request
		if(pCommunication->CurrentFrame.pCallback != NULL){
			//we can call the callback
			pCommunication->CurrentFrame.pCallback(pCommunication->CurrentFrame.FinalLength, pCommunication->CurrentFrame.pPayload);
			//we need to free data allocated when we received length of the callback
			free(pCommunication->CurrentFrame.pPayload);
		}
		//we have completed the request => we can restore current frame to default state
		__uart_frame_init(&pCommunication->CurrentFrame);
	}

	//this if checks if we need to start transmission,
	//we need to call this function only once per transmission,
	//because next time it will be called in "interrupts chain"
	if(pCommunication->Transsmision == false && pCommunication->WriteBytesQueue.Size > 0){
		//flag is needed to make sure we only begin the transmission
		pCommunication->Transsmision = true;
		UART_Communication_Transmit_Interrupt_Callback(pCommunication);
	}

	return COMMUNICATION_OK;
}

UART_CommunicationStatusTypeDef UART_Communication_Clean(UART_CommunicationTypeDef* pCommunication){
	if(pCommunication == NULL)
			return COMMUNICATION_NULL_ERROR;

	/*Just clean up queues and registered callbacks*/

	if(UART_Queue_Dispose(&pCommunication->ReadBytesQueue) != QUEUE_OK){
		return COMMUNICATION_QUEUE_FAILED;
	}

	if(UART_Queue_Dispose(&pCommunication->WriteBytesQueue) != QUEUE_OK){
		return COMMUNICATION_QUEUE_FAILED;
	}

	free(pCommunication->pRegisteredCallbacks);

	return COMMUNICATION_OK;
}

/*Definition of callback for receive transmission end interrupt*/
UART_CommunicationStatusTypeDef UART_Communication_Receive_Interrupt_Callback(UART_CommunicationTypeDef* pCommunication){
	if(pCommunication == NULL)
			return COMMUNICATION_NULL_ERROR;

	if(UART_Queue_Enqueue(&pCommunication->ReadBytesQueue, pCommunication->ReceivedByte) != QUEUE_OK){
		return COMMUNICATION_QUEUE_FAILED;
	}

	//Start next read, when reading has ended this callback should be called again
	if(HAL_UART_Receive_IT(pCommunication->HAL_UART_Handle, &pCommunication->ReceivedByte, 1) != HAL_OK)
		return COMMUNICATION_HAL_ERROR;

	return COMMUNICATION_OK;
}

/*This callback sends one byte from a queue, it is called when previous transimission has ended*/
UART_CommunicationStatusTypeDef UART_Communication_Transmit_Interrupt_Callback(UART_CommunicationTypeDef* pCommunication){
	if(pCommunication == NULL)
			return COMMUNICATION_NULL_ERROR;

	uint8_t byte;
	//try dequeuing one byte
	if(UART_Queue_Dequeue(&pCommunication->WriteBytesQueue, &byte) == QUEUE_OK){
		//send one byte
		if(HAL_UART_Transmit_IT(pCommunication->HAL_UART_Handle, &byte, 1) != HAL_OK)
			return COMMUNICATION_HAL_ERROR;
	} else {
		//that means that queue is empty, all bytes send, transmission has ended
		pCommunication->Transsmision = false;
	}

	return COMMUNICATION_OK;
}

/*Definition of __io_putchar(int ch) function from syscalls.c files, enables us to use printf to wrtie to uart*/
UART_CommunicationStatusTypeDef UART_Communication__io_put_char(UART_CommunicationTypeDef* pCommunication, int ch){
	if(pCommunication == NULL)
			return COMMUNICATION_NULL_ERROR;

	//fix, for some terminals we have to set \r before \n for proper new line,
	if(ch == '\n'){
		uint8_t ch2 = '\r';
		if(UART_Queue_Enqueue(&pCommunication->WriteBytesQueue, (uint8_t)ch2) != QUEUE_OK){
			return COMMUNICATION_QUEUE_FAILED;
		}
	}

	//try to enqueue byte to writing queue
	if(UART_Queue_Enqueue(&pCommunication->WriteBytesQueue, (uint8_t)ch) != QUEUE_OK){
		return COMMUNICATION_QUEUE_FAILED;
	}
	return COMMUNICATION_OK;
}

UART_CommunicationStatusTypeDef __find_callback(UART_CommunicationTypeDef* pCommunication, uint8_t ID, int* frame_index){
	if(pCommunication == NULL)
			return COMMUNICATION_NULL_ERROR;

	uint8_t i = 0;
	//just iterate through all registered callbacks
	for(; i < pCommunication->RegisteredCallbacksCount; i++){
		if(pCommunication->pRegisteredCallbacks[i].ID == ID){
			//that means we have found suitable callback, we can assign value and return
			(*frame_index) = i;
			return COMMUNICATION_OK;
		}
	}
	//we didn't find suitable callback, return -1 as not found signal
	(*frame_index) = -1;
	return COMMUNICATION_CALLBACK_NOT_FOUND;
}

UART_CommunicationStatusTypeDef __uart_frame_init(UART_FrameTypeDef* frame){
	if(frame == NULL)
		return COMMUNICATION_NULL_ERROR;
	/*Just reset frame struct to default state*/
	frame->ID = 0;
	frame->State = REQUEST_EMPTY;
	frame->FinalLength = 0;
	frame->CurrentLength = 0;
	frame->pPayload = NULL;
	frame->pCallback = NULL;

	return COMMUNICATION_OK;
}


