/*******************************************************************************
  Class B Library v2.0.0 Release

  Company:
    Microchip Technology Inc.

  File Name:
    classb_interrupt_test.c

  Summary:
    Class B Library source file for the Interrupt test

  Description:
    This file provides self-test functions for the Interrupt.

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
#include "classb/classb_interrupt_test.h"
#include "peripheral/tc/plib_tc_common.h"
#include "peripheral/rtt/plib_rtt_common.h"

/*----------------------------------------------------------------------------
 *     Constants
 *----------------------------------------------------------------------------*/
#define CLASSB_INTR_DEVICE_VECT_OFFSET      (16U)
#define CLASSB_INTR_VECTOR_TABLE_SIZE       (CLASSB_INTR_DEVICE_VECT_OFFSET + PERIPH_MAX_IRQn)
#define CLASSB_INTR_MAX_INT_COUNT           (30U)
#define CLASSB_INTR_TEST_RTC_COUNT          (50U)
#define CLASSB_INTR_TEST_TC_COUNT           (512U)

/*----------------------------------------------------------------------------
 *     Global Variables
 *----------------------------------------------------------------------------*/
extern uint32_t __svectors;
extern volatile uint8_t * interrupt_tests_status;
extern volatile uint32_t * interrupt_count;
// Align the vector table at 1024 byte boundary
__attribute__ ((aligned (1024)))
uint32_t classb_ram_vector_table[CLASSB_INTR_VECTOR_TABLE_SIZE];
uint32_t vtor_default_value = 0;

/*----------------------------------------------------------------------------
 *     Functions
 *----------------------------------------------------------------------------*/
extern void _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE test_type,
    CLASSB_TEST_ID test_id, CLASSB_TEST_STATUS value);

/*============================================================================
static void _CLASSB_RTT_handler(void)
------------------------------------------------------------------------------
Purpose: Custom handler used for RTT Interrupt test
Input  : None.
Output : None.
Notes  : The RTC is reset after successfully performing the test.
============================================================================*/
static void _CLASSB_RTT_Handler(void)
{
    uint32_t status = RTT_REGS->RTT_SR;
	uint32_t flags = RTT_REGS->RTT_MR;
    
    /* disable RTT interupts */
	RTT_REGS->RTT_MR&= ~(RTT_MR_ALMIEN_Msk | RTT_MR_RTTINCIEN_Msk);
    
	if(flags & RTT_MR_RTTINCIEN_Msk)
	{
		if(status & RTT_SR_RTTINC_Msk)
		{
            *interrupt_tests_status = CLASSB_TEST_STARTED;
		}
	}
    
    /* Disable RTT */
    RTT_REGS->RTT_MR|= RTT_MR_RTTDIS_Msk;
    
}

/*============================================================================
static void _CLASSB_TC0_Handler(void)
------------------------------------------------------------------------------
Purpose: Custom handler used for TC Interrupt test. It clears the interrupt
         flag and updates the interrupt count variable.
Input  : None.
Output : None.
Notes  : None.
============================================================================*/
static void _CLASSB_TC0_Handler(void)
{
    /* Read SR  */
    TC_TIMER_STATUS timer_status = (TC_TIMER_STATUS)(TC0_REGS->TC_CHANNEL[0].TC_SR & TC_TIMER_STATUS_MSK);
    
    /* If flag set increment count */
    if (TC_TIMER_NONE != timer_status)
    {
        (*interrupt_count)++;
    }
   
}

/*============================================================================
static void _CLASSB_BuildVectorTable(void)
------------------------------------------------------------------------------
Purpose: Build the vector table for Interrupt self-test
Input  : None.
Output : None.
Notes  : The vector table used by this test is placed in SRAM.
============================================================================*/
static void _CLASSB_BuildVectorTable(void)
{
    uint32_t i = 0;
    uint32_t vector_start = (uint32_t)&__svectors;

    for(i = 0; i < CLASSB_INTR_VECTOR_TABLE_SIZE; i++)
    {
        // Get the interrupt handler address from the original vector table.
        classb_ram_vector_table[i] = *(uint32_t *)(vector_start + (i * 4));
    }
    // Modify the tested interrupt handler address
    classb_ram_vector_table[CLASSB_INTR_DEVICE_VECT_OFFSET + RTT_IRQn] = (uint32_t )&_CLASSB_RTT_Handler;
    classb_ram_vector_table[CLASSB_INTR_DEVICE_VECT_OFFSET + TC0_CH0_IRQn] = (uint32_t )&_CLASSB_TC0_Handler;
    vtor_default_value = SCB->VTOR;
    // Update VTOR to point to the new vector table in SRAM
    SCB->VTOR = ((uint32_t)&classb_ram_vector_table[0] & SCB_VTOR_TBLOFF_Msk);
}

/*============================================================================
static void _CLASSB_RTT_Init(void)
------------------------------------------------------------------------------
Purpose: Configure RTT peripheral for Interrupt self-test
Input  : None.
Output : None.
Notes  : The clocks required for RTC are enabled after reset. This function
         does not modify the default clocks.
============================================================================*/
static void _CLASSB_RTT_Init(void)
{
    uint32_t status = 0;

       
    // RTT Init
    RTT_REGS->RTT_MR |= RTT_MR_RTTRST_Msk;
    RTT_REGS->RTT_MR = RTT_MR_RTPRES(CLASSB_INTR_TEST_RTC_COUNT) | RTT_MR_RTTDIS_Msk | RTT_MR_ALMIEN_Msk;

    /* disable all interrupts */
    RTT_REGS->RTT_MR &= ~(RTT_PERIODIC | RTT_ALARM);

    /* clear any stale interrupts */
    status = RTT_REGS->RTT_SR;
    // unused variable error
    while(status & (~RTT_SR_Msk)){};
    
    // enable interrupt
    RTT_REGS->RTT_MR |= RTT_PERIODIC;
    
    // Enable NVIC IRQn for RTC
    NVIC_EnableIRQ(RTT_IRQn);
    
    // RTT Enable
    RTT_REGS->RTT_MR&= ~(RTT_MR_RTTDIS_Msk);
    
    // RTT Reset
    RTT_REGS->RTT_MR |= RTT_MR_RTTRST_Msk;

}

/*============================================================================
static void _CLASSB_TC0_CompareInit(void)
------------------------------------------------------------------------------
Purpose: Configure TC peripheral for Interrupt self-test
Input  : None.
Output : None.
Notes  : The TC0 is reset after successfully performing the test.
============================================================================*/
static void _CLASSB_TC0_CompareInit( void )
{
    TC_TIMER_STATUS timer_status = 0;
    

    /* Enable Peripheral Clock */
    PMC_REGS->PMC_PCER0=0x835c00;
    
    /* Use peripheral clock */
    TC0_REGS->TC_CHANNEL[0].TC_EMR = TC_EMR_NODIVCLK_Msk;
    /* clock selection and waveform selection */
    TC0_REGS->TC_CHANNEL[0].TC_CMR =  TC_CMR_WAVEFORM_WAVSEL_UP_RC | TC_CMR_WAVE_Msk ;

    /* write period */
    TC0_REGS->TC_CHANNEL[0].TC_RC = CLASSB_INTR_TEST_TC_COUNT;

    /* enable interrupt */
    TC0_REGS->TC_CHANNEL[0].TC_IER = TC_IER_CPCS_Msk;
    /* wait for sync */
    while((TC0_REGS->TC_CHANNEL[0].TC_IMR & TC_IMR_CPCS_Msk) != TC_IMR_CPCS_Msk)
    {
        // Wait for Synchronization
        ;
    }
    
    /* Clear status */
    timer_status = (TC_TIMER_STATUS)(TC0_REGS->TC_CHANNEL[0].TC_SR & TC_TIMER_STATUS_MSK);
    /* Call registered callback function */
    if (TC_TIMER_NONE != timer_status)
    {
        timer_status = timer_status;
    }
    
    /*  Enable NVIC IRQn for TC0 */
    NVIC_EnableIRQ(TC0_CH0_IRQn);
    
    /*  Start TC0 */
    TC0_REGS->TC_CHANNEL[0].TC_CCR = (TC_CCR_CLKEN_Msk | TC_CCR_SWTRG_Msk);
    
    
}

/*============================================================================
static void _CLASSB_NVIC_Init(void)
------------------------------------------------------------------------------
Purpose: Initializes the NVIC
Input  : None.
Output : None.
Notes  : None.
============================================================================*/
static void _CLASSB_NVIC_Init(void)
{
    /* Enable NVIC Controller */
    __DMB();
    __enable_irq();
}

/*============================================================================
CLASSB_TEST_STATUS CLASSB_SST_InterruptTest(void)
------------------------------------------------------------------------------
Purpose: Test interrupt generation and handling.
Input  : None.
Output : Test status.
Notes  : None.
============================================================================*/
CLASSB_TEST_STATUS CLASSB_SST_InterruptTest(void)
{
    CLASSB_TEST_STATUS intr_test_status = CLASSB_TEST_NOT_EXECUTED;

    // Reset the counter
    *interrupt_count = 0;
    _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_INTERRUPT,
        CLASSB_TEST_INPROGRESS);
    _CLASSB_BuildVectorTable();
    _CLASSB_NVIC_Init();
    _CLASSB_RTT_Init();
    _CLASSB_TC0_CompareInit();
    // Wait until the flags are updated from the interrupt handlers
    while((*interrupt_tests_status == CLASSB_TEST_NOT_STARTED))
    {
        ;
    }

    if ((*interrupt_tests_status == CLASSB_TEST_STARTED)
        &&  (*interrupt_count < CLASSB_INTR_MAX_INT_COUNT)
        &&  (*interrupt_count > 0))
    {
        intr_test_status = CLASSB_TEST_PASSED;
        _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_INTERRUPT,
            CLASSB_TEST_PASSED);


        /* Disable TC0  */
        TC0_REGS->TC_CHANNEL[0].TC_CCR = (TC_CCR_CLKDIS_Msk);
        
        /* disable interrupt */
        TC0_REGS->TC_CHANNEL[0].TC_IDR = TC_IDR_CPCS_Msk;
        
        /* wait for sync */
        while((TC0_REGS->TC_CHANNEL[0].TC_IMR & TC_IMR_CPCS_Msk) != 0)
        {
            // Wait for Synchronization
            ;
        }
                    
        // Restore SCB->VTOR
        SCB->VTOR = vtor_default_value;
    }
    else
    {
        intr_test_status = CLASSB_TEST_FAILED;
        _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_SST, CLASSB_TEST_INTERRUPT,
            CLASSB_TEST_FAILED);
        // Restore SCB->VTOR
        SCB->VTOR = vtor_default_value;
        // The failsafe function must not return.
        CLASSB_SelfTest_FailSafe(CLASSB_TEST_INTERRUPT);
    }

    return intr_test_status;
}
