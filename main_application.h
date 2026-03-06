#ifndef MAIN_APPLICATION_H
#define MAIN_APPLICATION_H

/*===========================================================================*/
/* STANDARD INCLUDES                                                         */
/*===========================================================================*/
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/*===========================================================================*/
/* KERNEL INCLUDES                                                           */
/*===========================================================================*/
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <timers.h>
#include <extint.h>

/* Definisanje tipova podataka */
typedef unsigned int uint_t;
typedef float float_t;
typedef char char_t;
typedef unsigned char uchar_t;


/* BUFFER DEFINITIONS */
#define BUFFER_SIZE                 12                  /* Maksimalna velicina bafera */

/* Definisanje bool tipa zbog MISRE */
typedef _Bool bool_t;

/* main */
extern void main_demo(void);

/* Task functions */
extern void LCD_Task(const void* pvParameters);
extern void SerialSend_Task0(const void* pvParameters);
extern void SerialReceive_Task0(const void* pvParameters);
extern void SerialReceive_Task1(const void* pvParameters);
extern void SerialSend_Task2(const void* pvParameters);
extern void SerialReceive_Task2(const void* pvParameters);
extern void PutEnc_Task(const void* pvParameters);
extern void MerenjeBrzine_Task(const void* pvParameters);

/* Utility functions */
extern void posaljiBrzinu(float_t speed);
extern void posaljiPut(float_t put);

/* Interrupt handlers */
extern uint32_t prvProcessRXCInterrupt(void);
extern uint32_t OnLED_ChangeInterrupt(void);


/* Semaphore handles - extern declarations */
extern SemaphoreHandle_t LED_INT_BinarySemaphore;      /* binarni semafor za interapt LED BAR-a */
extern SemaphoreHandle_t RXC_BinarySemaphore0;         /* binarni semafor za serijski port COM_CH_0 */
extern SemaphoreHandle_t RXC_BinarySemaphore1;         /* binarni semafor za serijski port COM_CH_1 */
extern SemaphoreHandle_t RXC_BinarySemaphore2;         /* binarni semafor za serijski port COM_CH_2 */

/* Queue handles - extern declarations */
extern QueueHandle_t Queue_Obim;
extern QueueHandle_t Queue_Levi;
extern QueueHandle_t Queue_Desni;
extern QueueHandle_t Queue_Rst;
extern QueueHandle_t Queue_Put;
extern QueueHandle_t Queue_DeltaPut;
extern QueueHandle_t Queue_TrBrzina;
extern QueueHandle_t Queue_ProsaoPut;
extern QueueHandle_t Queue_DeltaPut2;


/* 7-SEG NUMBER DATABASE - extern declaration */
extern const char_t hexnum[16];                            /* HEX DIGITS [0 1 2 3 4 5 6 7 8 9 A B C D E F] */

#endif /* MAIN_APPLICATION_H */