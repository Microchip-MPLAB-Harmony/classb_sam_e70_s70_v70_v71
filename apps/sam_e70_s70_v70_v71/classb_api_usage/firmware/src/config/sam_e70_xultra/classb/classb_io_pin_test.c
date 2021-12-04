/*******************************************************************************
  Class B Library v1.0.0 Release

  Company:
    Microchip Technology Inc.

  File Name:
    classb_io_pin_test.c

  Summary:
    Class B Library source file for the Clock test

  Description:
    This file provides self-test functions for the Clock.

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
#include "classb/classb_io_pin_test.h"

/*----------------------------------------------------------------------------
 *     Constants
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *     Global Variables
 *----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
 *     Functions
 *----------------------------------------------------------------------------*/
extern void _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE test_type,
    CLASSB_TEST_ID test_id, CLASSB_TEST_STATUS value);

/*============================================================================
void CLASSB_IO_InputSamplingEnable(CLASSB_PORT_INDEX port, CLASSB_PORT_PIN pin);
------------------------------------------------------------------------------
Purpose: Enable input sampling for a pin
Input  : None.
Output : None.
Notes  : Before testing an output pin, call this function to enable input
         sampling, so that the 'IN' register will have the data from the
         port pin.
============================================================================*/
void CLASSB_IO_InputSamplingEnable(CLASSB_PORT_INDEX port, CLASSB_PORT_PIN pin)
{
   // Enable input sampling
    switch(port)
    {
        case PORTA:
            PIOA_REGS->PIO_ODR = (1 << pin);
            break;
        case PORTB:
            PIOB_REGS->PIO_ODR = (1 << pin);
            break;
        case PORTC:
            PIOC_REGS->PIO_ODR = (1 << pin);
            break;
        case PORTD:
            PIOD_REGS->PIO_ODR = (1 << pin);
            break;
        case PORTE:
            PIOE_REGS->PIO_ODR = (1 << pin);
            break;
        default:
            break;
        
    }
}

/*============================================================================
CLASSB_TEST_STATUS CLASSB_RST_IOTest(CLASSB_PORT_INDEX port, CLASSB_PORT_PIN pin,
    CLASSB_PORT_PIN_STATE state);
------------------------------------------------------------------------------
Purpose: Check whether the given I/O pin is at specified state
Input  : PORT index, pin number and expected state.
Output : Test status.
Notes  : None.
============================================================================*/
CLASSB_TEST_STATUS CLASSB_RST_IOTest(CLASSB_PORT_INDEX port, CLASSB_PORT_PIN pin,
    CLASSB_PORT_PIN_STATE state)
{
    CLASSB_TEST_STATUS io_test_status = CLASSB_TEST_NOT_EXECUTED;
    CLASSB_PORT_PIN_STATE pin_read_state  = PORT_PIN_INVALID;
    uint32_t port_status = 0;

    // Check the input variable limits
    if ((port > PORTD) || (pin > PIN31))
    {
        ;
    }

    else
    {
        io_test_status = CLASSB_TEST_INPROGRESS;
        _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_IO,
                CLASSB_TEST_INPROGRESS);
        

        switch(port)
        {
            case PORTA:
                port_status = PIOA_REGS->PIO_PDSR;
                if(port_status & (1 << pin))
                    {pin_read_state = PORT_PIN_HIGH;}
                else
                    {pin_read_state = PORT_PIN_LOW;}
                break;
            case PORTB:
                port_status = PIOB_REGS->PIO_PDSR;
                if(port_status & (1 << pin))
                    {pin_read_state = PORT_PIN_HIGH;}
                else
                    {pin_read_state = PORT_PIN_LOW;}
                break;
            case PORTC:
                port_status = PIOC_REGS->PIO_PDSR;
                if(port_status & (1 << pin))
                    {pin_read_state = PORT_PIN_HIGH;}
                else
                    {pin_read_state = PORT_PIN_LOW;}
                break;
            case PORTD:
                port_status = PIOD_REGS->PIO_PDSR;
                if(port_status & (1 << pin))
                    {pin_read_state = PORT_PIN_HIGH;}
                else
                    {pin_read_state = PORT_PIN_LOW;}
                break;
            case PORTE:
                port_status = PIOE_REGS->PIO_PDSR;
                if(port_status & (1 << pin))
                    {pin_read_state = PORT_PIN_HIGH;}
                else
                    {pin_read_state = PORT_PIN_LOW;}
                break;
            default:
                break;

        }
        
        if (pin_read_state == state)
        {
            io_test_status = CLASSB_TEST_PASSED;
            _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_IO,
                CLASSB_TEST_PASSED);
        }
        else
        {
            io_test_status = CLASSB_TEST_FAILED;
            _CLASSB_UpdateTestResult(CLASSB_TEST_TYPE_RST, CLASSB_TEST_IO,
                CLASSB_TEST_FAILED);
        }
    }

    return io_test_status;
}
