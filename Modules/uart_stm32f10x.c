/*************************************************************************
 *    File name   : uart_stm32f10x.c for STM
 *    Description : UARTs module
 *    Author      : Sergey Koryakin
**************************************************************************/
#include "uart_stm32f10x.h"
#include "timers.h"
#include "string.h"

#define UART_BYTESTOP   3       // Байтовый промежуток между посылками
#define UARTS           3       // количество портов

typedef struct{
  uint8_t* rxbuffer;
  uint16_t bfsize;
  uint16_t received;
  uint16_t timetowait;
  uint16_t twait;
  uint8_t state;
  uint8_t receiving;
}uart_t;

#pragma data_alignment=4
static uart_t uart[UARTS];

/*************************************************************************
 * Функция обработки прерывания по UART1
*************************************************************************/
#pragma call_graph_root="interrupt"
void USART1_IRQHandler(void)
{
  if ((USART1->SR & USART_FLAG_RXNE) != (uint16_t)RESET){
    USART1->SR = (uint16_t)~USART_FLAG_RXNE;
    uart_addbyte(0, USART1->DR);
  } 
}

/*************************************************************************
 * Функция обработки прерывания по UART2
*************************************************************************/
#pragma call_graph_root="interrupt"
void USART2_IRQHandler(void)
{
  if ((USART2->SR & USART_FLAG_RXNE) != (uint16_t)RESET){
    USART2->SR = (uint16_t)~USART_FLAG_RXNE;
    uart_addbyte(1, USART2->DR);
  }
}

/*************************************************************************
 * Функция обработки прерывания по UART3
*************************************************************************/
#pragma call_graph_root="interrupt"
void USART3_IRQHandler(void)
{
  if ((USART3->SR & USART_FLAG_RXNE) != (uint16_t)RESET){
    USART3->SR = (uint16_t)~USART_FLAG_RXNE;
    uart_addbyte(2, USART3->DR);
  }
}

/*************************************************************************
 * Инициализация Uart
 * Uart - номер порта
 * Baudrate - скорость
 * Stopbits - количество стоп-битов 0 - один, 1 - два
 * Parity - чётность 0 - нет, 1 - чёт, 2 - нечёт 
*************************************************************************/
bool uart_init (uint8_t Uart, uint32_t Baudrate, uint8_t Stopbits, uint8_t* rxbuffer, uint16_t bfsize)
{
  if (Uart >= UARTS) return false;   
    
  GPIO_InitTypeDef GPIO_InitStructure;
  USART_InitTypeDef USART_InitStruct;

  GPIO_StructInit(&GPIO_InitStructure);
  USART_StructInit(&USART_InitStruct);

  USART_InitStruct.USART_BaudRate = Baudrate;
  if (Stopbits == 0) USART_InitStruct.USART_StopBits = USART_StopBits_1;
  else USART_InitStruct.USART_StopBits = USART_StopBits_2;
  USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  /*switch (Parity)
   {
   case 0: USART_InitStruct.USART_Parity = USART_Parity_No; break;
   case 1: USART_InitStruct.USART_Parity = USART_Parity_Even; break;
   case 2: USART_InitStruct.USART_Parity = USART_Parity_Odd; break;
   }*/

  // Alternate function clock
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

  uart[Uart].rxbuffer = rxbuffer;
  uart[Uart].bfsize = bfsize;
  uart[Uart].timetowait = 2;
  uart[Uart].received = 0;
  uart[Uart].twait = 0;
  uart[Uart].state = 0;
  uart[Uart].receiving = 0;
  

  switch (Uart)
   {
   case 0:
     USART_DeInit(USART1);   
     RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);
     // Rx pin
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_Init(GPIOA, &GPIO_InitStructure);
     // Tx pin
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_Init(GPIOA, &GPIO_InitStructure);
     
     USART_Init(USART1, &USART_InitStruct);
     USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
     NVIC_USART_Config(USART1_IRQn);
     USART_Cmd(USART1, ENABLE);
     break;

   case 1:
     USART_DeInit(USART2);
     RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2 | RCC_APB2Periph_GPIOA, ENABLE);
     // Rx pin
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_Init(GPIOA, &GPIO_InitStructure);
     // Tx pin
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_Init(GPIOA, &GPIO_InitStructure);
     
     USART_Init(USART2, &USART_InitStruct);
     USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
     NVIC_USART_Config(USART2_IRQn);
     USART_Cmd(USART2, ENABLE);
     break;

   case 2:
     USART_DeInit(USART3);
     RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3 | RCC_APB2Periph_GPIOB, ENABLE);
     // Rx pin
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_Init(GPIOB, &GPIO_InitStructure);
     // Tx pin
     GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
     GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
     GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
     GPIO_Init(GPIOB, &GPIO_InitStructure);
     
     USART_Init(USART3, &USART_InitStruct);
     USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
     NVIC_USART_Config(USART3_IRQn);
     USART_Cmd(USART3, ENABLE);
     break;
   }
  return true;
}

/************************************************************************
* Функция отправки байта по UART
* uart - номер порта
* ch - символ
*************************************************************************/
static void uart_sendbyte (Int8U uart, Int8U ch)
{
  switch (uart)
   {
   case 0: while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
           USART_SendData(USART1, ch);
           break;
   case 1: while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
           USART_SendData(USART2, ch);
           break;
   case 2: while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
           USART_SendData(USART3, ch);
           break;
   }
}

/************************************************************************
* Функция отправки строки по UART
* uart - номер порта
* string - строка
*************************************************************************/
void uart_sendstr (Int8U uart, char* string)
{
  if (uart_checkenable(uart) == 0) return; 
  if (string == NULL) return;

  while ((*string) != 0)
   {
   uart_sendbyte (uart, *string);
   string++;
   }
}

/************************************************************************
* Функция отправки буфера определенной длины по UART
* uart - номер порта
* string - указатель на буфер
* size - размер
*************************************************************************/
void uart_send (Int8U uart, pInt8U buf, Int16U size)
{
  if (uart_checkenable(uart) == 0) return;
  if (buf == NULL) return;

  for (Int16U i = 0; i < size; i++) 
    uart_sendbyte (uart, buf[i]);
}

/************************************************************************
* Функция очистки буфера приема 
* num - номер буфера
*************************************************************************/
void uart_clear (Int8U num)
{
  if (num >= UARTS) return;
  if (uart[num].rxbuffer == NULL) return;

  for (uint16_t i=0; i<uart[num].received; i++) 
    uart[num].rxbuffer[i] = 0;
  uart[num].received = 0;
  uart[num].state = 0;
  uart[num].receiving = 0;
}

// Возвращает кол-во принятых байт в буфере
Int16U uart_received (Int8U num)
{
  if (num >= UARTS) return 0;  
  
  return uart[num].received;
}

// Возвращает указатель на буфер
pInt8U uart_getptr (Int8U num)
{
  if (num >= UARTS) return NULL;  
  
  return uart[num].rxbuffer;
}

// Возвращает true, если буфер готов
Int8U uart_getstate (Int8U num)
{
  if (num >= UARTS) return 0;  
  if (uart[num].twait) return 0;
  
  return uart[num].state;
}

/************************************************************************
* Обработчик подсчета времени.
*************************************************************************/
void uart_timer1ms (void)
{
  for (uint8_t i=0; i<UARTS; i++) // Цикл проверки всех задержек
   {
   if (uart[i].twait) // Задержка перед проверкой буфера приема
     if (--uart[i].twait == 0){
       uart[i].state = 1;   // Буфер готов
       uart[i].receiving = 0;
     }
   }
}

// Ожидание ответа по порту UART
// timeout - время, через которое выходим, в мс
// uart - порт по которому ждем
// pattern - то что ищем
// Поиск ограничен снимком количества принятых байтов, завершающий NUL не требуется.
static bool uart_matchresponse(Int8U num, uint16_t first, const char *pattern)
{
  uint16_t count = uart[num].received;
  uint32_t n = strlen(pattern);
  if (count > uart[num].bfsize) count = uart[num].bfsize;
  if (first > count || n > (uint16_t)(count - first)) return false;
  if (n == 0) return true;
  for (uint16_t i = first; i <= (uint16_t)(count - n); i++){
    if (memcmp(uart[num].rxbuffer + i, pattern, n) == 0) return true;
  }
  return false;
}

bool uart_waitresp(Int8U num, Int16U t_ms, char* pattern)
{
  if (num >= UARTS) return false;

  uart[num].twait = 0;
  uart[num].state = 0;

  uint16_t startrec = uart[num].received;
  uint16_t received = uart[num].received;  
  uint32_t start = timers_getMillis();
 
  while ((timers_getMillis() - start) < t_ms){
    // принят байт
    if (received != uart[num].received){
      received = uart[num].received;
      start = timers_getMillis();
    }
    // приняли
    if (uart[num].state){  
      if (pattern == NULL){
        return true;
      } else{
        if (uart_matchresponse(num, startrec, pattern))
          return true;
        else
          return false;
      }
    }
  }
  return false;
}

// Включить порт
void uart_on (Int8U port)
{
  switch (port)
   {
   case 0: 
      USART_Cmd(USART1, ENABLE);
      break;
   case 1: 
      USART_Cmd(USART2, ENABLE);
      break;
   case 2:
      USART_Cmd(USART3, ENABLE);
      break;
   }
}

// Выключить порт
void uart_off (Int8U port)
{
  switch (port)
   {
   case 0: 
      USART_Cmd(USART1, DISABLE);
      break;
   case 1: 
      USART_Cmd(USART2, DISABLE);
      break;
   case 2:
      USART_Cmd(USART3, DISABLE);
      break; 
   }
}

// добавляет байт в буфер
inline bool uart_addbyte (Int8U num, Int8U data)
{
  if (uart[num].rxbuffer == NULL) return false;

  if (uart[num].received < uart[num].bfsize) // Считываем байт и записываем по указателю
   {
   uart[num].rxbuffer[uart[num].received] = data;  
   uart[num].received++;
   uart[num].twait = uart[num].timetowait;
   uart[num].receiving = 1;
   return true;
   }

  return false;
}

// Узнать размер буфера
Int16U uart_getbuffersize(Int8U num)
{
  if (num >= UARTS) return 0;
  if (uart[num].rxbuffer == NULL) return 0;
  
  return uart[num].bfsize;
}

// установить время ожидания буфера приемника
void uart_setRecTimeout (unsigned char u, unsigned int t)
{
  if (u >= UARTS) return;
  if (t == 0) t = 1;
  
  uart[u].timetowait = t;
}

// включаем обработку прерывания по приёму байт
void NVIC_USART_Config (Int8U us)
{
  NVIC_InitTypeDef NVIC_InitStructure;

  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  NVIC_InitStructure.NVIC_IRQChannel = us;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 7;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
}

// проверить включен порт или нет
Int8U uart_checkenable (Int8U p)
{
  switch (p)
   {
   case 0: if ((USART1->CR1&USART_CR1_UE) == USART_CR1_UE) return 1;
     break;
   case 1: if ((USART2->CR1&USART_CR1_UE) == USART_CR1_UE) return 1;
     break;
   case 2: if ((USART3->CR1&USART_CR1_UE) == USART_CR1_UE) return 1;
     break;  
   }
  return 0;
}

/************************************************************************
* Указать буфер приема
* u - номер порта
*************************************************************************/
void uart_setrxbuffer (uint8_t u, uint8_t* rxbuffer, uint16_t bfsize)
{
  if (u >= UARTS) return;

  uart[u].rxbuffer = rxbuffer;
  uart[u].bfsize = bfsize;
  uart[u].received = 0;
  uart[u].state = 0;
}

// Отправляем строку и ждем ответ
// timeout - время, через которое выходим, в мс
// uart - порт по которому ждем
// pattern - то что ищем
bool uart_sendStrAndWaitResp(Int8U num, char *str, char *rec, Int16U recsize, Int16U t_ms, char* pattern)
{
  if (num >= UARTS) return false;

  uint16_t received = 0;
  uint32_t start;
  
  // чистим
  uart_clear(num);

  // отправляем
  uart_sendstr(num, str);

  if (rec != NULL)
    *rec = 0;  
  
  if (t_ms == 0)
    return true;
  
  start = timers_getMillis();

  while ((timers_getMillis() - start) < t_ms){
    // принят байт
    if (received != uart[num].received){
      received = uart[num].received;
      start = timers_getMillis();
    }
    // приняли
    if (uart[num].state){
      // копируем в приемный буфер
      if (rec != NULL){
        // Ответ бывает длиннее приёмника: в кадр попадает URC с данными от сервера
        Int16U len = uart[num].received;

        if (recsize == 0)
          len = 0;
        else if (len > (Int16U)(recsize - 1))
          len = (Int16U)(recsize - 1);

        memcpy(rec, &uart[num].rxbuffer[0], len);
        rec[len] = 0;
      }
      
      if (pattern == NULL){
        return true;
      } else{
        if (uart_matchresponse(num, 0, pattern))
          return true;
        else
          return false;
      }
    }
    wdt_reload();
    delay_ms(1);
  }
  return false;
}

// Проверить что идет прием или нет
bool uart_isReceiving(uint8_t num)
{
  if (num >= UARTS) return false;
  if (uart[num].receiving != 0)
    return true;
  return false;
}