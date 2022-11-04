# RoverMotorControler

Biblioteka sama w sobie znajduje się w Core/Utils. W pliku main.c wykorzystałem tą bibliotekę do komunikacji przez UART. Zdecydowałem się aby nie definiować 
callback-ów UART wywoływanych przez biblotekę HAL w samej bibliotece aby nie definiować jednego zachowania dla wszystkich portów UART.
Komunikacja przez UART odbywa się w ciągu przerwań. Każdy następny bajt danych jest odbierany dopiero gdy transmisja poprzedniego została zakończona. Tak samo
każdy kolejny bajt wysyłany jest gdy został wysłany poprzedni. W celu dynamicznego zarządzania pamięcią zaimplementowałem mechanizm kolejki w `UART_Queue.h`

Same callbacki ramek też zdefiniowałem w main.c, lecz nic nie stoi na przeszkodzie by były gdzieś indziej, konstrukcja mojej biblioteki umożliwia łatwe 
dodawanie nowych callbacków.

Projekt skonfigurowany jest w nastęujący sposób:
  - Zegar: 160Mhz
  - Debugger: Piny PA14 (SWCLK), PA13 (SWDIO)
  - USART1: Piny PC4(TX), PC5(RX)
  
Zaimplementowałem też podstawową obsługę błędów.

# Obsługa biblioteki
1. Na początek inicjalizujemy strukturę `UART_CommunicationTypeDef` przez funckcję:
`UART_CommunicationStatusTypeDef UART_Communication_Init(UART_CommunicationTypeDef* pCommunication, UART_HandleTypeDef* huart, uint8_t frame_start, uint32_t queue_size)`
2. Dodajemy callbacki przez: `UART_CommunicationStatusTypeDef UART_Communication_Register_Callback(UART_CommunicationTypeDef* pCommunication, uint8_t ID, void (*pCallback)(uint8_t len, uint8_t* payload))`
3. W głównej pętli należy wywoływać: `UART_CommunicationStatusTypeDef UART_Communication_Update(UART_CommunicationTypeDef* pCommunication)` co spowoduje przetworzenie odebranych sygnałów.
4. Po zakończeniu korzystania z biblioteki należy wyczyścić dane przez `UART_CommunicationStatusTypeDef UART_Communication_Clean(UART_CommunicationTypeDef* pCommunication)`

Funkcje `UART_CommunicationStatusTypeDef UART_Communication_Transmit_Interrupt_Callback(UART_CommunicationTypeDef* pCommunication)` i ` UART_CommunicationStatusTypeDef UART_Communication_Receive_Interrupt_Callback(UART_CommunicationTypeDef* pCommunication)` powiiny być wywoływane w callbackach 
bibliteki HAL, kolejno void `HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)` i `void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)`, tak samo `UART_CommunicationStatusTypeDef UART_Communication__io_put_char(UART_CommunicationTypeDef* pCommunication, int ch` w `int __io_putchar(int ch)`
