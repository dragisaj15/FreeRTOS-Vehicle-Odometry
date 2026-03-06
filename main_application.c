/* KERNEL INCLUDES */
#include "main_application.h"

/* HARDWARE SIMULATOR UTILITY FUNCTIONS */
#include <HW_access.h>



/* SERIAL SIMULATOR CHANNEL TO USE */
#define COM_CH_0 (0)                /* Enkoder levi tocak */
#define COM_CH_1 (1)                /* Enkoder desni tocak */
#define COM_CH_2 (2)                /* Komunikacija sa PC */

/* TASK PRIORITIES */
#define TASK_SERIAL_SEND_PRI        ( tskIDLE_PRIORITY + 2 )
#define TASK_SERIAl_REC_PRI         ( tskIDLE_PRIORITY + 3 )
#define SERVICE_TASK_PRI            ( tskIDLE_PRIORITY + 1 )
#define TASK_SERIAL_BRZINA          ( tskIDLE_PRIORITY + 3)


/* 7-SEG NUMBER DATABASE - ALL HEX DIGITS [ 0 1 2 3 4 5 6 7 8 9 A B C D E F ] */
const char_t hexnum[] = {
    (char_t)0x3FU, (char_t)0x06U, (char_t)0x5BU, (char_t)0x4FU,
    (char_t)0x66U, (char_t)0x6DU, (char_t)0x7DU, (char_t)0x07U,
    (char_t)0x7FU, (char_t)0x6FU, (char_t)0x77U, (char_t)0x7CU,
    (char_t)0x39U, (char_t)0x5EU, (char_t)0x79U, (char_t)0x71U
};

/* GLOBAL OS-HANDLES */
SemaphoreHandle_t LED_INT_BinarySemaphore;   /* binarni semafor za interapt LED BAR-a */
SemaphoreHandle_t RXC_BinarySemaphore0;      /* binarni semafor za serijski port COM_CH_0 */
SemaphoreHandle_t RXC_BinarySemaphore1;      /* binarni semafor za serijski port COM_CH_1 */
SemaphoreHandle_t RXC_BinarySemaphore2;      /* binarni semafor za serijski port COM_CH_2 */
QueueHandle_t Queue_Obim;
QueueHandle_t Queue_Levi;
QueueHandle_t Queue_Desni;
QueueHandle_t Queue_Rst;
QueueHandle_t Queue_Put;
QueueHandle_t Queue_DeltaPut;
QueueHandle_t Queue_TrBrzina;
QueueHandle_t Queue_ProsaoPut;
QueueHandle_t Queue_DeltaPut2;

/* INTERRUPTS */
/* OPC - ON INPUT CHANGE - INTERRUPT HANDLER */
uint32_t OnLED_ChangeInterrupt(void) {
    BaseType_t xHigherPTW = pdFALSE;

    if (xSemaphoreGiveFromISR(LED_INT_BinarySemaphore, &xHigherPTW) != pdPASS) {
        printf("Semafor nije predat\n");
    }
    portYIELD_FROM_ISR(xHigherPTW);
}

/* RXC - RECEPTION COMPLETE - INTERRUPT HANDLER */
uint32_t prvProcessRXCInterrupt(void) {
    BaseType_t xHigherPTW = pdFALSE;

    if (get_RXC_status(0) != 0)
        if (xSemaphoreGiveFromISR(RXC_BinarySemaphore0, &xHigherPTW) != pdTRUE) {
        }
    if (get_RXC_status(1) != 0)
        if (xSemaphoreGiveFromISR(RXC_BinarySemaphore1, &xHigherPTW) != pdTRUE) {
        }
    if (get_RXC_status(2) != 0)
        if (xSemaphoreGiveFromISR(RXC_BinarySemaphore2, &xHigherPTW) != pdTRUE) {
        }

    portYIELD_FROM_ISR(xHigherPTW);
}


/* Pomocna funkcija - konverzija i slanje trenutne brzine preko serijske komunikacije */
void posaljiBrzinu(float_t speed) {
    char_t buffer[20];                           /* dovoljno za float_t kao string */
    sprintf(buffer, "%.2f", speed);              /* konvertuj float_t u string sa 2 decimale, misra 21.6 (zastarela funkcija, ne proverava duzinu bafera,
                                                    ali u ovom zadatku to ne predstavlja problem) */

                                                    /* slanje karakter po karakter */
    for (uint_t i = 0; buffer[i] != '\0'; i++) {
        if (send_serial_character(COM_CH_2, (uchar_t)buffer[i]) != pdFALSE) {
            printf("Greska\n");
        }
    }

    /* posalji newline za oznacavanje kraja poruke */
    if (send_serial_character(COM_CH_2, (uchar_t)'\n') != pdFALSE) {
        printf("Greska\n");
    }
}

/* Pomocna funkcija - konverzija i slanje trenutnog puta preko serijske komunikacije */
void posaljiPut(float_t put) {
    char_t buffer[20];                    /* dovoljno za float_t kao string */
    sprintf(buffer, "%.2f", put);         /* konvertuj float_t u string sa 2 decimale, misra 21.6 (zastarela funkcija, ne proverava duzinu bafera,
                                             ali u ovom zadatku to ne predstavlja problem) */

                                             /* slanje karakter po karakter */
    for (uint_t i = 0; buffer[i] != '\0'; i++) {
        if (send_serial_character(COM_CH_2, (uchar_t)buffer[i]) != pdFALSE) {
            printf("Greska\n");
        }
    }

    /* posalji newline za oznacavanje kraja poruke */
    if (send_serial_character(COM_CH_2, (uchar_t)'\n') != pdFALSE) {
        printf("Greska\n");
    }
}

/* MAIN - SYSTEM STARTUP POINT */
void main_demo(void) {
    /* INITIALIZATION OF THE PERIPHERALS */
    if (init_7seg_comm() != 0) {
        printf("inicijalizacija 7SEG displeja nije uspela\n");
    };
    if (init_LED_comm() != 0) {
        printf("inicijalizacija LEDBAR-a nije uspela\n");
    };

    if (init_serial_uplink(COM_CH_0) != 0) {
        printf("inicijalizacija kanala 0 nije uspela\n");
    };
    if (init_serial_downlink(COM_CH_0) != 0) {
        printf("inicijalizacija kanala 0 nije uspela\n");
    };
    if (init_serial_uplink(COM_CH_1) != 0) {
        printf("inicijalizacija kanala 1 nije uspela\n");
    };
    if (init_serial_downlink(COM_CH_1) != 0) {
        printf("inicijalizacija kanala 1 nije uspela\n");
    };
    if (init_serial_uplink(COM_CH_2) != 0) {
        printf("inicijalizacija kanala 2 nije uspela\n");
    };
    if (init_serial_downlink(COM_CH_2) != 0) {
        printf("inicijalizacija kanala 2 nije uspela\n");
    };

    /* INTERRUPT HANDLERS */
    vPortSetInterruptHandler(portINTERRUPT_SRL_OIC, OnLED_ChangeInterrupt);     /* ON INPUT CHANGE INTERRUPT HANDLER */
    vPortSetInterruptHandler(portINTERRUPT_SRL_RXC, prvProcessRXCInterrupt);    /* SERIAL RECEPTION INTERRUPT HANDLER */

    /* BINARY SEMAPHORES */
    LED_INT_BinarySemaphore = xSemaphoreCreateBinary();	    // CREATE LED INTERRUPT SEMAPHORE
    if (LED_INT_BinarySemaphore == NULL) {
        printf("Greska prilikom kreiranja LED_INT_BinarySemaphore semafora\n");
    }
    RXC_BinarySemaphore0 = xSemaphoreCreateBinary();		// CREATE RXC SEMAPHORE - SERIAL RECEIVE COMM
    if (RXC_BinarySemaphore0 == NULL) {
        printf("Greska prilikom kreiranja RXC_BinarySemaphore semafora\n");
    }
    RXC_BinarySemaphore1 = xSemaphoreCreateBinary();		// CREATE RXC SEMAPHORE - SERIAL RECEIVE COMM
    if (RXC_BinarySemaphore1 == NULL) {
        printf("Greska prilikom kreiranja RXC_BinarySemaphore semafora\n");
    }
    RXC_BinarySemaphore2 = xSemaphoreCreateBinary();		// CREATE RXC SEMAPHORE - SERIAL RECEIVE COMM
    if (RXC_BinarySemaphore2 == NULL) {
        printf("Greska prilikom kreiranja RXC_BinarySemaphore semafora\n");
    }

    /* QUEUES */
    Queue_Obim = xQueueCreate(2, sizeof(uint16_t));
    if (Queue_Obim == NULL) {
        printf("Greska prilikom kreiranja reda Queue_Obim\n");
    }
    Queue_Levi = xQueueCreate(2, sizeof(uint16_t));
    if (Queue_Levi == NULL) {
        printf("Greska prilikom kreiranja reda Queue_Levi\n");
    }
    Queue_Desni = xQueueCreate(2, sizeof(uint16_t));
    if (Queue_Desni == NULL) {
        printf("Greska prilikom kreiranja reda Queue_Desni\n");
    }
    Queue_Rst = xQueueCreate(2, sizeof(uint16_t));
    if (Queue_Rst == NULL) {
        printf("Greska prilikom kreiranja reda Queue_Rst\n");
    }
    Queue_Put = xQueueCreate(2, sizeof(float_t));
    if (Queue_Put == NULL) {
        printf("Greska prilikom kreiranja reda Queue_Put\n");
    }
    Queue_DeltaPut = xQueueCreate(2, sizeof(float_t));
    if (Queue_DeltaPut == NULL) {
        printf("Greska prilikom kreiranja reda Queue_DeltaPut\n");
    }
    Queue_DeltaPut2 = xQueueCreate(2, sizeof(float_t));
    if (Queue_DeltaPut2 == NULL) {
        printf("Greska prilikom kreiranja reda Queue_DeltaPut2\n");
    }
    Queue_TrBrzina = xQueueCreate(2, sizeof(float_t));
    if (Queue_TrBrzina == NULL) {
        printf("Greska prilikom kreiranja reda Queue_TrBrzina\n");
    }
    Queue_ProsaoPut = xQueueCreate(2, sizeof(float_t));
    if (Queue_ProsaoPut == NULL) {
        printf("Greska prilikom kreiranja reda Queue_ProsaoPut\n");
    }

    /* TASKS */
    BaseType_t task;
    task = xTaskCreate(SerialSend_Task0, "STx0", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAL_SEND_PRI, NULL);        /* SERIAL TRANSMITTER TASK 0 */
    if (task != pdPASS) {
        printf("Greska prilikom kreiranja STx0 taska\n");
    }
    task = xTaskCreate(SerialReceive_Task0, "SRx0", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAl_REC_PRI, NULL);      /* SERIAL RECEIVER TASK 0 */
    if (task != pdPASS) {
        printf("Greska prilikom kreiranja SRx0 taska\n");
    }
    task = xTaskCreate(SerialReceive_Task1, "SRx1", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAl_REC_PRI, NULL);      /* SERIAL RECEIVER TASK 1 */
    if (task != pdPASS) {
        printf("Greska prilikom kreiranja SRx1 taska\n");
    }
    task = xTaskCreate(SerialSend_Task2, "STx2", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAL_SEND_PRI, NULL);        /* SERIAL TRANSMITTER TASK 2 */
    if (task != pdPASS) {
        printf("Greska prilikom kreiranja STx2 taska\n");
    }
    task = xTaskCreate(SerialReceive_Task2, "SRx2", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAl_REC_PRI, NULL);      /* SERIAL RECEIVER TASK 2 */
    if (task != pdPASS) {
        printf("Greska prilikom kreiranja SRx2 taska\n");
    }
    task = xTaskCreate(PutEnc_Task, "PutEnc", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAl_REC_PRI, NULL);            /* TASK ZA RACUNANJE PUTA */
    if (task != pdPASS) {
        printf("Greska prilikom kreiranja PutEnc taska\n");
    }
    task = xTaskCreate(MerenjeBrzine_Task, "MerenjeBrzine", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAL_BRZINA, NULL);/* TASK ZA MERENJE BRZINE */
    if (task != pdPASS) {
        printf("Greska prilikom kreiranja MerenjeBrzine taska\n");
    }
    task = xTaskCreate(LCD_Task, "LCD", configMINIMAL_STACK_SIZE, NULL, SERVICE_TASK_PRI, NULL);                      /* CREATE LED BAR TASK */
    if (task != pdPASS) {
        printf("Greska prilikom kreiranja LCD taska\n");
    }

    /* START SCHEDULER */
    vTaskStartScheduler();
    for (;;) {}
}


/* Task koji svakih 200ms salje triger ('X') na kanale 0 i 1 kako bi oni slali inkremente automatski */
void SerialSend_Task0(const void* pvParameters) {
    (void)pvParameters;  /* Eksplicitno oznacava da se parametar ne koristi */

    for (;;) {  /* MISRA preporucuje for(;;) umesto while(1) */
        vTaskDelay(pdMS_TO_TICKS(200));
        if (send_serial_character(COM_CH_0, (uchar_t)'X') != pdFALSE) {
            printf("Greska\n");
        }
        if (send_serial_character(COM_CH_1, (uchar_t)'X') != pdFALSE) {
            printf("Greska\n");
        }
    }
}


/* Task koji ocitava poruke poslate sa kanala 0 (cita inkremente sa levog tocka) */
void SerialReceive_Task0(const void* pvParameters) {
    (void)pvParameters;  /* Eksplicitno oznacava da se parametar ne koristi */

    char_t receivedChar = '\0';   /* Inicijalizacija */
    char_t buffer[6] = { '\0' };  /* Koristi character literal */
    uint_t index = 0U;
    uint_t wheel_increment = 0U;
    bool_t reading = (bool_t)0;   /* Koristi bool_t umesto uint_t */
    BaseType_t QS;

    for (;;) {
        if (xSemaphoreTake(RXC_BinarySemaphore0, portMAX_DELAY) != pdTRUE) {
        }
        else {
            /* Dodato else za Rule 15.7 */
        }

        if (get_serial_character(COM_CH_0, (uint8_t*)&receivedChar) != pdFALSE) {
        }
        else {
            /* Dodato else za Rule 15.7 */
        }

        if (receivedChar == (char_t)'L') {
            index = 0U;
            reading = (bool_t)1;
        }
        /* Ispravka za Rule 12.1 - dodane zagrade za precedenciju */
        else if ((receivedChar == (char_t)'<') && reading) {
            buffer[index] = '\0';  /* Null terminator */

            /* MISRA compliant string to int konverzija umesto atoi */
            wheel_increment = 0U;
            uint_t i = 0U;
            while ((i < index) && (buffer[i] >= (char_t)'0') && (buffer[i] <= (char_t)'9')) {
                uchar_t uchar_value = (uchar_t)buffer[i];
                uint_t char_value = (uint_t)uchar_value;
                uint_t zero_value = (uint_t)'0';
                uint_t digit_value = char_value - zero_value;
                wheel_increment = (wheel_increment * 10U) + digit_value;
                i++;
            }

            QS = xQueueSend(Queue_Levi, &wheel_increment, 0);
            if (QS != pdPASS) {
            }
            else {
                /* Dodato else za Rule 15.7 */
            }
            reading = (bool_t)0;
        }
        /* Ispravka za Rule 12.1 - dodane zagrade za sve operatore */
        else if (reading &&
            (receivedChar >= (char_t)'0') &&
            (receivedChar <= (char_t)'9') &&
            (index < (sizeof(buffer) - 1U))) {

            /* Ispravka za Rule 13.3 - odvojeni increment od assignment */
            buffer[index] = receivedChar;
            index++;
        }
        else {
            /* Dodato else na kraju if-else za Rule 15.7 */
        }
    }
}


/* Task koji ocitava poruke poslate sa kanala 1 (cita inkremente sa desnog tocka) */
void SerialReceive_Task1(const void* pvParameters) {
    (void)pvParameters;  /* Eksplicitno oznacava da se parametar ne koristi */

    char_t receivedChar = '\0';   /* Inicijalizacija */
    char_t buffer[6] = { '\0' };  /* Koristi character literal */
    uint_t index = 0U;
    uint_t wheel_increment = 0U;
    bool_t reading = (bool_t)0;   /* Koristi bool_t umesto uint_t */
    BaseType_t QR;

    for (;;) {
        if (xSemaphoreTake(RXC_BinarySemaphore1, portMAX_DELAY) != pdTRUE) {
        }
        else {
            /* Dodato else za Rule 15.7 */
        }

        /* Ispravno kastovanje pointera */
        if (get_serial_character(COM_CH_1, (uint8_t*)&receivedChar) != pdFALSE) {
        }
        else {
            /* Dodato else za Rule 15.7 */
        }

        if (receivedChar == (char_t)'R') {
            index = 0U;
            reading = (bool_t)1;
        }
        /* Ispravka za Rule 12.1 - dodane zagrade za precedenciju */
        else if ((receivedChar == (char_t)'<') && reading) {
            buffer[index] = '\0';  /* Null terminator */

            /* MISRA compliant string to int konverzija umesto atoi */
            wheel_increment = 0U;
            uint_t i = 0U;
            while ((i < index) && (buffer[i] >= (char_t)'0') && (buffer[i] <= (char_t)'9')) {
                uchar_t uchar_value = (uchar_t)buffer[i];
                uint_t char_value = (uint_t)uchar_value;
                uint_t zero_value = (uint_t)'0';
                uint_t digit_value = char_value - zero_value;
                wheel_increment = (wheel_increment * 10U) + digit_value;
                i++;
            }

            QR = xQueueSend(Queue_Desni, &wheel_increment, 0);
            if (QR != pdPASS) {
            }
            else {
                /* Dodato else za Rule 15.7 */
            }
            reading = (bool_t)0;
        }
        /* Ispravka za Rule 12.1 - dodane zagrade za sve operatore */
        else if (reading &&
            (receivedChar >= (char_t)'0') &&
            (receivedChar <= (char_t)'9') &&
            (index < (sizeof(buffer) - 1U))) {

            buffer[index] = receivedChar;
            index++;
        }
        else {
            /* Dodato else na kraju if-else za Rule 15.7 */
        }
    }
}

/* Task za komunikaciju sa PC - omogucava podesavanje obima tocka i reset */
void SerialReceive_Task2(const void* pvParameters) {
    (void)pvParameters;  /* Eksplicitno oznacava da se parametar ne koristi */

    uint8_t cc = 0U;                         /* Primljeni karakter */
    char_t obim[6] = { '\0' };               /* String za broj (maksimalno 5 cifara + NULL) */
    uint16_t reset = 1U;                     /* Flag za reset */
    uint16_t obim_tocka = 0U;                /* Vrednost obima tocka */
    char_t r_buffer2[BUFFER_SIZE];           /* Bafer za primljene podatke */
    uint8_t r_point2 = 0U;                   /* Indeks u baferu */
    BaseType_t QR2;

    for (;;) {
        /* Cekamo dolazak podatka preko serijske komunikacije */
        if (xSemaphoreTake(RXC_BinarySemaphore2, portMAX_DELAY) != pdTRUE) {
        }
        else {
            /* else za Rule 15.7 */
        }

        if (get_serial_character(COM_CH_2, &cc) != pdFALSE) { /* Citanje karaktera */
        }
        else {
            /* else za Rule 15.7 */
        }

        if (cc == (uint8_t)13U) {                 /* Ako je primljen CR (kraj poruke) */
            r_buffer2[r_point2] = '\0';           /* Zavrsavanje stringa */

            if ((r_point2 == (uint8_t)5U) &&
                (r_buffer2[0] == 'R') &&
                (r_buffer2[1] == 'E') &&
                (r_buffer2[2] == 'S') &&
                (r_buffer2[3] == 'E') &&
                (r_buffer2[4] == 'T')) {

                /* Ako je primljen RESET, resetuj vrednosti predjenog puta */
                reset = 0U;
                printf("Predjeni put resetovan!\n");

                QR2 = xQueueSend(Queue_Rst, &reset, 0);
                if (QR2 != pdPASS) {
                }
                else {
                    /* else za Rule 15.7 */
                }

                reset = 1U;              /* Vrati reset (flag) na jedinicu */
                QR2 = xQueueSend(Queue_Rst, &reset, 0);
                if (QR2 != pdPASS) {
                }
                else {
                    /* else za Rule 15.7 */
                }
            }
            else if ((r_point2 >= (uint8_t)4U) &&
                (r_buffer2[0] == 'O') &&
                (r_buffer2[1] == 'M')) {

                uint8_t cnt = 0U;

                for (uint8_t i = 2U; i < r_point2; i++) {
                    if ((r_buffer2[i] >= '0') && (r_buffer2[i] <= '9')) {   /* Provera da li je cifra */
                        obim[cnt] = r_buffer2[i];                           /* Dodaj cifru u string */
                        cnt++;
                    }
                    else {
                        /* else za Rule 15.7 */
                    }
                }
                obim[cnt] = '\0';       /* Dodaj zavrsetak stringa */

                /* MISRA compliant konverzija stringa u broj */
                obim_tocka = 0U;
                {
                    uint_t j = 0U;
                    while ((j < (uint_t)cnt) &&
                        (obim[j] >= (char_t)'0') &&
                        (obim[j] <= (char_t)'9')) {

                        uchar_t uchar_value = (uchar_t)obim[j];
                        uint_t char_value = (uint_t)uchar_value;
                        uint_t zero_value = (uint_t)'0';
                        uint_t digit_value = char_value - zero_value;

                        obim_tocka = (uint16_t)((obim_tocka * 10U) + (uint16_t)digit_value);

                        j++;
                    }
                }

                QR2 = xQueueSend(Queue_Obim, &obim_tocka, 0);
                if (QR2 != pdPASS) {
                }
                else {
                    /* else za Rule 15.7 */
                }

                printf("Obim tocka: %u cm\n", obim_tocka);
            }
            else {
                /* else za Rule 15.7 */
            }

            r_point2 = 0U;               /* Resetujemo bafer posle obrade poruke */
        }
        else {
            if (r_point2 < (uint8_t)(BUFFER_SIZE - 1)) {
                r_buffer2[r_point2] = (char_t)cc;
                r_point2++;
            }
            else {
                /* else za Rule 15.7 */
            }
        }
    }
}


/* Task za izracunavanje predjenog puta, LAP puta, puta po inkrementu */
void PutEnc_Task(const void* pvParameters) {
    (void)pvParameters;  /* Eksplicitno oznacava da se parametar ne koristi */

    uint16_t obimTocka = 0U;
    uint16_t inkLevi = 0U;
    uint16_t inkDesni = 0U;
    float_t putEnkoderaLevi = 0.0f;
    float_t putEnkoderaDesni = 0.0f;
    float_t put_po_inkrementu = 0.0f;
    float_t predjeniPut = 0.0f;
    float_t predjeniPutStop = 0.0f;
    float_t predjeniPutStart = 0.0f;
    uint16_t reset = 1U;
    uint8_t d = 0U;
    uint8_t prevLedState = 0U;
    float_t deltaPutLevi = 0.0f;
    float_t deltaPutDesni = 0.0f;
    float_t deltaPredjeniPut = 0.0f;
    BaseType_t QS2, QR;


    for (;;) {
        /* Citanje podataka iz redova */
        QS2 = xQueueReceive(Queue_Obim, &obimTocka, 0U);
        if (QS2 != pdPASS) {
        }

        QS2 = xQueueReceive(Queue_Levi, &inkLevi, portMAX_DELAY);
        if (QS2 != pdPASS) {
        }

        QS2 = xQueueReceive(Queue_Desni, &inkDesni, portMAX_DELAY);
        if (QS2 != pdPASS) {
        }

        QS2 = xQueueReceive(Queue_Rst, &reset, 0U);
        if (QS2 != pdPASS) {
        }

        /* Izracunaj put po inkrementu */
        put_po_inkrementu = (float_t)obimTocka / 36000.0f;   /* cm po inkrementu */

        deltaPutLevi = put_po_inkrementu * (float_t)inkLevi;
        deltaPutDesni = put_po_inkrementu * (float_t)inkDesni;

        /* Predjeni put u poslednjih 0.2s */
        deltaPredjeniPut = (float_t)reset * (deltaPutLevi + deltaPutDesni) / 2.0f;
        QR = xQueueSend(Queue_DeltaPut, &deltaPredjeniPut, 0U);
        if (QR != pdPASS) {
        }

        /* Azuriranje kumulativnog puta */
        putEnkoderaLevi = (float_t)reset * (putEnkoderaLevi + deltaPutLevi);
        putEnkoderaDesni = (float_t)reset * (putEnkoderaDesni + deltaPutDesni);

        /* Ukupan predjeni put vozila od pocetka */
        predjeniPut = (float_t)reset * (putEnkoderaLevi + putEnkoderaDesni) / 2.0f;
        printf("UKUPNI PREDJENI PUT: %f cm\n", predjeniPut);
        QR = xQueueSend(Queue_Put, &predjeniPut, 0U);
        if (QR != pdPASS) {
        }

        /* Merenje LAP-a odnosno predjenog puta izmedju starta(upaljena prva dioda u 1. koloni) i stopa(upaljena druga dioda u 1. koloni) */
        if (xSemaphoreTake(LED_INT_BinarySemaphore, 0U) == pdPASS) {
            if (get_LED_BAR(0U, &d) != 0) {
                printf("Greska prilikom preuzimanja vrednosti sa LEDBAR-a\n");
            }

            /* LED za START je bit 7 (0x80) u prvoj koloni (start merenja LAP-a) */
            if (((prevLedState & 0x80U) == 0U) && ((d & 0x80U) != 0U)) {  /* LED 7 je pre bila ugasena, sada upaljena -> START dogadjaj */
                predjeniPutStart = predjeniPut;
                if (set_LED_BAR(1U, 0x80U) != 0) { /* Vizuelno potvrdi start */
                    printf("Greska prilikom setovanja LED dioda\n");
                }
            }

            /* LED za STOP je bit 6 (0x40) u prvoj koloni (prestanak merenja LAP-a) */
            if (((prevLedState & 0x40U) == 0U) && ((d & 0x40U) != 0U)) { /* LED 6 je pre bila ugasena, sada upaljena -> STOP dogadjaj */
                predjeniPutStop = predjeniPut - predjeniPutStart;
                QR = xQueueSend(Queue_ProsaoPut, &predjeniPutStop, 0U);
                if (QR != pdPASS) {
                }
                if (set_LED_BAR(1U, 0x00U) != 0) { /* Vizuelno potvrdi stop */
                    printf("Greska prilikom setovanja LED dioda\n");
                }
            }

            /* Azuriraj prethodno stanje */
            prevLedState = d;
        }
    }
}


/* Task koji meri brzinu kretanja na osnovu dobijenog puta */
void MerenjeBrzine_Task(const void* pvParameters) {
    (void)pvParameters;  /* Eksplicitno oznacava da se parametar ne koristi */

    float_t deltaPredjeniPut = 0.0f;
    float_t brzina = 0.0f;
    BaseType_t QS, QR;


    for (;;) {
        QS = xQueueReceive(Queue_DeltaPut, &deltaPredjeniPut, 0);
        if (QS != pdPASS) {
        }
        /* Brzina u cm/s pa konverzija u km/h */
        brzina = (deltaPredjeniPut / 0.2f) * 0.036f;    /* (cm/s -> km/h) */
        QR = xQueueSend(Queue_TrBrzina, &brzina, 0);
        if (QR != pdPASS) {
        }
        printf("BRZINA JE: %f  KM/H\n", brzina);
        posaljiBrzinu(brzina);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task koji salje trenutni put PC-ju */
void SerialSend_Task2(const void* pvParameters) {
    (void)pvParameters;  /* Eksplicitno oznacava da se parametar ne koristi */

    float_t Put = 0.0f;
    BaseType_t QS, QR;

    for (;;) {
        QS = xQueueReceive(Queue_DeltaPut, &Put, 0);
        if (QS != pdPASS) {
        }
        posaljiPut(Put);
        QR = xQueueSend(Queue_DeltaPut2, &Put, 0);
        if (QR != pdPASS) {
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task za prikazivanje trenutne brzine i trenutnog puta ili ukupnog puta i LAP puta na 7-seg displeju */
void LCD_Task(const void* pvParameters) {
    (void)pvParameters;  /* Eksplicitno oznacava da se parametar ne koristi */

    float_t deltaPredjeniPut = 0.0f;
    float_t brzina = 0.0f;
    float_t predjeniPut = 0.0f;
    float_t prosaoPut = 0.0f;
    uint16_t brzinaInt = 0;
    uint16_t deltaPredjeniPutInt = 0;
    uint16_t predjeniPutInt = 0;
    uint16_t prosaoPutInt = 0;
    uint8_t d;
    BaseType_t QS;

    for (;;) {
        /* Ucitaj podatke iz redova */
        QS = xQueueReceive(Queue_DeltaPut2, &deltaPredjeniPut, 0);
        if (QS != pdPASS) {
        }
        QS = xQueueReceive(Queue_TrBrzina, &brzina, 0);
        if (QS != pdPASS) {
        }
        QS = xQueueReceive(Queue_Put, &predjeniPut, 0);
        if (QS != pdPASS) {
        }
        QS = xQueueReceive(Queue_ProsaoPut, &prosaoPut, 0);
        if (QS != pdPASS) {
        }

        /* Proveri stanje LED bara */
        if (get_LED_BAR(2, &d) != 0) {
            printf("Greska prilikom preuzimanja vrednosti sa LEDBAR-a\n");
        }

        if (d == (uint8_t)0x80U) {
            /* Prikaz brzine i delta puta */

            /* Konverzija vrednosti (MISRA-compliant zaokruzivanje) */
            float_t temp_brzina = brzina + 0.5f;
            brzinaInt = (uint16_t)temp_brzina;
            float_t temp_delta_put = deltaPredjeniPut + 0.5f;
            deltaPredjeniPutInt = (uint16_t)temp_delta_put;

            /* Ispis trenutne brzine */
            uint8_t cifra_brzina_1 = (uint8_t)((brzinaInt / 1000U) % 10U);
            uint8_t cifra_brzina_2 = (uint8_t)((brzinaInt / 100U) % 10U);
            uint8_t cifra_brzina_3 = (uint8_t)((brzinaInt / 10U) % 10U);
            uint8_t cifra_brzina_4 = (uint8_t)(brzinaInt % 10U);

            if (select_7seg_digit(0) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_brzina_1]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(1) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_brzina_2]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(2) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_brzina_3]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(3) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_brzina_4]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }

            /* Ispis trenutnog puta */
            uint8_t cifra_put_1 = (uint8_t)((deltaPredjeniPutInt / 1000U) % 10U);
            uint8_t cifra_put_2 = (uint8_t)((deltaPredjeniPutInt / 100U) % 10U);
            uint8_t cifra_put_3 = (uint8_t)((deltaPredjeniPutInt / 10U) % 10U);
            uint8_t cifra_put_4 = (uint8_t)(deltaPredjeniPutInt % 10U);

            if (select_7seg_digit(5) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_put_1]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(6) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_put_2]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(7) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_put_3]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(8) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_put_4]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
        }
        else if (d == (uint8_t)0x01U) {
            /* Prikaz ukupnog i predjenog puta */

            /* Konverzija vrednosti (MISRA-compliant zaokruzivanje) */
            float_t temp_predjeni_put = predjeniPut + 0.5f;
            predjeniPutInt = (uint16_t)temp_predjeni_put;
            float_t temp_prosao_put = prosaoPut + 0.5f;
            prosaoPutInt = (uint16_t)temp_prosao_put;

            /* Ispis ukupnog puta */
            uint8_t cifra_predjeni_1 = (uint8_t)((predjeniPutInt / 1000U) % 10U);
            uint8_t cifra_predjeni_2 = (uint8_t)((predjeniPutInt / 100U) % 10U);
            uint8_t cifra_predjeni_3 = (uint8_t)((predjeniPutInt / 10U) % 10U);
            uint8_t cifra_predjeni_4 = (uint8_t)(predjeniPutInt % 10U);

            if (select_7seg_digit(0) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_predjeni_1]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(1) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_predjeni_2]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(2) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_predjeni_3]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(3) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_predjeni_4]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }

            /* Ispis LAP puta */
            uint8_t cifra_prosao_1 = (uint8_t)((prosaoPutInt / 1000U) % 10U);
            uint8_t cifra_prosao_2 = (uint8_t)((prosaoPutInt / 100U) % 10U);
            uint8_t cifra_prosao_3 = (uint8_t)((prosaoPutInt / 10U) % 10U);
            uint8_t cifra_prosao_4 = (uint8_t)(prosaoPutInt % 10U);

            if (select_7seg_digit(5) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_prosao_1]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(6) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_prosao_2]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(7) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_prosao_3]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
            if (select_7seg_digit(8) != pdFALSE) {
                printf("Greska prilikom odabira displeja\n");
            }
            if (set_7seg_digit((uint8_t)hexnum[cifra_prosao_4]) != pdFALSE) {
                printf("Greska prilikom setovanja displeja\n");
            }
        }
        else {
            /* Nije pritisnuta nijedna LED -> ocisti prikaz */
            uint8_t i;
            for (i = 0U; i < 9U; i++) {
                if (select_7seg_digit(i) != pdFALSE) {
                    printf("Greska prilikom odabira displeja\n");
                }
                if (set_7seg_digit(0x00U) != pdFALSE) {
                    printf("Greska prilikom setovanja displeja\n");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(150)); /* Osvezavanje svakih 150ms */
    }
}