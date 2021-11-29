/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <string.h>                     // Defines NULL
#include "definitions.h"                // SYS function prototypes

/* Flash */
//0x450000
#define FLASH_CRC32_ADDR            0x450000
#define FLASH_START_ADDRESS         0x400000
#define APP_FLASH_LIMIT             0x050000
#define PAGE_SIZE                   512


/* RAM */
#define SRAM_RST_SIZE               32768
#define RX_BUFFER_SIZE              256
#define BUFFER_SIZE                 128


char test_status_str[4][25] = {"CLASSB_TEST_NOT_EXECUTED",
                                "CLASSB_TEST_PASSED",
                                "CLASSB_TEST_FAILED",
                                "CLASSB_TEST_INPROGRESS"};
char console_message[] = "\r\n Type a line of characters and press the Enter key. \r\n\
 \r\n Entered line will be echoed back, and the LED0 is toggled on the reception of each character. \r\n";
char newline[] = "\r\n ";
char error_message[] = "\r\n!!!!! USART error has occurred !!!!!r\n";
char receive_buffer[RX_BUFFER_SIZE] = {};
char data = 0;
uint32_t crc_val[1] = {0};
uint32_t * app_crc_addr = (uint32_t *)FLASH_CRC32_ADDR;


/*============================================================================
bool runtimeClassBChecks(void)
------------------------------------------------------------------------------
Purpose: Execute periodic run-time self-tests
Input  : None.
Output : None.
Notes  : Insert the required self-tests into this function.
============================================================================*/
bool runtimeClassBChecks(void)
{
    bool ret_val = false;
    CLASSB_TEST_STATUS classb_rst1_status = CLASSB_TEST_NOT_EXECUTED;
    CLASSB_TEST_STATUS classb_rst2_status = CLASSB_TEST_NOT_EXECUTED;
    
    classb_rst1_status = CLASSB_CPU_RegistersTest(true);
    classb_rst2_status = CLASSB_FlashCRCTest(0, APP_FLASH_LIMIT,
            *(uint32_t *)FLASH_CRC32_ADDR, true);
    
    if ((classb_rst1_status == CLASSB_TEST_PASSED) &&
            (classb_rst2_status == CLASSB_TEST_PASSED))
    {
        ret_val = true;
    }
    
    return ret_val;
}

/*============================================================================
uint32_t readCRC(void)
------------------------------------------------------------------------------
Purpose: Reads flash sector and returns crc value
Input  : None.
Output : None.
Notes  : 
============================================================================*/
uint32_t readCRC(void)
{
    uint32_t l_crc_val[1]  ={0};
    
    while(EFC_IsBusy());
    
    EFC_Read( (uint32_t *)&l_crc_val[0], (uint32_t)4, (uint32_t)FLASH_CRC32_ADDR );
    
    return l_crc_val[0];

}

/*============================================================================
bool writeCRC(void)
------------------------------------------------------------------------------
Purpose: Reads flash sector and returns crc value
Input  : None.
Output : None.
Notes  : 
============================================================================*/
 bool writeCRC(uint32_t crc_val)
{
    uint32_t data [BUFFER_SIZE] = {0};
    bool test_res = false;
    
    while(EFC_IsBusy());
    
    test_res = EFC_Read( (uint32_t *)data, (uint32_t)PAGE_SIZE, (uint32_t)FLASH_CRC32_ADDR  );
    
    while(EFC_IsBusy());
    
    /*Erase the page*/
    test_res = EFC_SectorErase((uint32_t)FLASH_CRC32_ADDR );
    
    while(EFC_IsBusy());
    
    /* write new value */
    data[0] = crc_val;

    /*Program 512 byte page*/
    test_res = EFC_PageWrite(data, (uint32_t)FLASH_CRC32_ADDR );
    
    return test_res;
}

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{    
    uint16_t rx_counter=0;
    bool rst_status = false;
    
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    printf("\r\n\r\n        Class B API Usage Demo      \r\n\r\n");
    printf("\r\n\r\n Class B start-up self-test results \r\n");
    CLASSB_TEST_STATUS classb_test_status = CLASSB_TEST_NOT_EXECUTED;
    classb_test_status = CLASSB_GetTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_CPU);
    printf("\r\n Result of CPU SST is %s\r\n", test_status_str[classb_test_status]);
    classb_test_status = CLASSB_GetTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_FPU);
    printf("\r\n Result of FPU SST is %s\r\n", test_status_str[classb_test_status]);
    classb_test_status = CLASSB_GetTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_PC);
    printf("\r\n Result of PC SST is %s\r\n", test_status_str[classb_test_status]);
    classb_test_status = CLASSB_GetTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_RAM);
    printf("\r\n Result of SRAM SST is %s\r\n", test_status_str[classb_test_status]);
    classb_test_status = CLASSB_GetTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_FLASH);
    printf("\r\n Result of Flash SST is %s\r\n", test_status_str[classb_test_status]);
    classb_test_status = CLASSB_GetTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_CLOCK);
    printf("\r\n Result of Clock SST is %s\r\n", test_status_str[classb_test_status]);
    classb_test_status = CLASSB_GetTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_INTERRUPT);
    printf("\r\n Result of Interrupt SST is %s\r\n", test_status_str[classb_test_status]);
    
    WDT_Clear();

    printf("\r\n\r\n Class B run-time self-tests \r\n");
    classb_test_status = CLASSB_TEST_FAILED;
    classb_test_status = CLASSB_CPU_RegistersTest(true);
    printf("\r\n Result of CPU RST is %s\r\n", test_status_str[classb_test_status]);
    
    WDT_Clear();
    
    classb_test_status = CLASSB_TEST_FAILED;
    classb_test_status = CLASSB_CPU_PCTest(true);
    printf("\r\n Result of PC RST is %s\r\n", test_status_str[classb_test_status]);
    
    WDT_Clear();

    // Enable cache before SRAM test
    SYS_CACHE_EnableDCache();
   
    classb_test_status = CLASSB_TEST_FAILED;
    __disable_irq();
    classb_test_status = CLASSB_SRAM_MarchTestInit((uint32_t *)0x20400400,
                SRAM_RST_SIZE, CLASSB_SRAM_MARCH_C, true, CLASSB_MEM_REGION_SRAM);
    __enable_irq();
    
    
    printf("\r\n Result of SRAM RST is %s\r\n", test_status_str[classb_test_status]);
    
    WDT_Clear();
    // Generate CRC-32 over internal flash address 0 to APP_FLASH_LIMIT
    crc_val[0] = CLASSB_FlashCRCGenerate(0, APP_FLASH_LIMIT);
    
    WDT_Clear();
    
    // Use NVMCTRL to write the calculated CRC into a Flash location   
    bool efc_ret_val = false;
    efc_ret_val = writeCRC((uint32_t) crc_val[0]);
            
    if(efc_ret_val)
    {
        printf("\r\n EFC write succeeded\r\n");
    }
    else
    {
        printf("\r\n EFC write failed\r\n");
    }
    
    WDT_Clear();
    
    classb_test_status = CLASSB_TEST_FAILED;
    
    // Read CRC from flash
    crc_val[0] = readCRC();
    
    WDT_Clear();

    classb_test_status = CLASSB_FlashCRCTest(0, APP_FLASH_LIMIT,
        *(uint32_t *)crc_val, true);
    
    WDT_Clear();
    
    printf("\r\n Result of Flash RST is %s\r\n", test_status_str[classb_test_status]);
    
    WDT_Clear();
    __disable_irq();
    // Note - Systick is default DIV2
    classb_test_status = CLASSB_ClockTest(150000000, 5, 500, true);
    __enable_irq();
    printf("\r\n Result of CPU Clock RST is %s\r\n", test_status_str[classb_test_status]);
        
    //Drive LOW on the pin to be tested.
    LED0_OutputEnable();
    LED0_Clear();
    CLASSB_RST_IOTest(PORTA, PIN5, PORT_PIN_LOW);
    classb_test_status = CLASSB_GetTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_IO);
    printf("\r\n Result of PA5 LOW test is %s\r\n", test_status_str[classb_test_status]);
    
    //Drive HIGH on the pin to be tested.
    LED0_OutputEnable();
    LED0_Set();
    CLASSB_IO_InputSamplingEnable(PORTA, PIN5);
    CLASSB_RST_IOTest(PORTA, PIN5, PORT_PIN_HIGH);
    classb_test_status = CLASSB_GetTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_IO);
    printf("\r\n Result of PA5 HIGH is %s\r\n", test_status_str[classb_test_status]);
    printf("\r\n\r\n Class B API Tests complete. Running UART application \r\n");
    
    printf("%s", console_message);

    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        WDT_Clear();
        /* Check if there is a received character */
        if(USART1_ReceiverIsReady() == true)
        {
            if(USART1_ErrorGet() == USART_ERROR_NONE)
            {
                USART1_Read(&data, 1);
                LED0_OutputEnable();
                LED0_Toggle();
                if((data == '\n') || (data == '\r'))
                {
                    printf("\r\n Received : ");
                    USART1_Write(&receive_buffer[0],rx_counter);
                    printf("\r\n");
                    rx_counter = 0;
                    printf("\r\n Executing periodic run-time tests : ");
                    rst_status = runtimeClassBChecks();
                    if (rst_status == true)
                    {
                        printf(" All periodic tests passed \r\n");
                    }
                    else
                    {
                        printf(" Periodic test failure \r\n");
                    }
                }
                else
                {
                    receive_buffer[rx_counter++] = data;
                }
            }
            else
            {
                USART1_Write(&error_message[0],sizeof(error_message));
            }
        }
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}



/*******************************************************************************
 End of File
*/

