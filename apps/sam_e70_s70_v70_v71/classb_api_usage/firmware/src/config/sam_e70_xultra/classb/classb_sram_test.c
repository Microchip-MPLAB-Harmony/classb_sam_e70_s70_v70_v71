/*******************************************************************************
  Class B Library v2.0.0 Release

  Company:
    Microchip Technology Inc.

  File Name:
    classb_sram_test.c

  Summary:
    Class B Library SRAM self-test source file

  Description:
    This file provides SRAM self-test function.

*******************************************************************************/

/*******************************************************************************
* Copyright (C) 2020 Microchip Technology Inc. and its subsidiaries.
*
* Subject to your compliance with these terms, you may use Microchip software
* and any derivatives exclusively with Microchip products. It is your
* responsibility to comply with third party license terms applicable to your
* use of third party software (including open source software) that may
* accompany Microchip software.
*
* THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER
* EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY IMPLIED
* WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS FOR A
* PARTICULAR PURPOSE.
*
* IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE,
* INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND
* WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS
* BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO THE
* FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN
* ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
* THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
*******************************************************************************/

/*----------------------------------------------------------------------------
 *     include files
 *----------------------------------------------------------------------------*/
#include "classb/classb_sram_test.h"

/*----------------------------------------------------------------------------
 *     Constants
 *----------------------------------------------------------------------------*/
/* This final address value will be fixed at 32KB */
#define CLASSB_ITCM_FINAL_WORD_ADDRESS      (0x00007ffcU)
#define CLASSB_ITCM_BUFF_START_ADDRESS      (0x00000200U)
#define CLASSB_ITCM_TEMP_STACK_ADDRESS      (0x00000100U)
#define CLASSB_ITCM_ALL_32BITS_HIGH         (0xFFFFFFFFU)

/* This final address value will be fixed at 32KB */
#define CLASSB_DTCM_FINAL_WORD_ADDRESS      (0x00007ffcU)
#define CLASSB_DTCM_BUFF_START_ADDRESS      (0x20000200U)
#define CLASSB_DTCM_TEMP_STACK_ADDRESS      (0x20000100U)
#define CLASSB_DTCM_ALL_32BITS_HIGH         (0xFFFFFFFFU)

/* This final address value will differ according to device variant */
#define CLASSB_SRAM_FINAL_WORD_ADDRESS      (0x2045fffcU)
#define CLASSB_SRAM_BUFF_START_ADDRESS      (0x20400200U)
#define CLASSB_SRAM_TEMP_STACK_ADDRESS      (0x20400100U)
#define CLASSB_SRAM_ALL_32BITS_HIGH         (0xFFFFFFFFU)

/*----------------------------------------------------------------------------
 *     Global Variables
 *----------------------------------------------------------------------------*/
extern uint32_t _heap;
extern uint32_t _min_heap_size;
extern uint32_t _stack;

/*----------------------------------------------------------------------------
 *     Functions
 *----------------------------------------------------------------------------*/


extern void _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE test_type,
    CLASSB_TEST_ID test_id, CLASSB_TEST_STATUS value);



/*============================================================================
static uint32_t _CLASSB_GetStackPointer(void)
------------------------------------------------------------------------------
Purpose: Get the address stored in the Stack Pointer.
Input  : None
Output : Address stored in the Stack Pointer register.
Notes  : This function is used by SRAM tests.
============================================================================*/
static uint32_t _CLASSB_GetStackPointer(void)
{
    uint32_t result = 0;

    __ASM volatile ("MRS %0, msp\n" : "=r" (result));

    return result;
}

/*============================================================================
static void _CLASSB_SetStackPointer(uint32_t new_stack_address)
------------------------------------------------------------------------------
Purpose: Store a new address into the Stack Pointer.
Input  : New address for the Stack.
Output : None
Notes  : This function is used by SRAM tests.
============================================================================*/
static void _CLASSB_SetStackPointer(uint32_t new_stack_address)
{
    __ASM volatile ("MSR msp, %0\n" : : "r" (new_stack_address));
}

/*============================================================================
static void _CLASSB_MemCopy(uint32_t* dest, uint32_t* src, uint32_t size_in_bytes)
------------------------------------------------------------------------------
Purpose: Copies given number of bytes, from one SRAM area to the other.
Input  : Destination address, source address and size
Output : None
Notes  : This function is used by SRAM tests.
============================================================================*/
static void _CLASSB_MemCopy(uint32_t* dest, uint32_t* src, uint32_t size_in_bytes)
{
    uint32_t i = 0;
    uint32_t size_in_words = size_in_bytes / 4;

    for (i = 0; i < size_in_words; i++)
    {
        dest[i] = src[i];
    }
}

/*============================================================================
bool CLASSB_RAMMarchC(  uint32_t * start_addr, 
                        uint32_t test_size_bytes,
                        CLASSB_MEM_REGION mem_region )
------------------------------------------------------------------------------
Purpose: Runs March C algorithm on the given SRAM area
Input  : Start address, size and memory region
Output : Success or failure
Notes  : This function is used by SRAM tests. It performs the following,
        // March C
        // Low to high, write zero
        // Low to high, read zero write one
        // Low to high, read one write zero
        // Low to high, read zero

        // High to low, read zero write one
        // High to low, read one write zero
        // High to low, read zero
============================================================================*/
bool CLASSB_RAMMarchC(  uint32_t * start_addr, 
                        uint32_t test_size_bytes,
                        CLASSB_MEM_REGION mem_region )
{
    bool sram_march_c_result = true;
    int32_t i = 0;
    uint32_t sram_read_val = 0;
    uint32_t test_size_words = (test_size_bytes / 4);
    uint32_t test_buffer_size = 0;
    
    /* memory region switch */
    switch(mem_region)
    {
        case CLASSB_MEM_REGION_ITCM:
            test_buffer_size = CLASSB_ITCM_TEST_BUFFER_SIZE;
            break;
        case CLASSB_MEM_REGION_DTCM:
            test_buffer_size = CLASSB_DTCM_TEST_BUFFER_SIZE;
            break;
        case CLASSB_MEM_REGION_SRAM:
            test_buffer_size = CLASSB_SRAM_TEST_BUFFER_SIZE;
            break;
        default:
            sram_march_c_result = false;
            break;           
    }

    // Test size is limited to CLASSB_SRAM_TEST_BUFFER_SIZE
    if (test_size_bytes > test_buffer_size)
    {
        sram_march_c_result = false;
    }

    // Perform the next check only if the previous stage is passed
    if (sram_march_c_result == true)
    {
        // Low to high, write zero
        for (i = 0; i < test_size_words; i++)
        {
            start_addr[i] = 0;
        }
        // Low to high, read zero write one
        for (i = 0; i < test_size_words; i++)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == 0)
            {
                start_addr[i] = CLASSB_SRAM_ALL_32BITS_HIGH;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // Low to high, read one write zero
        for (i = 0; i < test_size_words; i++)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == CLASSB_SRAM_ALL_32BITS_HIGH)
            {
                start_addr[i] = 0;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // Low to high, read zero
        for (i = 0; i < test_size_words; i++)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val != 0)
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // High to low, read zero, write one
        for (i = (test_size_words - 1); i >= 0 ; i--)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == 0)
            {
                start_addr[i] = CLASSB_SRAM_ALL_32BITS_HIGH;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // High to low, read one, write zero
        for (i = (test_size_words - 1); i >= 0 ; i--)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == CLASSB_SRAM_ALL_32BITS_HIGH)
            {
                start_addr[i] = 0;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // High to low, read zero
        for (i = (test_size_words - 1); i >= 0 ; i--)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val != 0)
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    return sram_march_c_result;
}

/*============================================================================
bool CLASSB_RAMMarchCMinus( uint32_t * start_addr, 
                            uint32_t test_size_bytes,
                            CLASSB_MEM_REGION mem_region )
------------------------------------------------------------------------------
Purpose: Runs March C algorithm on the given SRAM area
Input  : Start address, size and memory region
Output : Success or failure
Notes  : This function is used by SRAM tests. It performs the following,
        // March C minus
        // Low to high, write zero
        // Low to high, read zero write one
        // Low to high, read one write zero

        // High to low, read zero write one
        // High to low, read one write zero
        // High to low, read zero
============================================================================*/
bool CLASSB_RAMMarchCMinus( uint32_t * start_addr, 
                            uint32_t test_size_bytes,
                            CLASSB_MEM_REGION mem_region )
{
    bool sram_march_c_result = true;
    int32_t i = 0;
    uint32_t sram_read_val = 0;
    uint32_t test_size_words = (test_size_bytes / 4);
    uint32_t test_buffer_size = 0;
    
    /* memory region switch */
    switch(mem_region)
    {
        case CLASSB_MEM_REGION_ITCM:
            test_buffer_size = CLASSB_ITCM_TEST_BUFFER_SIZE;
            break;
        case CLASSB_MEM_REGION_DTCM:
            test_buffer_size = CLASSB_DTCM_TEST_BUFFER_SIZE;
            break;
        case CLASSB_MEM_REGION_SRAM:
            test_buffer_size = CLASSB_SRAM_TEST_BUFFER_SIZE;
            break;
        default:
            sram_march_c_result = false;
            break;           
    }

    // Test size is limited to CLASSB_SRAM_TEST_BUFFER_SIZE
    if (test_size_bytes > test_buffer_size)
    {
        sram_march_c_result = false;
    }

    // Perform the next check only if the previous stage is passed
    if (sram_march_c_result == true)
    {
        // Low to high, write zero
        for (i = 0; i < test_size_words; i++)
        {
            start_addr[i] = 0;
        }
        // Low to high, read zero write one
        for (i = 0; i < test_size_words; i++)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == 0)
            {
                start_addr[i] = CLASSB_SRAM_ALL_32BITS_HIGH;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // Low to high, read one write zero
        for (i = 0; i < test_size_words; i++)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == CLASSB_SRAM_ALL_32BITS_HIGH)
            {
                start_addr[i] = 0;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // High to low, read zero, write one
        for (i = (test_size_words - 1); i >= 0 ; i--)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == 0)
            {
                start_addr[i] = CLASSB_SRAM_ALL_32BITS_HIGH;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // High to low, read one, write zero
        for (i = (test_size_words - 1); i >= 0 ; i--)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == CLASSB_SRAM_ALL_32BITS_HIGH)
            {
                start_addr[i] = 0;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // High to low, read zero
        for (i = (test_size_words - 1); i >= 0 ; i--)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val != 0)
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    return sram_march_c_result;
}

/*============================================================================
bool CLASSB_RAMMarchB(  uint32_t * start_addr, 
                        uint32_t test_size_bytes,
                        CLASSB_MEM_REGION mem_region)
------------------------------------------------------------------------------
Purpose: Runs March C algorithm on the given SRAM area
Input  : Start address, size and memory region
Output : Success or failure
Notes  : This function is used by SRAM tests. It performs the following,
        // March B
        // Low to high, write zero
        // Low to high, read zero write one, read one write zero,
               read zero write one
        // Low to high, read one write zero, write one

        // High to low, read one write zero, write one write zero
        // High to low, read zero write one, write zero
============================================================================*/
bool CLASSB_RAMMarchB(  uint32_t * start_addr, 
                        uint32_t test_size_bytes,
                        CLASSB_MEM_REGION mem_region )
{
    bool sram_march_c_result = true;
    int32_t i = 0;
    uint32_t sram_read_val = 0;
    uint32_t test_size_words = (test_size_bytes / 4);
    uint32_t test_buffer_size = 0;
    
    /* memory region switch */
    switch(mem_region)
    {
        case CLASSB_MEM_REGION_ITCM:
            test_buffer_size = CLASSB_ITCM_TEST_BUFFER_SIZE;
            break;
        case CLASSB_MEM_REGION_DTCM:
            test_buffer_size = CLASSB_DTCM_TEST_BUFFER_SIZE;
            break;
        case CLASSB_MEM_REGION_SRAM:
            test_buffer_size = CLASSB_SRAM_TEST_BUFFER_SIZE;
            break;
        default:
            sram_march_c_result = false;
            break;           
    }

    // Test size is limited to CLASSB_SRAM_TEST_BUFFER_SIZE
    if (test_size_bytes > test_buffer_size)
    {
        sram_march_c_result = false;
    }

    // Perform the next check only if the previous stage is passed
    if (sram_march_c_result == true)
    {
        // Low to high, write zero
        for (i = 0; i < test_size_words; i++)
        {
            start_addr[i] = 0;
        }
        // Low to high
        for (i = 0; i < test_size_words; i++)
        {
            // Read zero write one
            sram_read_val = start_addr[i];
            if (sram_read_val == 0)
            {
                start_addr[i] = CLASSB_SRAM_ALL_32BITS_HIGH;
                // Read one write zero
                sram_read_val = start_addr[i];
                if (sram_read_val == CLASSB_SRAM_ALL_32BITS_HIGH)
                {
                    start_addr[i] = 0;
                    // Read zero write one
                    sram_read_val = start_addr[i];
                    if (sram_read_val == 0)
                    {
                        start_addr[i] = CLASSB_SRAM_ALL_32BITS_HIGH;
                    }
                    else
                    {
                        sram_march_c_result = false;
                        break;
                    }
                }
                else
                {
                    sram_march_c_result = false;
                    break;
                }
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }

    if (sram_march_c_result == true)
    {
        // Low to high
        for (i = 0; i < test_size_words; i++)
        {
            // Read one write zero
            sram_read_val = start_addr[i];
            if (sram_read_val == CLASSB_SRAM_ALL_32BITS_HIGH)
            {
                start_addr[i] = 0;
                // Write one
                start_addr[i] = CLASSB_SRAM_ALL_32BITS_HIGH;
             }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }

    // High to low tests
    if (sram_march_c_result == true)
    {
        // High to low, read one, write zero
        for (i = (test_size_words - 1); i >= 0 ; i--)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == CLASSB_SRAM_ALL_32BITS_HIGH)
            {
                start_addr[i] = 0;
                // Write one
                start_addr[i] = CLASSB_SRAM_ALL_32BITS_HIGH;
                // Write zero
                start_addr[i] = 0;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }
    if (sram_march_c_result == true)
    {
        // High to low, read zero, write one
        for (i = (test_size_words - 1); i >= 0 ; i--)
        {
            sram_read_val = start_addr[i];
            if (sram_read_val == 0)
            {
                start_addr[i] = CLASSB_SRAM_ALL_32BITS_HIGH;
                // Write zero
                start_addr[i] = 0;
            }
            else
            {
                sram_march_c_result = false;
                break;
            }
        }
    }

    return sram_march_c_result;
}

/*============================================================================
CLASSB_TEST_STATUS CLASSB_SRAM_MarchTestInit(   uint32_t * start_addr,
                                                uint32_t test_size_bytes, 
                                                CLASSB_SRAM_MARCH_ALGO march_algo,
                                                bool running_context,
                                                CLASSB_MEM_REGION mem_region )
------------------------------------------------------------------------------
Purpose: Initialize to perform March-tests on SRAM.
Input  : Start address, size of SRAM area to be tested,
         selected algorithm and the context (startup or run-time)
         memory region to be tested
Output : Test status.
Notes  : This function uses register variables since the stack
         in SRAM also need to be tested.
============================================================================*/
CLASSB_TEST_STATUS CLASSB_SRAM_MarchTestInit(   uint32_t * start_addr,
                                                uint32_t test_size_bytes, 
                                                CLASSB_SRAM_MARCH_ALGO march_algo,
                                                bool running_context,
                                                CLASSB_MEM_REGION mem_region)
{
    /* This function uses register variables since the Stack also
     * need to be tested
     */
    // Find the last word address in the tested area
    register uint32_t march_test_end_address asm("r4") = (uint32_t)start_addr +
        test_size_bytes - 4;
    // Use a local variable for calculations
    register uint32_t mem_start_address  asm("r5") = (uint32_t)start_addr;
    register uint32_t stack_address asm("r6") = 0;
    register CLASSB_TEST_STATUS sram_init_retval asm("r8") =
        CLASSB_TEST_NOT_EXECUTED;
    register uint32_t max_march_test_end_address  asm("r9") = 0;
    register uint32_t min_mem_start_address  asm("r10") = 0;
    register uint32_t stack_pointer_address  asm("r11") = 0;

    /* memory region switch */
    switch(mem_region)
    {
        case CLASSB_MEM_REGION_ITCM:
            max_march_test_end_address = CLASSB_ITCM_FINAL_WORD_ADDRESS;
            min_mem_start_address = CLASSB_ITCM_APP_AREA_START;
            stack_pointer_address = CLASSB_ITCM_TEMP_STACK_ADDRESS;
            break;
        case CLASSB_MEM_REGION_DTCM:
            max_march_test_end_address = CLASSB_DTCM_FINAL_WORD_ADDRESS;
            min_mem_start_address = CLASSB_DTCM_APP_AREA_START;
            stack_pointer_address = CLASSB_DTCM_TEMP_STACK_ADDRESS;
            break;
        case CLASSB_MEM_REGION_SRAM:
            max_march_test_end_address = CLASSB_SRAM_FINAL_WORD_ADDRESS;
            min_mem_start_address = CLASSB_SRAM_APP_AREA_START;
            stack_pointer_address = CLASSB_SRAM_TEMP_STACK_ADDRESS;
            break;
        default:
            // error out the test
            min_mem_start_address = CLASSB_SRAM_APP_AREA_START + 10;
            break;           
    }
    
    /* The address and test size must be a multiple of 4
     * The tested area should be above the reserved SRAM for Class B library
     * Address should be within the last SRAM word address.
     */

    if ((((uint32_t)start_addr % 4) != 0)
            || ((test_size_bytes % 4) != 0)
            || (march_test_end_address > max_march_test_end_address)
            || (mem_start_address < min_mem_start_address))
    {
        ;
    }
    else
    {
        // Move stack pointer to the reserved area before any SRAM test
        stack_address = _CLASSB_GetStackPointer();
        _CLASSB_SetStackPointer(stack_pointer_address);
        if (running_context == true)
        {
            _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_RAM,
                CLASSB_TEST_INPROGRESS);
        }
        else
        {
            _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_RAM,
                CLASSB_TEST_INPROGRESS);
        }
        
        sram_init_retval = CLASSB_SRAM_MarchTest(start_addr, test_size_bytes,
            march_algo, running_context, mem_region);

        if (sram_init_retval == CLASSB_TEST_PASSED)
        {
            if (running_context == true)
            {
                _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_RAM,
                    CLASSB_TEST_PASSED);
            }
            else
            {
                _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_RAM,
                    CLASSB_TEST_PASSED);
            }
        }
        else if (sram_init_retval == CLASSB_TEST_FAILED)
        {
            if (running_context == true)
            {
                _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_RAM,
                    CLASSB_TEST_FAILED);
            }
            else
            {
                _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_RAM,
                    CLASSB_TEST_FAILED);
            }
            
            CLASSB_SelfTest_FailSafe(CLASSB_TEST_RAM);
        }
        
        _CLASSB_SetStackPointer(stack_address);
    }

    return sram_init_retval;
}

/*============================================================================
CLASSB_TEST_STATUS CLASSB_SRAM_MarchTest(   uint32_t * start_addr,
                                            uint32_t test_size_bytes, 
                                            CLASSB_SRAM_MARCH_ALGO march_algo,
                                            bool running_context,
                                            CLASSB_MEM_REGION mem_region )
------------------------------------------------------------------------------
Purpose: Perform March-tests on SRAM.
Input  : Start address, size of SRAM area to be tested,
         selected algorithm and the context (startup or run-time)
Output : Test status.
Notes  : None.
============================================================================*/
CLASSB_TEST_STATUS CLASSB_SRAM_MarchTest(   uint32_t * start_addr,
                                            uint32_t test_size_bytes, 
                                            CLASSB_SRAM_MARCH_ALGO march_algo,
                                            bool running_context,
                                            CLASSB_MEM_REGION mem_region)
{
    CLASSB_TEST_STATUS classb_sram_status = CLASSB_TEST_NOT_EXECUTED;
    bool march_test_retval = false;
    // Use a local variable for calculations
    uint32_t mem_start_address = (uint32_t)start_addr;
    // Test will be done on blocks on 512 bytes
    volatile uint32_t march_c_iterations = 0;
    // If the size is not a multiple of 512, then check the remaining area
    volatile uint32_t march_c_short_itr_size = 0;
    // Variable for loops
    int32_t i = 0;
    uint32_t * iteration_start_addr = 0;
    uint32_t classb_buff_start_add = 0;
    uint32_t classb_test_buff_size = 0;    
    
    /* memory region switch */
    switch(mem_region)
    {
        case CLASSB_MEM_REGION_ITCM:
            classb_test_buff_size = CLASSB_ITCM_TEST_BUFFER_SIZE;
            classb_buff_start_add = CLASSB_ITCM_BUFF_START_ADDRESS;
            march_c_iterations = (test_size_bytes / CLASSB_ITCM_TEST_BUFFER_SIZE);
            march_c_short_itr_size = (test_size_bytes % CLASSB_ITCM_TEST_BUFFER_SIZE);
            break;
        case CLASSB_MEM_REGION_DTCM:
            classb_test_buff_size = CLASSB_DTCM_TEST_BUFFER_SIZE;
            classb_buff_start_add = CLASSB_DTCM_BUFF_START_ADDRESS;
            march_c_iterations = (test_size_bytes / CLASSB_DTCM_TEST_BUFFER_SIZE);
            march_c_short_itr_size = (test_size_bytes % CLASSB_DTCM_TEST_BUFFER_SIZE);
            break;
        case CLASSB_MEM_REGION_SRAM:
            classb_test_buff_size = CLASSB_SRAM_TEST_BUFFER_SIZE;
            classb_buff_start_add = CLASSB_SRAM_BUFF_START_ADDRESS;
            march_c_iterations = (test_size_bytes / CLASSB_SRAM_TEST_BUFFER_SIZE);
            march_c_short_itr_size = (test_size_bytes % CLASSB_SRAM_TEST_BUFFER_SIZE);
            break;
        default:
            // error out the test
            march_c_iterations = 0;
            break;           
    }

    for (i = 0; i < march_c_iterations; i++)
    {
        iteration_start_addr = (uint32_t *)(mem_start_address + (i * classb_test_buff_size));
        // Copy the tested area
        _CLASSB_MemCopy((uint32_t *)classb_buff_start_add,
            iteration_start_addr, classb_test_buff_size);
        // Run the selected RAM March algorithm
        if (march_algo == CLASSB_SRAM_MARCH_C)
        {
            march_test_retval = CLASSB_RAMMarchC(iteration_start_addr,
                classb_test_buff_size, mem_region);
        }
        else if (march_algo == CLASSB_SRAM_MARCH_C_MINUS)
        {
            march_test_retval = CLASSB_RAMMarchCMinus(iteration_start_addr,
                classb_test_buff_size, mem_region);
        }
        else if (march_algo == CLASSB_SRAM_MARCH_B)
        {
            march_test_retval = CLASSB_RAMMarchB(iteration_start_addr,
                classb_test_buff_size, mem_region);
        }
        if (march_test_retval == false)
        {
            // If March test fails, exit the loop
            classb_sram_status = CLASSB_TEST_FAILED;
            break;
        }
        else
        {
        // Restore the tested area
        _CLASSB_MemCopy(iteration_start_addr, (uint32_t *)classb_buff_start_add,
            classb_test_buff_size);
        }
        classb_sram_status = CLASSB_TEST_PASSED;
    }

    // If the tested area is not a multiple of 512 bytes
    if ((march_c_short_itr_size > 0) && (march_test_retval == true))
    {
        iteration_start_addr = (uint32_t *)(mem_start_address + (march_c_iterations * classb_test_buff_size));
        _CLASSB_MemCopy((uint32_t *)classb_buff_start_add,
            iteration_start_addr, march_c_short_itr_size);
        // Run the selected RAM March algorithm
        if (march_algo == CLASSB_SRAM_MARCH_C)
        {
            march_test_retval = CLASSB_RAMMarchC(iteration_start_addr,
                classb_test_buff_size, mem_region);
        }
        else if (march_algo == CLASSB_SRAM_MARCH_C_MINUS)
        {
            march_test_retval = CLASSB_RAMMarchCMinus(iteration_start_addr,
                classb_test_buff_size, mem_region);
        }
        else if (march_algo == CLASSB_SRAM_MARCH_B)
        {
            march_test_retval = CLASSB_RAMMarchB(iteration_start_addr,
                classb_test_buff_size, mem_region);
        }
        if (march_test_retval == false)
        {
            classb_sram_status = CLASSB_TEST_FAILED;
        }
        else
        {
            classb_sram_status = CLASSB_TEST_PASSED;
            // Restore the tested area
            _CLASSB_MemCopy(iteration_start_addr,
                (uint32_t *)classb_buff_start_add, march_c_short_itr_size);
        }
    }

    return classb_sram_status;
}