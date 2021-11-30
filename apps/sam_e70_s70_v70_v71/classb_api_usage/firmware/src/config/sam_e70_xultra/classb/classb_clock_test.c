/*******************************************************************************
  Class B Library v1.0.0 Release

  Company:
    Microchip Technology Inc.

  File Name:
    classb_clock_test.c

  Summary:
    Class B Library CPU clock frequency self-test source file

  Description:
    This file provides CPU clock frequency self-test.

*******************************************************************************/

/*******************************************************************************
* Copyright (C) 2021 Microchip Technology Inc. and its subsidiaries.
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
#include "classb/classb_clock_test.h"

/*----------------------------------------------------------------------------
 *     Constants
 *----------------------------------------------------------------------------*/
#define CLASSB_CLOCK_MAX_CLOCK_FREQ         (300000000U)
#define CLASSB_CLOCK_MAX_SYSTICK_VAL        (0xffffffU)
#define CLASSB_CLOCK_RTC_CLK_FREQ           (32768U)
#define CLASSB_CLOCK_MAX_TEST_ACCURACY      (5U)
/* Since no floating point is used for clock test, multiply intermediate
 * values with 128.
 */
#define CLASSB_CLOCK_MUL_FACTOR             (128U)

/*----------------------------------------------------------------------------
 *     Global Variables
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *     Functions
 *----------------------------------------------------------------------------*/
extern void _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE test_type,
    CLASSB_TEST_ID test_id, CLASSB_TEST_STATUS value);

/*============================================================================
static uint32_t _CLASSB_Clock_SysTickGetVal(void)
------------------------------------------------------------------------------
Purpose: Reads the VAL register of the SysTick
Input  : None.
Output : None.
Notes  : None.
============================================================================*/
static uint32_t _CLASSB_Clock_SysTickGetVal ( void )
{
	return (SysTick->VAL);
}

/*============================================================================
static void _CLASSB_Clock_SysTickStart(void)
------------------------------------------------------------------------------
Purpose: Configure the SysTick for clock self-test
Input  : None.
Output : None.
Notes  : If SysTick is used by the application, ensure that it
         is reconfigured after the clock self test.
============================================================================*/
static void _CLASSB_Clock_SysTickStart ( void )
{
	SysTick->LOAD = CLASSB_CLOCK_MAX_SYSTICK_VAL;
    SysTick->VAL = 0;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
}

/*============================================================================
static void _CLASSB_Clock_RTT_Enable(void)
------------------------------------------------------------------------------
Purpose: Enables the RTT
Input  : None.
Output : None.
Notes  : None.
============================================================================*/
static void _CLASSB_Clock_RTT_Enable ( void )
{
    uint32_t status = 0;
    
    RTT_REGS->RTT_MR|= RTT_MR_RTTRST_Msk;
    RTT_REGS->RTT_MR&= ~(RTT_MR_RTTDIS_Msk);
    
    // read status register to clear flag
    status = RTT_REGS->RTT_SR & RTT_SR_Msk;
    // unused variable error
    while(status & (~RTT_SR_Msk)){};
    
}

/*============================================================================
static void _CLASSB_Clock_RTT_ClockInit(void)
------------------------------------------------------------------------------
Purpose: Configure clocks for the RTT peripheral
Input  : None.
Output : None.
Notes  : This self-test configures RTC to use an external
         32.768kHz Crystal as reference clock. Do not use this self-test
         if the external crystal is not available.
============================================================================*/
static void _CLASSB_Clock_RTT_ClockInit(void)
{
 
    /* External clock signal on XIN32 pin is selected as the Slow Clock (SLCK) source.
       Bypass 32K Crystal Oscillator  */
    SUPC_REGS->SUPC_MR |= SUPC_MR_KEY_PASSWD | SUPC_MR_OSCBYPASS_BYPASS;
    SUPC_REGS->SUPC_CR |= SUPC_CR_KEY_PASSWD | SUPC_CR_XTALSEL_CRYSTAL_SEL;

    /* Wait until the external clock signal is ready and
       Slow Clock (SLCK) is switched to external clock signal */
    while (!(SUPC_REGS->SUPC_SR & SUPC_SR_OSCSEL_Msk))
    {
    }
    
}

/*============================================================================
static void _CLASSB_Clock_RTT_Init(void)
------------------------------------------------------------------------------
Purpose: Configure RTT peripheral for CPU clock self-test
Input  : None.
Output : None.
Notes  : The clocks required for RTC are configured in a separate function.
         ** - 32,768 >= test_cycles >= 3
============================================================================*/
static void _CLASSB_Clock_RTT_Init(uint32_t test_cycles)
{
      
    // RTT Init
    RTT_REGS->RTT_MR = RTT_MR_RTTRST_Msk;
    RTT_REGS->RTT_MR = RTT_MR_RTPRES(test_cycles) | RTT_MR_RTTDIS_Msk | RTT_MR_ALMIEN_Msk;
    
}

/*============================================================================
CLASSB_TEST_STATUS CLASSB_ClockTest(uint32_t cpu_clock_freq,
    uint8_t error_limit,
    uint8_t clock_test_rtc_cycles,
    bool running_context);
------------------------------------------------------------------------------
Purpose: Check whether CPU clock frequency is within acceptable limits.
Input  : Expected CPU clock frequency value, acceptable error percentage,
         test duration (in RTC cycles) and running context.
Output : Test status.
Notes  : None.
============================================================================*/
CLASSB_TEST_STATUS CLASSB_ClockTest(uint32_t cpu_clock_freq,
    uint8_t error_limit,
    uint16_t clock_test_rtc_cycles,
    bool running_context)
{
    CLASSB_TEST_STATUS clock_test_status = CLASSB_TEST_NOT_EXECUTED;
    int64_t expected_ticks = ((cpu_clock_freq / CLASSB_CLOCK_RTC_CLK_FREQ) * clock_test_rtc_cycles);
    volatile uint32_t systick_count_a = 0;
    volatile uint32_t systick_count_b = 0;
    int64_t ticks_passed = 0;
    uint8_t calculated_error_limit = 0;

    if ((expected_ticks > CLASSB_CLOCK_MAX_SYSTICK_VAL)
        || (cpu_clock_freq > CLASSB_CLOCK_MAX_CLOCK_FREQ)
        || (error_limit < CLASSB_CLOCK_MAX_TEST_ACCURACY))
    {
        ;
    }
    else
    {
        if (running_context == true)
        {
            _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_CLOCK,
                CLASSB_TEST_INPROGRESS);
        }
        else
        {
            _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_CLOCK,
                CLASSB_TEST_INPROGRESS);
        }
      
        _CLASSB_Clock_RTT_ClockInit();
        _CLASSB_Clock_RTT_Init(clock_test_rtc_cycles);
        _CLASSB_Clock_SysTickStart();
        _CLASSB_Clock_RTT_Enable();

        systick_count_a = _CLASSB_Clock_SysTickGetVal();
        /* Wait for RTT Time lag to be set */
        while(!((RTT_REGS->RTT_SR & RTT_SR_RTTINC_Msk) == RTT_SR_RTTINC_Msk))
        {
            ;
        }
        systick_count_b = _CLASSB_Clock_SysTickGetVal();

        expected_ticks = expected_ticks * CLASSB_CLOCK_MUL_FACTOR;
        ticks_passed = (systick_count_a - systick_count_b) * CLASSB_CLOCK_MUL_FACTOR;

        if (ticks_passed < expected_ticks)
        {
            // The CPU clock is slower than expected
            calculated_error_limit = ((((expected_ticks - ticks_passed) * CLASSB_CLOCK_MUL_FACTOR)/ (expected_ticks)) * 100) / CLASSB_CLOCK_MUL_FACTOR;
        }
        else
        {
            // The CPU clock is faster than expected
            calculated_error_limit = ((((ticks_passed - expected_ticks) * CLASSB_CLOCK_MUL_FACTOR)/ (expected_ticks)) * 100) / CLASSB_CLOCK_MUL_FACTOR;
        }

        if (error_limit > calculated_error_limit)
        {
            clock_test_status = CLASSB_TEST_PASSED;
            if (running_context == true)
            {
                _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_CLOCK,
                    CLASSB_TEST_PASSED);
            }
            else
            {
                _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_CLOCK,
                    CLASSB_TEST_PASSED);
            }
        }
        else
        {
            clock_test_status = CLASSB_TEST_FAILED;
            if (running_context == true)
            {
                _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_CLOCK,
                    CLASSB_TEST_FAILED);
            }
            else
            {
                _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_CLOCK,
                    CLASSB_TEST_FAILED);
            }
            CLASSB_SelfTest_FailSafe(CLASSB_TEST_CLOCK);
        }
    }

    return clock_test_status;
}
