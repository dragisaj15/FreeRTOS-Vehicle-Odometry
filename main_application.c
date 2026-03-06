/* KERNEL INCLUDES */
#include "main_application.h"

/* HARDWARE SIMULATOR UTILITY FUNCTIONS */
#include <HW_access.h>

/* SERIAL SIMULATOR CHANNEL TO USE */
#define COM_CH_0 (0)                /* Left wheel encoder */
#define COM_CH_1 (1)                /* Right wheel encoder */
#define COM_CH_2 (2)                /* PC Communication */

/* TASK PRIORITIES */
#define TASK_SERIAL_SEND_PRI        ( tskIDLE_PRIORITY + 2 )
#define TASK_SERIAl_REC_PRI         ( tskIDLE_PRIORITY + 3 )
#define SERVICE_TASK_PRI            ( tskIDLE_PRIORITY + 1 )
#define TASK_SERIAL_SPEED           ( tskIDLE_PRIORITY + 3)

/* 7-SEG NUMBER DATABASE - ALL HEX DIGITS [ 0 1 2 3 4 5 6 7 8 9 A B C D E F ] */
const char_t hexnum[] = {
    (char_t)0x3FU, (char_t)0x06U, (char_t)0x5BU, (char_t)0x4FU,
    (char_t)0x66U, (char_t)0x6DU, (char_t)0x7DU, (char_t)0x07U,
    (char_t)0x7FU, (char_t)0x6FU, (char_t)0x77U, (char_t)0x7CU,
    (char_t)0x39U, (char_t)0x5EU, (char_t)0x79U, (char_t)0x71U
};

/* GLOBAL OS-HANDLES */
SemaphoreHandle_t LED_INT_BinarySemaphore;   /* Binary semaphore for LED BAR interrupt */
SemaphoreHandle_t RXC_BinarySemaphore0;      /* Binary semaphore for serial port COM_CH_0 */
SemaphoreHandle_t RXC_BinarySemaphore1;      /* Binary semaphore for serial port COM_CH_1 */
SemaphoreHandle_t RXC_BinarySemaphore2;      /* Binary semaphore for serial port COM_CH_2 */
QueueHandle_t Queue_Circumference;
QueueHandle_t Queue_LeftEnc;
QueueHandle_t Queue_RightEnc;
QueueHandle_t Queue_Rst;
QueueHandle_t Queue_TotalDist;
QueueHandle_t Queue_DeltaDist;
QueueHandle_t Queue_CurrentSpeed;
QueueHandle_t Queue_LapDist;
QueueHandle_t Queue_DeltaDist2;

/* INTERRUPTS */
/* OPC - ON INPUT CHANGE - INTERRUPT HANDLER */
uint32_t OnLED_ChangeInterrupt(void) {
    BaseType_t xHigherPTW = pdFALSE;

    if (xSemaphoreGiveFromISR(LED_INT_BinarySemaphore, &xHigherPTW) != pdPASS) {
        printf("Semaphore not given\\n");
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


/* Helper function - convert and send current speed via serial communication */
void sendSpeed(float_t speed) {
    char_t buffer[20];                           /* Large enough for float_t as string */
    sprintf(buffer, "%.2f", speed);              /* Convert float_t to string with 2 decimals, MISRA 21.6 
                                                    (deprecated function, does not check buffer length, 
                                                    but not an issue in this specific scope) */

    /* Send character by character */
    for (uint_t i = 0; buffer[i] != '\\0'; i++) {
        if (send_serial_character(COM_CH_2, (uchar_t)buffer[i]) != pdFALSE) {
            printf("Error\\n");
        }
    }

    /* Send newline to indicate end of message */
    if (send_serial_character(COM_CH_2, (uchar_t)'\\n') != pdFALSE) {
        printf("Error\\n");
    }
}

/* Helper function - convert and send current distance via serial communication */
void sendDistance(float_t distance) {
    char_t buffer[20];                    /* Large enough for float_t as string */
    sprintf(buffer, "%.2f", distance);    /* Convert float_t to string with 2 decimals */

    /* Send character by character */
    for (uint_t i = 0; buffer[i] != '\\0'; i++) {
        if (send_serial_character(COM_CH_2, (uchar_t)buffer[i]) != pdFALSE) {
            printf("Error\\n");
        }
    }

    /* Send newline to indicate end of message */
    if (send_serial_character(COM_CH_2, (uchar_t)'\\n') != pdFALSE) {
        printf("Error\\n");
    }
}

/* MAIN - SYSTEM STARTUP POINT */
void main_demo(void) {
    /* INITIALIZATION OF THE PERIPHERALS */
    if (init_7seg_comm() != 0) {
        printf("7SEG display init failed\\n");
    };
    if (init_LED_comm() != 0) {
        printf("LEDBAR init failed\\n");
    };

    if (init_serial_uplink(COM_CH_0) != 0) {
        printf("CH 0 init failed\\n");
    };
    if (init_serial_downlink(COM_CH_0) != 0) {
        printf("CH 0 init failed\\n");
    };
    if (init_serial_uplink(COM_CH_1) != 0) {
        printf("CH 1 init failed\\n");
    };
    if (init_serial_downlink(COM_CH_1) != 0) {
        printf("CH 1 init failed\\n");
    };
    if (init_serial_uplink(COM_CH_2) != 0) {
        printf("CH 2 init failed\\n");
    };
    if (init_serial_downlink(COM_CH_2) != 0) {
        printf("CH 2 init failed\\n");
    };

    /* INTERRUPT HANDLERS */
    vPortSetInterruptHandler(portINTERRUPT_SRL_OIC, OnLED_ChangeInterrupt);     /* ON INPUT CHANGE INTERRUPT HANDLER */
    vPortSetInterruptHandler(portINTERRUPT_SRL_RXC, prvProcessRXCInterrupt);    /* SERIAL RECEPTION INTERRUPT HANDLER */

    /* BINARY SEMAPHORES */
    LED_INT_BinarySemaphore = xSemaphoreCreateBinary();	    // CREATE LED INTERRUPT SEMAPHORE
    if (LED_INT_BinarySemaphore == NULL) {
        printf("Error creating LED_INT_BinarySemaphore\\n");
    }
    RXC_BinarySemaphore0 = xSemaphoreCreateBinary();		// CREATE RXC SEMAPHORE - SERIAL RECEIVE COMM
    if (RXC_BinarySemaphore0 == NULL) {
        printf("Error creating RXC_BinarySemaphore0\\n");
    }
    RXC_BinarySemaphore1 = xSemaphoreCreateBinary();		// CREATE RXC SEMAPHORE - SERIAL RECEIVE COMM
    if (RXC_BinarySemaphore1 == NULL) {
        printf("Error creating RXC_BinarySemaphore1\\n");
    }
    RXC_BinarySemaphore2 = xSemaphoreCreateBinary();		// CREATE RXC SEMAPHORE - SERIAL RECEIVE COMM
    if (RXC_BinarySemaphore2 == NULL) {
        printf("Error creating RXC_BinarySemaphore2\\n");
    }

    /* QUEUES */
    Queue_Circumference = xQueueCreate(2, sizeof(uint16_t));
    if (Queue_Circumference == NULL) {
        printf("Error creating Queue_Circumference\\n");
    }
    Queue_LeftEnc = xQueueCreate(2, sizeof(uint16_t));
    if (Queue_LeftEnc == NULL) {
        printf("Error creating Queue_LeftEnc\\n");
    }
    Queue_RightEnc = xQueueCreate(2, sizeof(uint16_t));
    if (Queue_RightEnc == NULL) {
        printf("Error creating Queue_RightEnc\\n");
    }
    Queue_Rst = xQueueCreate(2, sizeof(uint16_t));
    if (Queue_Rst == NULL) {
        printf("Error creating Queue_Rst\\n");
    }
    Queue_TotalDist = xQueueCreate(2, sizeof(float_t));
    if (Queue_TotalDist == NULL) {
        printf("Error creating Queue_TotalDist\\n");
    }
    Queue_DeltaDist = xQueueCreate(2, sizeof(float_t));
    if (Queue_DeltaDist == NULL) {
        printf("Error creating Queue_DeltaDist\\n");
    }
    Queue_DeltaDist2 = xQueueCreate(2, sizeof(float_t));
    if (Queue_DeltaDist2 == NULL) {
        printf("Error creating Queue_DeltaDist2\\n");
    }
    Queue_CurrentSpeed = xQueueCreate(2, sizeof(float_t));
    if (Queue_CurrentSpeed == NULL) {
        printf("Error creating Queue_CurrentSpeed\\n");
    }
    Queue_LapDist = xQueueCreate(2, sizeof(float_t));
    if (Queue_LapDist == NULL) {
        printf("Error creating Queue_LapDist\\n");
    }

    /* TASKS */
    BaseType_t task;
    task = xTaskCreate(SerialSend_Task0, "STx0", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAL_SEND_PRI, NULL);        /* SERIAL TRANSMITTER TASK 0 */
    if (task != pdPASS) {
        printf("Error creating STx0 task\\n");
    }
    task = xTaskCreate(SerialReceive_Task0, "SRx0", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAl_REC_PRI, NULL);      /* SERIAL RECEIVER TASK 0 */
    if (task != pdPASS) {
        printf("Error creating SRx0 task\\n");
    }
    task = xTaskCreate(SerialReceive_Task1, "SRx1", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAl_REC_PRI, NULL);      /* SERIAL RECEIVER TASK 1 */
    if (task != pdPASS) {
        printf("Error creating SRx1 task\\n");
    }
    task = xTaskCreate(SerialSend_Task2, "STx2", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAL_SEND_PRI, NULL);        /* SERIAL TRANSMITTER TASK 2 */
    if (task != pdPASS) {
        printf("Error creating STx2 task\\n");
    }
    task = xTaskCreate(SerialReceive_Task2, "SRx2", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAl_REC_PRI, NULL);      /* SERIAL RECEIVER TASK 2 */
    if (task != pdPASS) {
        printf("Error creating SRx2 task\\n");
    }
    task = xTaskCreate(CalcDistance_Task, "CalcDist", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAl_REC_PRI, NULL);    /* DISTANCE CALCULATION TASK */
    if (task != pdPASS) {
        printf("Error creating CalcDist task\\n");
    }
    task = xTaskCreate(MeasureSpeed_Task, "MeasureSpd", configMINIMAL_STACK_SIZE, NULL, TASK_SERIAL_SPEED, NULL);    /* SPEED MEASUREMENT TASK */
    if (task != pdPASS) {
        printf("Error creating MeasureSpd task\\n");
    }
    task = xTaskCreate(LCD_Task, "LCD", configMINIMAL_STACK_SIZE, NULL, SERVICE_TASK_PRI, NULL);                     /* LCD DISPLAY TASK */
    if (task != pdPASS) {
        printf("Error creating LCD task\\n");
    }

    /* START SCHEDULER */
    vTaskStartScheduler();
    for (;;) {}
}


/* Task that sends trigger ('X') every 200ms to channels 0 and 1 to request automatic increments */
void SerialSend_Task0(const void* pvParameters) {
    (void)pvParameters;  /* Explicitly mark parameter as unused */

    for (;;) {  /* MISRA compliant infinite loop */
        vTaskDelay(pdMS_TO_TICKS(200));
        if (send_serial_character(COM_CH_0, (uchar_t)'X') != pdFALSE) {
            printf("Error\\n");
        }
        if (send_serial_character(COM_CH_1, (uchar_t)'X') != pdFALSE) {
            printf("Error\\n");
        }
    }
}


/* Task reading messages from channel 0 (reads increments from the left wheel) */
void SerialReceive_Task0(const void* pvParameters) {
    (void)pvParameters;  

    char_t receivedChar = '\\0';   
    char_t buffer[6] = { '\\0' };  
    uint_t index = 0U;
    uint_t wheel_increment = 0U;
    bool_t reading = (bool_t)0;   
    BaseType_t QS;

    for (;;) {
        if (xSemaphoreTake(RXC_BinarySemaphore0, portMAX_DELAY) != pdTRUE) {
        }
        else {
            /* Added else for Rule 15.7 compliance */
        }

        if (get_serial_character(COM_CH_0, (uint8_t*)&receivedChar) != pdFALSE) {
        }
        else {
        }

        if (receivedChar == (char_t)'L') {
            index = 0U;
            reading = (bool_t)1;
        }
        /* Rule 12.1 fix - added parentheses for precedence */
        else if ((receivedChar == (char_t)'<') && reading) {
            buffer[index] = '\\0';  /* Null terminator */

            /* MISRA compliant string-to-int conversion (avoids atoi) */
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

            QS = xQueueSend(Queue_LeftEnc, &wheel_increment, 0);
            if (QS != pdPASS) {
            }
            else {
            }
            reading = (bool_t)0;
        }
        /* Rule 12.1 fix - added parentheses for all operators */
        else if (reading &&
            (receivedChar >= (char_t)'0') &&
            (receivedChar <= (char_t)'9') &&
            (index < (sizeof(buffer) - 1U))) {

            /* Rule 13.3 fix - separated increment from assignment */
            buffer[index] = receivedChar;
            index++;
        }
        else {
            /* Added else at end of if-else chain for Rule 15.7 */
        }
    }
}


/* Task reading messages from channel 1 (reads increments from the right wheel) */
void SerialReceive_Task1(const void* pvParameters) {
    (void)pvParameters;  

    char_t receivedChar = '\\0';   
    char_t buffer[6] = { '\\0' };  
    uint_t index = 0U;
    uint_t wheel_increment = 0U;
    bool_t reading = (bool_t)0;   
    BaseType_t QR;

    for (;;) {
        if (xSemaphoreTake(RXC_BinarySemaphore1, portMAX_DELAY) != pdTRUE) {
        }
        else {
        }

        /* Correct pointer casting */
        if (get_serial_character(COM_CH_1, (uint8_t*)&receivedChar) != pdFALSE) {
        }
        else {
        }

        if (receivedChar == (char_t)'R') {
            index = 0U;
            reading = (bool_t)1;
        }
        else if ((receivedChar == (char_t)'<') && reading) {
            buffer[index] = '\\0'; 

            /* MISRA compliant string-to-int conversion */
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

            QR = xQueueSend(Queue_RightEnc, &wheel_increment, 0);
            if (QR != pdPASS) {
            }
            else {
            }
            reading = (bool_t)0;
        }
        else if (reading &&
            (receivedChar >= (char_t)'0') &&
            (receivedChar <= (char_t)'9') &&
            (index < (sizeof(buffer) - 1U))) {

            buffer[index] = receivedChar;
            index++;
        }
        else {
        }
    }
}

/* PC Communication Task - enables wheel circumference configuration and distance reset */
void SerialReceive_Task2(const void* pvParameters) {
    (void)pvParameters;  

    uint8_t cc = 0U;                         /* Received character */
    char_t circumference_str[6] = { '\\0' };  /* String for number (max 5 digits + NULL) */
    uint16_t reset_flag = 1U;                /* Reset flag */
    uint16_t wheel_circumference = 0U;       /* Wheel circumference value */
    char_t r_buffer2[BUFFER_SIZE];           /* Receive buffer */
    uint8_t r_point2 = 0U;                   /* Buffer index */
    BaseType_t QR2;

    for (;;) {
        /* Wait for data arrival via serial comm */
        if (xSemaphoreTake(RXC_BinarySemaphore2, portMAX_DELAY) != pdTRUE) {
        }
        else {
        }

        if (get_serial_character(COM_CH_2, &cc) != pdFALSE) { 
        }
        else {
        }

        if (cc == (uint8_t)13U) {                 /* If CR (Carriage Return) is received - End of message */
            r_buffer2[r_point2] = '\\0';           /* Terminate string */

            if ((r_point2 == (uint8_t)5U) &&
                (r_buffer2[0] == 'R') &&
                (r_buffer2[1] == 'E') &&
                (r_buffer2[2] == 'S') &&
                (r_buffer2[3] == 'E') &&
                (r_buffer2[4] == 'T')) {

                /* If RESET command received, reset distance values */
                reset_flag = 0U;
                printf("Distance reset!\\n");

                QR2 = xQueueSend(Queue_Rst, &reset_flag, 0);
                if (QR2 != pdPASS) {
                }
                else {
                }

                reset_flag = 1U;              /* Return flag to 1 */
                QR2 = xQueueSend(Queue_Rst, &reset_flag, 0);
                if (QR2 != pdPASS) {
                }
                else {
                }
            }
            else if ((r_point2 >= (uint8_t)4U) &&
                (r_buffer2[0] == 'O') &&
                (r_buffer2[1] == 'M')) {

                uint8_t cnt = 0U;

                for (uint8_t i = 2U; i < r_point2; i++) {
                    if ((r_buffer2[i] >= '0') && (r_buffer2[i] <= '9')) {   /* Check if it's a digit */
                        circumference_str[cnt] = r_buffer2[i];              /* Add digit to string */
                        cnt++;
                    }
                    else {
                    }
                }
                circumference_str[cnt] = '\\0';       /* Null terminate */

                /* MISRA compliant string-to-int conversion */
                wheel_circumference = 0U;
                {
                    uint_t j = 0U;
                    while ((j < (uint_t)cnt) &&
                        (circumference_str[j] >= (char_t)'0') &&
                        (circumference_str[j] <= (char_t)'9')) {

                        uchar_t uchar_value = (uchar_t)circumference_str[j];
                        uint_t char_value = (uint_t)uchar_value;
                        uint_t zero_value = (uint_t)'0';
                        uint_t digit_value = char_value - zero_value;

                        wheel_circumference = (uint16_t)((wheel_circumference * 10U) + (uint16_t)digit_value);

                        j++;
                    }
                }

                QR2 = xQueueSend(Queue_Circumference, &wheel_circumference, 0);
                if (QR2 != pdPASS) {
                }
                else {
                }

                printf("Wheel circumference set: %u cm\\n", wheel_circumference);
            }
            else {
            }

            r_point2 = 0U;               /* Reset buffer index after processing */
        }
        else {
            if (r_point2 < (uint8_t)(BUFFER_SIZE - 1)) {
                r_buffer2[r_point2] = (char_t)cc;
                r_point2++;
            }
            else {
            }
        }
    }
}


/* Task for calculating total distance, LAP distance, and distance per increment */
void CalcDistance_Task(const void* pvParameters) {
    (void)pvParameters;  

    uint16_t wheelCircumference = 0U;
    uint16_t incLeft = 0U;
    uint16_t incRight = 0U;
    float_t distLeftEnc = 0.0f;
    float_t distRightEnc = 0.0f;
    float_t dist_per_increment = 0.0f;
    float_t totalDist = 0.0f;
    float_t totalDistStop = 0.0f;
    float_t totalDistStart = 0.0f;
    uint16_t reset_flag = 1U;
    uint8_t led_data = 0U;
    uint8_t prevLedState = 0U;
    float_t deltaDistLeft = 0.0f;
    float_t deltaDistRight = 0.0f;
    float_t deltaTotalDist = 0.0f;
    BaseType_t QS2, QR;


    for (;;) {
        /* Read data from queues */
        QS2 = xQueueReceive(Queue_Circumference, &wheelCircumference, 0U);
        if (QS2 != pdPASS) {
        }

        QS2 = xQueueReceive(Queue_LeftEnc, &incLeft, portMAX_DELAY);
        if (QS2 != pdPASS) {
        }

        QS2 = xQueueReceive(Queue_RightEnc, &incRight, portMAX_DELAY);
        if (QS2 != pdPASS) {
        }

        QS2 = xQueueReceive(Queue_Rst, &reset_flag, 0U);
        if (QS2 != pdPASS) {
        }

        /* Calculate distance per increment */
        dist_per_increment = (float_t)wheelCircumference / 36000.0f;   /* cm per increment */

        deltaDistLeft = dist_per_increment * (float_t)incLeft;
        deltaDistRight = dist_per_increment * (float_t)incRight;

        /* Distance covered in the last 0.2s */
        deltaTotalDist = (float_t)reset_flag * (deltaDistLeft + deltaDistRight) / 2.0f;
        QR = xQueueSend(Queue_DeltaDist, &deltaTotalDist, 0U);
        if (QR != pdPASS) {
        }

        /* Update cumulative distance */
        distLeftEnc = (float_t)reset_flag * (distLeftEnc + deltaDistLeft);
        distRightEnc = (float_t)reset_flag * (distRightEnc + deltaDistRight);

        /* Total distance of the vehicle from start */
        totalDist = (float_t)reset_flag * (distLeftEnc + distRightEnc) / 2.0f;
        printf("TOTAL DISTANCE: %f cm\\n", totalDist);
        QR = xQueueSend(Queue_TotalDist, &totalDist, 0U);
        if (QR != pdPASS) {
        }

        /* LAP Measurement: Distance between Start (first LED in col 1) and Stop (second LED in col 1) */
        if (xSemaphoreTake(LED_INT_BinarySemaphore, 0U) == pdPASS) {
            if (get_LED_BAR(0U, &led_data) != 0) {
                printf("Error retrieving LEDBAR values\\n");
            }

            /* LED for START is bit 7 (0x80) in column 1 (start LAP measurement) */
            if (((prevLedState & 0x80U) == 0U) && ((led_data & 0x80U) != 0U)) {  /* LED 7 was off, now on -> START event */
                totalDistStart = totalDist;
                if (set_LED_BAR(1U, 0x80U) != 0) { /* Visually confirm start */
                    printf("Error setting LED\\n");
                }
            }

            /* LED for STOP is bit 6 (0x40) in column 1 (stop LAP measurement) */
            if (((prevLedState & 0x40U) == 0U) && ((led_data & 0x40U) != 0U)) { /* LED 6 was off, now on -> STOP event */
                totalDistStop = totalDist - totalDistStart;
                QR = xQueueSend(Queue_LapDist, &totalDistStop, 0U);
                if (QR != pdPASS) {
                }
                if (set_LED_BAR(1U, 0x00U) != 0) { /* Visually confirm stop */
                    printf("Error setting LED\\n");
                }
            }

            /* Update previous state */
            prevLedState = led_data;
        }
    }
}


/* Task to calculate moving speed based on traveled distance */
void MeasureSpeed_Task(const void* pvParameters) {
    (void)pvParameters;  

    float_t deltaTotalDist = 0.0f;
    float_t current_speed = 0.0f;
    BaseType_t QS, QR;


    for (;;) {
        QS = xQueueReceive(Queue_DeltaDist, &deltaTotalDist, 0);
        if (QS != pdPASS) {
        }
        /* Speed in cm/s converted to km/h */
        current_speed = (deltaTotalDist / 0.2f) * 0.036f;    /* (cm/s -> km/h) */
        QR = xQueueSend(Queue_CurrentSpeed, &current_speed, 0);
        if (QR != pdPASS) {
        }
        printf("SPEED IS: %f  KM/H\\n", current_speed);
        sendSpeed(current_speed);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task sending current distance to PC */
void SerialSend_Task2(const void* pvParameters) {
    (void)pvParameters;  

    float_t currentDist = 0.0f;
    BaseType_t QS, QR;

    for (;;) {
        QS = xQueueReceive(Queue_DeltaDist, &currentDist, 0);
        if (QS != pdPASS) {
        }
        sendDistance(currentDist);
        QR = xQueueSend(Queue_DeltaDist2, &currentDist, 0);
        if (QR != pdPASS) {
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task displaying current speed & distance OR total distance & lap distance on 7-segment display */
void LCD_Task(const void* pvParameters) {
    (void)pvParameters;  

    float_t deltaTotalDist = 0.0f;
    float_t current_speed = 0.0f;
    float_t totalDist = 0.0f;
    float_t lapDist = 0.0f;
    uint16_t speedInt = 0;
    uint16_t deltaDistInt = 0;
    uint16_t totalDistInt = 0;
    uint16_t lapDistInt = 0;
    uint8_t led_data;
    BaseType_t QS;

    for (;;) {
        /* Read data from queues */
        QS = xQueueReceive(Queue_DeltaDist2, &deltaTotalDist, 0);
        if (QS != pdPASS) {
        }
        QS = xQueueReceive(Queue_CurrentSpeed, &current_speed, 0);
        if (QS != pdPASS) {
        }
        QS = xQueueReceive(Queue_TotalDist, &totalDist, 0);
        if (QS != pdPASS) {
        }
        QS = xQueueReceive(Queue_LapDist, &lapDist, 0);
        if (QS != pdPASS) {
        }

        /* Check LED bar status */
        if (get_LED_BAR(2, &led_data) != 0) {
            printf("Error retrieving LEDBAR values\\n");
        }

        if (led_data == (uint8_t)0x80U) {
            /* Display Speed and Delta Distance */

            /* MISRA-compliant rounding */
            float_t temp_speed = current_speed + 0.5f;
            speedInt = (uint16_t)temp_speed;
            float_t temp_delta = deltaTotalDist + 0.5f;
            deltaDistInt = (uint16_t)temp_delta;

            /* Render Current Speed */
            uint8_t speed_digit_1 = (uint8_t)((speedInt / 1000U) % 10U);
            uint8_t speed_digit_2 = (uint8_t)((speedInt / 100U) % 10U);
            uint8_t speed_digit_3 = (uint8_t)((speedInt / 10U) % 10U);
            uint8_t speed_digit_4 = (uint8_t)(speedInt % 10U);

            if (select_7seg_digit(0) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[speed_digit_1]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(1) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[speed_digit_2]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(2) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[speed_digit_3]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(3) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[speed_digit_4]) != pdFALSE) { printf("Display set error\\n"); }

            /* Render Current Distance */
            uint8_t dist_digit_1 = (uint8_t)((deltaDistInt / 1000U) % 10U);
            uint8_t dist_digit_2 = (uint8_t)((deltaDistInt / 100U) % 10U);
            uint8_t dist_digit_3 = (uint8_t)((deltaDistInt / 10U) % 10U);
            uint8_t dist_digit_4 = (uint8_t)(deltaDistInt % 10U);

            if (select_7seg_digit(5) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[dist_digit_1]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(6) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[dist_digit_2]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(7) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[dist_digit_3]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(8) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[dist_digit_4]) != pdFALSE) { printf("Display set error\\n"); }
        }
        else if (led_data == (uint8_t)0x01U) {
            /* Display Total Distance and Lap Distance */

            /* MISRA-compliant rounding */
            float_t temp_total = totalDist + 0.5f;
            totalDistInt = (uint16_t)temp_total;
            float_t temp_lap = lapDist + 0.5f;
            lapDistInt = (uint16_t)temp_lap;

            /* Render Total Distance */
            uint8_t total_digit_1 = (uint8_t)((totalDistInt / 1000U) % 10U);
            uint8_t total_digit_2 = (uint8_t)((totalDistInt / 100U) % 10U);
            uint8_t total_digit_3 = (uint8_t)((totalDistInt / 10U) % 10U);
            uint8_t total_digit_4 = (uint8_t)(totalDistInt % 10U);

            if (select_7seg_digit(0) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[total_digit_1]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(1) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[total_digit_2]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(2) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[total_digit_3]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(3) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[total_digit_4]) != pdFALSE) { printf("Display set error\\n"); }

            /* Render LAP Distance */
            uint8_t lap_digit_1 = (uint8_t)((lapDistInt / 1000U) % 10U);
            uint8_t lap_digit_2 = (uint8_t)((lapDistInt / 100U) % 10U);
            uint8_t lap_digit_3 = (uint8_t)((lapDistInt / 10U) % 10U);
            uint8_t lap_digit_4 = (uint8_t)(lapDistInt % 10U);

            if (select_7seg_digit(5) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[lap_digit_1]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(6) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[lap_digit_2]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(7) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[lap_digit_3]) != pdFALSE) { printf("Display set error\\n"); }
            if (select_7seg_digit(8) != pdFALSE) { printf("Display select error\\n"); }
            if (set_7seg_digit((uint8_t)hexnum[lap_digit_4]) != pdFALSE) { printf("Display set error\\n"); }
        }
        else {
            /* No LED pressed -> Clear display */
            uint8_t i;
            for (i = 0U; i < 9U; i++) {
                if (select_7seg_digit(i) != pdFALSE) { printf("Display select error\\n"); }
                if (set_7seg_digit(0x00U) != pdFALSE) { printf("Display set error\\n"); }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(150)); /* Refresh every 150ms */
    }
}
