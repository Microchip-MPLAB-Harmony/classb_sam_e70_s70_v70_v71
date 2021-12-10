/*******************************************************************************
  Class B Library ${REL_VER} Release

  Company:
    Microchip Technology Inc.

  File Name:
    classb.c

  Summary:
    Class B Library main source file

  Description:
    This file provides general functions for the Class B library.

*******************************************************************************/

/*******************************************************************************
* Copyright (C) ${REL_YEAR} Microchip Technology Inc. and its subsidiaries.
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
#include "classb/classb.h"

/*----------------------------------------------------------------------------
 *     Constants
 *----------------------------------------------------------------------------*/
#define CLASSB_RESULT_ADDR                  (0x${CLASSB_SRAM_START_ADDRESS}U)
#define CLASSB_COMPL_RESULT_ADDR            (0x${CLASSB_SRAM_START_MSB + "04"}U)
#define CLASSB_ONGOING_TEST_VAR_ADDR        (0x${CLASSB_SRAM_START_MSB + "08"}U)
#define CLASSB_TEST_IN_PROG_VAR_ADDR        (0x${CLASSB_SRAM_START_MSB + "0c"}U)
#define CLASSB_WDT_TEST_IN_PROG_VAR_ADDR    (0x${CLASSB_SRAM_START_MSB + "10"}U)
#define CLASSB_FLASH_TEST_VAR_ADDR          (0x${CLASSB_SRAM_START_MSB + "14"}U)
#define CLASSB_INTERRUPT_TEST_VAR_ADDR      (0x${CLASSB_SRAM_START_MSB + "18"}U)
#define CLASSB_INTERRUPT_COUNT_VAR_ADDR     (0x${CLASSB_SRAM_START_MSB + "1c"}U)
#define CLASSB_ITCM_STARTUP_TEST_SIZE       (${CLASSB_ITCM_MARCH_SIZE}U)
#define CLASSB_DTCM_STARTUP_TEST_SIZE       (${CLASSB_DTCM_MARCH_SIZE}U)
#define CLASSB_SRAM_STARTUP_TEST_SIZE       (${CLASSB_SRAM_MARCH_SIZE}U)
#define CLASSB_CLOCK_ERROR_PERCENT          (${CLASSB_CLOCK_TEST_PERCENT}U)
<#if CLASSB_FLASH_CRC_CONF?has_content>
    <#if CLASSB_FLASH_CRC_CONF == true>
#define CLASSB_FLASH_CRC32_ADDR             (0x${CLASSB_FLASHCRC_ADDR}U)
    </#if>
</#if>
#define CLASSB_CLOCK_TEST_RTC_CYCLES        (200U)
// RTC is clocked from 32768 Hz Crystal. One RTC cycle is 30517 nano sec
#define CLASSB_CLOCK_TEST_RTC_RATIO_NS      (30517U)
#define CLASSB_CLOCK_TEST_RATIO_NS_MS       (1000000U)

// Depends on Master clock setup
#define CLASSB_CLOCK_DEFAULT_CLOCK_FREQ     (12000000U)
//#define CLASSB_CLOCK_DEFAULT_CLOCK_FREQ     (300000000U)
#define CLASSB_INVALID_TEST_ID              (0xFFU)

// Master clock setup  
#define CLASSB_MASTER_CLOCK_PRESCALE    PMC_MCKR_PRES_CLK_1
#define CLASSB_MASTER_CLOCK_DIVIDE      PMC_MCKR_MDIV_PCK_DIV2
#define CLASSB_MASTER_CLOCK_SOURCE          PMC_MCKR_CSS_MAIN_CLK
//#define CLASSB_MASTER_CLOCK_SOURCE          PMC_MCKR_CSS_PLLA_CLK

/*----------------------------------------------------------------------------
 *     Global Variables
 *----------------------------------------------------------------------------*/
volatile uint8_t * ongoing_sst_id;
volatile uint8_t * classb_test_in_progress;
volatile uint8_t * wdt_test_in_progress;
volatile uint8_t * interrupt_tests_status;
volatile uint32_t * interrupt_count;

/*----------------------------------------------------------------------------
 *     Functions
 *----------------------------------------------------------------------------*/

/* ***NOTE for DTCM/ITCM - */
/* DTCM/ITCM must be set to 32kB in fuse bits and the -mitcm option must 
 * be added as an 'Additional Driver Option' in the project properties 
 * xc32-ld section.  */

/* Disable TCM memory */
__STATIC_INLINE void TCM_Disable(void);
__STATIC_INLINE void __attribute__((optimize("-O1"))) TCM_Disable(void)
{
    __DSB();
    __ISB();
    SCB->ITCMCR &= ~(uint32_t)SCB_ITCMCR_EN_Msk;
    SCB->DTCMCR &= ~(uint32_t)SCB_DTCMCR_EN_Msk;
    __DSB();
    __ISB();
}

/* Enable TCM memory */
__STATIC_INLINE void TCM_Enable(void);
__STATIC_INLINE void __attribute__((optimize("-O1"))) TCM_Enable(void)
{
    __DSB();
    __ISB();
    SCB->ITCMCR |= (uint32_t)SCB_ITCMCR_EN_Msk;
    SCB->DTCMCR |= (uint32_t)SCB_DTCMCR_EN_Msk;
    __DSB();
    __ISB();
}



/*============================================================================
static void _CLASSB_MainClockInit ( void )
------------------------------------------------------------------------------
Purpose: Initializes the Main clock
Input  : None.
Output : None.
Notes  : 
============================================================================*/
static void _CLASSB_MainClockInit(void)
{
    /* Disable Main Crystal Oscillator and Enable External Clock Signal on XIN pin  */
    PMC_REGS->CKGR_MOR = (PMC_REGS->CKGR_MOR & ~CKGR_MOR_MOSCXTEN_Msk) | CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCXTBY_Msk;

     /* External clock signal (XIN pin) is selected as the Main Clock (MAINCK) source.
        Switch Main Clock (MAINCK) to External signal on XIN pin */
    PMC_REGS->CKGR_MOR |= CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCSEL_Msk;

    /* Wait until MAINCK is switched to External Clock Signal (XIN pin) */
    while ( (PMC_REGS->PMC_SR & PMC_SR_MOSCSELS_Msk) != PMC_SR_MOSCSELS_Msk);


    /* Enable the RC Oscillator */
    PMC_REGS->CKGR_MOR|= CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCRCEN_Msk;

    /* Wait until the RC oscillator clock is ready. */
    while( (PMC_REGS->PMC_SR & PMC_SR_MOSCRCS_Msk) != PMC_SR_MOSCRCS_Msk);

    /* Configure the RC Oscillator frequency */
    PMC_REGS->CKGR_MOR = (PMC_REGS->CKGR_MOR & ~CKGR_MOR_MOSCRCF_Msk) | CKGR_MOR_KEY_PASSWD | CKGR_MOR_MOSCRCF_12_MHz;

    /* Wait until the RC oscillator clock is ready */
    while( (PMC_REGS->PMC_SR& PMC_SR_MOSCRCS_Msk) != PMC_SR_MOSCRCS_Msk);

}

/*============================================================================
static void _CLASSB_PLLAInitialize ( void )
------------------------------------------------------------------------------
Purpose: Initializes the PLLA (PLLACK)
Input  : None.
Output : None.
Notes  : 
============================================================================*/

static void _CLASSB_PLLAInitialize(void)
{
    /* Configure and Enable PLLA */
    PMC_REGS->CKGR_PLLAR = CKGR_PLLAR_ONE_Msk | CKGR_PLLAR_PLLACOUNT(0x3f) |
                              CKGR_PLLAR_MULA(25 - 1) |
                              CKGR_PLLAR_DIVA(1);

    while ( (PMC_REGS->PMC_SR & PMC_SR_LOCKA_Msk) != PMC_SR_LOCKA_Msk);

}

/*============================================================================
static void _CLASSB_MasterClockInit ( void )
------------------------------------------------------------------------------
Purpose: Sets the Master clock to use the default internal RC (12MHz). Startup
 default is the slow clock.
Input  : uint32_t pmc_mckr_pres - prescaler value
 *       uint32_t pmc_mckr_mdiv - divider value
 *       uint32_t pmc_mckr_css - master clock source
Output : None.
Notes  : Modify clock setup here for other clock test setups
============================================================================*/
static void _CLASSB_MasterClockInit ( 
    uint32_t pmc_mckr_pres,
    uint32_t pmc_mckr_mdiv,
    uint32_t pmc_mckr_css)
{
    /* Program PMC_MCKR.PRES and wait for PMC_SR.MCKRDY to be set   */
    PMC_REGS->PMC_MCKR = (PMC_REGS->PMC_MCKR & ~PMC_MCKR_PRES_Msk) | pmc_mckr_pres;
    while ((PMC_REGS->PMC_SR & PMC_SR_MCKRDY_Msk) != PMC_SR_MCKRDY_Msk);

    /* Program PMC_MCKR.MDIV and Wait for PMC_SR.MCKRDY to be set   */
    PMC_REGS->PMC_MCKR = (PMC_REGS->PMC_MCKR & ~PMC_MCKR_MDIV_Msk) | pmc_mckr_mdiv;
    while ((PMC_REGS->PMC_SR & PMC_SR_MCKRDY_Msk) != PMC_SR_MCKRDY_Msk);

    /* Program PMC_MCKR.CSS and Wait for PMC_SR.MCKRDY to be set    */
    PMC_REGS->PMC_MCKR = (PMC_REGS->PMC_MCKR & ~PMC_MCKR_CSS_Msk) | pmc_mckr_css;
    while ((PMC_REGS->PMC_SR & PMC_SR_MCKRDY_Msk) != PMC_SR_MCKRDY_Msk);
}

/*============================================================================
void CLASSB_SelfTest_FailSafe(CLASSB_TEST_ID cb_test_id)
------------------------------------------------------------------------------
Purpose: Called if a non-critical self-test is failed.
Input  : The ID of the failed test.
Output : None
Notes  : The application decides the contents of this function. This function
         should perform failsafe operation after checking the 'cb_test_id'.
         This function must not return.
============================================================================*/
void CLASSB_SelfTest_FailSafe(CLASSB_TEST_ID cb_test_id)
{
#if (defined(__DEBUG) || defined(__DEBUG_D)) && defined(__XC32)
    __builtin_software_breakpoint();
#endif
    // Infinite loop
    while (1)
    {
        ;
    }
}

/*============================================================================
static void CLASSB_GlobalsInit(void)
------------------------------------------------------------------------------
Purpose: Initialization of global variables for the classb library.
Input  : None
Output : None
Notes  : This function is called before C startup code
============================================================================*/
static void CLASSB_GlobalsInit(void)
{
    /* Initialize persistent pointers
     * These variables point to address' in the reserved SRAM for the
     * Class B library.
     */
    ongoing_sst_id = (volatile uint8_t *)CLASSB_ONGOING_TEST_VAR_ADDR;
    classb_test_in_progress = (volatile uint8_t *)CLASSB_TEST_IN_PROG_VAR_ADDR;
    wdt_test_in_progress = (volatile uint8_t *)CLASSB_WDT_TEST_IN_PROG_VAR_ADDR;
    interrupt_tests_status = (volatile uint8_t *)CLASSB_INTERRUPT_TEST_VAR_ADDR;
    interrupt_count = (volatile uint32_t *)CLASSB_INTERRUPT_COUNT_VAR_ADDR;
    
    // Initialize variables
    *ongoing_sst_id = CLASSB_INVALID_TEST_ID;
    *classb_test_in_progress = CLASSB_TEST_NOT_STARTED;
    *wdt_test_in_progress = CLASSB_TEST_NOT_STARTED;
    *interrupt_tests_status = CLASSB_TEST_NOT_STARTED;
}

/*============================================================================
static void CLASSB_App_WDT_Recovery(void)
------------------------------------------------------------------------------
Purpose: Called if a WDT reset is caused by the application
Input  : None
Output : None
Notes  : The application decides the contents of this function.
============================================================================*/
static void CLASSB_App_WDT_Recovery(void)
{
#if (defined(__DEBUG) || defined(__DEBUG_D)) && defined(__XC32)
    __builtin_software_breakpoint();
#endif
    // Infinite loop
    while (1)
    {
        ;
    }
}

/*============================================================================
static void CLASSB_SST_WDT_Recovery(void)
------------------------------------------------------------------------------
Purpose: Called after WDT reset, to indicate that a Class B function is stuck.
Input  : None
Output : None
Notes  : The application decides the contents of this function.
============================================================================*/
void CLASSB_SST_WDT_Recovery(void)
{
#if (defined(__DEBUG) || defined(__DEBUG_D)) && defined(__XC32)
    __builtin_software_breakpoint();
#endif
    // Infinite loop
    while (1)
    {
        ;
    }
}

/*============================================================================
static void CLASSB_TestWDT(void)
------------------------------------------------------------------------------
Purpose: Function to check WDT after a device reset.
Input  : None
Output : None
Notes  : None
============================================================================*/
static void CLASSB_TestWDT(void)
{
    /* This persistent flag is checked after reset */
    *wdt_test_in_progress = CLASSB_TEST_STARTED;

    // If WDT is not enabled, enable WDT and wait
    if ((WDT_REGS->WDT_MR & WDT_MR_WDRSTEN_Msk) == 0)
    {
        // Configure timeout
        WDT_REGS->WDT_MR = WDT_MR_WDD (4095) | WDT_MR_WDV(4095) \
               | WDT_MR_WDRSTEN_Msk;
        // Infinite loop
        while (1)
        {
            ;
        }
    }
    else
    {
        // Infinite loop
        while (1)
        {
            ;
        }
    }

}

/*============================================================================
static CLASSB_INIT_STATUS CLASSB_Init(void)
------------------------------------------------------------------------------
Purpose: To check reset cause and decide the startup flow.
Input  : None
Output : None
Notes  : This function is executed on every device reset. This shall be
         called right after the reset, before any other initialization is
         performed.
============================================================================*/
static CLASSB_INIT_STATUS CLASSB_Init(void)
{
    uint32_t rstc_regs_rstc_sr = RSTC_REGS->RSTC_SR;
    
    /* Initialize persistent pointers
     * These variables point to address' in the reserved SRAM for the
     * Class B library.
     */
    ongoing_sst_id = (volatile uint8_t *)CLASSB_ONGOING_TEST_VAR_ADDR;
    classb_test_in_progress = (volatile uint8_t *)CLASSB_TEST_IN_PROG_VAR_ADDR;
    wdt_test_in_progress = (volatile uint8_t *)CLASSB_WDT_TEST_IN_PROG_VAR_ADDR;
    interrupt_tests_status = (volatile uint8_t *)CLASSB_INTERRUPT_TEST_VAR_ADDR;
    interrupt_count = (volatile uint32_t *)CLASSB_INTERRUPT_COUNT_VAR_ADDR;

    CLASSB_INIT_STATUS ret_val = CLASSB_SST_NOT_DONE;

    
    if ((rstc_regs_rstc_sr & RSTC_SR_RSTTYP_Msk) == RSTC_SR_RSTTYP_WDT_RST)
    {
        if (*wdt_test_in_progress == CLASSB_TEST_STARTED)
        {
            *wdt_test_in_progress = CLASSB_TEST_NOT_STARTED;
        }
        else if (*classb_test_in_progress == CLASSB_TEST_STARTED)
        {
            CLASSB_SST_WDT_Recovery();
        }
        else
        {
            CLASSB_App_WDT_Recovery();
        }
    }
    else
    {
        /* If it is a software reset and the Class B library has issued it */
        rstc_regs_rstc_sr = RSTC_REGS->RSTC_SR;
        if ((*classb_test_in_progress == CLASSB_TEST_STARTED) &&
            ((rstc_regs_rstc_sr & RSTC_SR_RSTTYP_Msk) == RSTC_SR_RSTTYP_SOFT_RST))
        {
            *classb_test_in_progress = CLASSB_TEST_NOT_STARTED;
            ret_val = CLASSB_SST_DONE;
        }
        else
        {
            /* For all other reset causes,
             * test the reserved SRAM,
             * initialize Class B variables
             * clear the test results and test WDT
             */
            bool result_area_test_ok = false;
            bool ram_buffer_test_ok = false;
            // Test the reserved SRAM
            result_area_test_ok = CLASSB_RAMMarchC((uint32_t *)IRAM_ADDR, CLASSB_SRAM_TEST_BUFFER_SIZE);
            ram_buffer_test_ok = CLASSB_RAMMarchC((uint32_t *)IRAM_ADDR + CLASSB_SRAM_TEST_BUFFER_SIZE, CLASSB_SRAM_TEST_BUFFER_SIZE);

            if ((result_area_test_ok == true) && (ram_buffer_test_ok == true))
            {
                // Initialize all Class B variables after the March test
                CLASSB_GlobalsInit();
                CLASSB_ClearTestResults(CLASSB_TEST_TYPE_SST);
                CLASSB_ClearTestResults(CLASSB_TEST_TYPE_RST);
                // Perform WDT test
                CLASSB_TestWDT();
            }
            else
            {
                while (1)
                {
                    ;
                }
            }
        }
    }

    return ret_val;
}

/*============================================================================
static CLASSB_STARTUP_STATUS CLASSB_Startup_Tests(void)
------------------------------------------------------------------------------
Purpose: Call all startup self-tests.
Input  : None
Output : None
Notes  : This function calls all the configured self-tests during startup.
         The MPLAB Harmony Configurator (MHC) has options to configure
         the startup self-tests. If startup tests are not enabled via MHC,
         this function enables the WDT and returns CLASSB_STARTUP_TEST_NOT_EXECUTED.
============================================================================*/
static CLASSB_STARTUP_STATUS CLASSB_Startup_Tests(void)
{
    CLASSB_STARTUP_STATUS cb_startup_status = CLASSB_STARTUP_TEST_NOT_EXECUTED;
    <#if (CLASSB_CPU_TEST_OPT?? && CLASSB_CPU_TEST_OPT == true) ||
         (CLASSB_FPU_OPT?? && CLASSB_FPU_OPT == true) ||
         (CLASSB_SRAM_TEST_OPT?? && CLASSB_SRAM_TEST_OPT == true) ||
         (CLASSB_FLASH_CRC_CONF?? && CLASSB_FLASH_CRC_CONF == true) ||
         (CLASSB_CLOCK_TEST_OPT?? && CLASSB_CLOCK_TEST_OPT == true) ||
         (CLASSB_INTERRUPT_TEST_OPT?? && CLASSB_INTERRUPT_TEST_OPT == true)>
    CLASSB_STARTUP_STATUS cb_temp_startup_status = CLASSB_STARTUP_TEST_NOT_EXECUTED;
    CLASSB_TEST_STATUS cb_test_status = CLASSB_TEST_NOT_EXECUTED;
    </#if>
    <#if CLASSB_CLOCK_TEST_OPT??>
        <#if CLASSB_CLOCK_TEST_OPT == true>
            <#if CLASSB_CLOCK_TEST_DURATION?has_content>
            <#lt>    uint16_t clock_test_rtc_cycles = ((${CLASSB_CLOCK_TEST_DURATION} * CLASSB_CLOCK_TEST_RATIO_NS_MS) / CLASSB_CLOCK_TEST_RTC_RATIO_NS);
            </#if>
        </#if>
    </#if>


     /* Enable WDT */
    if ((WDT_REGS->WDT_MR & WDT_MR_WDRSTEN_Msk) == 0)
    {
        WDT_REGS->WDT_MR = WDT_MR_WDD (4095) | WDT_MR_WDV(4095) \
                   | WDT_MR_WDRSTEN_Msk;
    }

     /* Init Main Clock */
    _CLASSB_MainClockInit();

    /* Init PLL */
    _CLASSB_PLLAInitialize();
    
    /* Init Master Clock */
    _CLASSB_MasterClockInit ((uint32_t)CLASSB_MASTER_CLOCK_PRESCALE, 
                             (uint32_t)CLASSB_MASTER_CLOCK_DIVIDE, 
                             (uint32_t)CLASSB_MASTER_CLOCK_SOURCE);

    <#if CLASSB_CPU_TEST_OPT?? && CLASSB_CPU_TEST_OPT == true>
        *ongoing_sst_id = CLASSB_TEST_CPU;
        // Test processor core registers
        cb_test_status = CLASSB_CPU_RegistersTest(false);

        if (cb_test_status == CLASSB_TEST_PASSED)
        {
            cb_temp_startup_status = CLASSB_STARTUP_TEST_PASSED;
        }
        else if (cb_test_status == CLASSB_TEST_FAILED)
        {
            cb_temp_startup_status = CLASSB_STARTUP_TEST_FAILED;
        }

        // Program Counter test
        *ongoing_sst_id = CLASSB_TEST_PC;
        cb_test_status = CLASSB_CPU_PCTest(false);

        if (cb_test_status == CLASSB_TEST_PASSED)
        {
            cb_temp_startup_status = CLASSB_STARTUP_TEST_PASSED;
        }
        else if (cb_test_status == CLASSB_TEST_FAILED)
        {
            cb_temp_startup_status = CLASSB_STARTUP_TEST_FAILED;
        }
    </#if>
    <#if CLASSB_FPU_OPT?? && CLASSB_FPU_OPT == true>
        // Enable FPU
        SCB->CPACR |= (0xFu << 20);
        __DSB();
        __ISB();
        // Test FPU registers
        *ongoing_sst_id = CLASSB_TEST_FPU;
        cb_test_status = CLASSB_FPU_RegistersTest(false);
        
        if (cb_test_status == CLASSB_TEST_PASSED)
        {
            cb_temp_startup_status = CLASSB_STARTUP_TEST_PASSED;
        }
        else if (cb_test_status == CLASSB_TEST_FAILED)
        {
            cb_temp_startup_status = CLASSB_STARTUP_TEST_FAILED;
        }
    </#if>
    <#if CLASSB_SRAM_TEST_OPT??>
        <#if CLASSB_SRAM_TEST_OPT == true>

            <#lt>    // SRAM test

            <#lt>    *ongoing_sst_id = CLASSB_TEST_RAM;

            <#if CLASSB_SRAM_MARCH_ALGORITHM?has_content>
                
                    <#lt>    // Clear WDT before test
                    <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);

                    <#lt>   /* ITCM Region */
                    <#lt>    cb_test_status = CLASSB_SRAM_MarchTestInit((uint32_t *)CLASSB_ITCM_APP_AREA_START,
                    <#lt>        CLASSB_ITCM_STARTUP_TEST_SIZE, ${CLASSB_SRAM_MARCH_ALGORITHM}, false, CLASSB_MEM_REGION_ITCM);
                
                    <#lt>    // Clear WDT before test
                    <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);

                    <#lt>   /* DTCM Region */
                    <#lt>    cb_test_status = CLASSB_SRAM_MarchTestInit((uint32_t *)CLASSB_DTCM_APP_AREA_START,
                    <#lt>        CLASSB_DTCM_STARTUP_TEST_SIZE, ${CLASSB_SRAM_MARCH_ALGORITHM}, false, CLASSB_MEM_REGION_DTCM);

                    <#lt>    // Clear WDT before test
                    <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);

                    <#lt>   /* SRAM Region */
                    <#lt>    cb_test_status = CLASSB_SRAM_MarchTestInit((uint32_t *)CLASSB_SRAM_APP_AREA_START,
                    <#lt>        CLASSB_SRAM_STARTUP_TEST_SIZE, ${CLASSB_SRAM_MARCH_ALGORITHM}, false, CLASSB_MEM_REGION_SRAM);

                    <#lt>    // Clear WDT before test
                    <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);

            <#else>
                
                    <#lt>    // Clear WDT before test
                    <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);

                    <#lt>   /* ITCM Region */
                    <#lt>    cb_test_status = CLASSB_SRAM_MarchTestInit((uint32_t *)CLASSB_ITCM_APP_AREA_START,
                    <#lt>        CLASSB_ITCM_STARTUP_TEST_SIZE, CLASSB_SRAM_MARCH_C, false, CLASSB_MEM_REGION_ITCM);

                    <#lt>    // Clear WDT before test
                    <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);

                    <#lt>   /* DTCM Region */
                    <#lt>    cb_test_status = CLASSB_SRAM_MarchTestInit((uint32_t *)CLASSB_DTCM_APP_AREA_START,
                    <#lt>        CLASSB_DTCM_STARTUP_TEST_SIZE, CLASSB_SRAM_MARCH_C, false, CLASSB_MEM_REGION_DTCM);

                    <#lt>    // Clear WDT before test
                    <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);

                    <#lt>   /* SRAM Region */
                    <#lt>    cb_test_status = CLASSB_SRAM_MarchTestInit((uint32_t *)CLASSB_SRAM_APP_AREA_START,
                    <#lt>        CLASSB_SRAM_STARTUP_TEST_SIZE, CLASSB_SRAM_MARCH_C, false, CLASSB_MEM_REGION_SRAM);

                    <#lt>    // Clear WDT before test
                    <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);

            </#if>
            <#lt>    if (cb_test_status == CLASSB_TEST_PASSED)
            <#lt>    {
            <#lt>       cb_temp_startup_status = CLASSB_STARTUP_TEST_PASSED;
            <#lt>    }
            <#lt>    else if (cb_test_status == CLASSB_TEST_FAILED)
            <#lt>    {
            <#lt>       cb_temp_startup_status = CLASSB_STARTUP_TEST_FAILED;
            <#lt>    }
        </#if>
    </#if>
    <#if CLASSB_FLASH_CRC_CONF?has_content>
        <#if CLASSB_FLASH_CRC_CONF == true>

            <#lt>    // Flash Test
            <#lt>    *ongoing_sst_id = CLASSB_TEST_FLASH;
            <#lt>    // Clear WDT before test
            <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);
            <#lt>    // Flash test. Read CRC-32 and verify it
            <#lt>    cb_test_status = CLASSB_FlashCRCTest(0, CLASSB_FLASH_CRC32_ADDR,
            <#lt>       *(uint32_t *)CLASSB_FLASH_CRC32_ADDR, false);
            <#lt>    if (cb_test_status == CLASSB_TEST_PASSED)
            <#lt>    {
            <#lt>       cb_temp_startup_status = CLASSB_STARTUP_TEST_PASSED;
            <#lt>    }
            <#lt>    else if (cb_test_status == CLASSB_TEST_FAILED)
            <#lt>    {
            <#lt>       cb_temp_startup_status = CLASSB_STARTUP_TEST_FAILED;
            <#lt>    }
        </#if>
    </#if>
    <#if CLASSB_CLOCK_TEST_OPT??>
        <#if CLASSB_CLOCK_TEST_OPT == true>

            <#lt>    // Clock Test
            <#lt>    *ongoing_sst_id = CLASSB_TEST_CLOCK;
            <#lt>    // Clear WDT before test
            <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);
            <#if CLASSB_CLOCK_TEST_PERCENT?has_content && CLASSB_CLOCK_TEST_DURATION?has_content>
                <#lt>    cb_test_status = CLASSB_ClockTest(CLASSB_CLOCK_DEFAULT_CLOCK_FREQ, CLASSB_CLOCK_ERROR_PERCENT, clock_test_rtc_cycles, false);
            <#else>
                <#lt>    cb_test_status = CLASSB_ClockTest(CLASSB_CLOCK_DEFAULT_CLOCK_FREQ, CLASSB_CLOCK_ERROR_PERCENT, CLASSB_CLOCK_TEST_RTC_CYCLES, false);
            </#if>
            <#lt>    if (cb_test_status == CLASSB_TEST_PASSED)
            <#lt>    {
            <#lt>        cb_temp_startup_status = CLASSB_STARTUP_TEST_PASSED;
            <#lt>    }
            <#lt>    else if (cb_test_status == CLASSB_TEST_FAILED)
            <#lt>    {
            <#lt>        cb_temp_startup_status = CLASSB_STARTUP_TEST_FAILED;
            <#lt>    }
        </#if>
    </#if>
    <#if CLASSB_INTERRUPT_TEST_OPT??>
        <#if CLASSB_INTERRUPT_TEST_OPT == true>

            <#lt>    // Interrupt Test
            <#lt>    *ongoing_sst_id = CLASSB_TEST_INTERRUPT;
            <#lt>    // Clear WDT before test
            <#lt>    WDT_REGS->WDT_CR = (WDT_CR_KEY_PASSWD | WDT_CR_WDRSTT_Msk);
            <#lt>    cb_test_status = CLASSB_SST_InterruptTest();
            <#lt>    if (cb_test_status == CLASSB_TEST_PASSED)
            <#lt>    {
            <#lt>        cb_temp_startup_status = CLASSB_STARTUP_TEST_PASSED;
            <#lt>    }
            <#lt>    else if (cb_test_status == CLASSB_TEST_FAILED)
            <#lt>    {
            <#lt>        cb_temp_startup_status = CLASSB_STARTUP_TEST_FAILED;
            <#lt>    }
        </#if>
    </#if>
    <#if (CLASSB_CPU_TEST_OPT?? && CLASSB_CPU_TEST_OPT == true) ||
         (CLASSB_FPU_OPT?? && CLASSB_FPU_OPT == true) ||
         (CLASSB_SRAM_TEST_OPT?? && CLASSB_SRAM_TEST_OPT == true) ||
         (CLASSB_FLASH_CRC_CONF?? && CLASSB_FLASH_CRC_CONF == true) ||
         (CLASSB_CLOCK_TEST_OPT?? && CLASSB_CLOCK_TEST_OPT == true) ||
         (CLASSB_INTERRUPT_TEST_OPT?? && CLASSB_INTERRUPT_TEST_OPT == true)>
    if (cb_temp_startup_status == CLASSB_STARTUP_TEST_PASSED)
    {
        cb_startup_status = CLASSB_STARTUP_TEST_PASSED;
    }
    else
    {
        cb_startup_status = CLASSB_STARTUP_TEST_FAILED;
    }
    </#if>
    return cb_startup_status;
}

/*============================================================================
void _on_reset(void)
------------------------------------------------------------------------------
Purpose: Handle reset causes and perform start-up self-tests.
Input  : None
Output : None
Notes  : This function is called from Reset_Handler.
============================================================================*/
void _on_reset(void)
{
    CLASSB_STARTUP_STATUS startup_tests_status = CLASSB_STARTUP_TEST_FAILED;

    /* Enable ITCM/DTCM memory */
    TCM_Enable();

    CLASSB_INIT_STATUS init_status = CLASSB_Init();

    if (init_status == CLASSB_SST_NOT_DONE)
    {
        *classb_test_in_progress = CLASSB_TEST_STARTED;
        // Run all startup self-tests
        startup_tests_status = CLASSB_Startup_Tests();

        if (startup_tests_status == CLASSB_STARTUP_TEST_PASSED)
        {
            // Reset the device if all tests are passed.
            NVIC_SystemReset();
        }
        else if (startup_tests_status == CLASSB_STARTUP_TEST_FAILED)
        {
#if (defined(__DEBUG) || defined(__DEBUG_D)) && defined(__XC32)
            __builtin_software_breakpoint();
#endif
            // Infinite loop
            while (1)
            {
                ;
            }
        }
        else
        {
            // If startup tests are not enabled via MHC, do nothing.
            ;
        }
    }
    else if (init_status == CLASSB_SST_DONE)
    {
        // Clear flags
        *classb_test_in_progress = CLASSB_TEST_NOT_STARTED;
    }
}
