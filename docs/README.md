# MPLAB® Harmony Class B Library for SAM E70 S70 V70 V71 devices

The Class B Library provides APIs to perform self-tests for the on-board systems of the microcontroller.

**Features Tested by the Class B Library**

Following table shows the components tested by the Class B library.

|Component|Reference \(Table H1 of IEC 60730-1\)|Fault/Error|Acceptable Measures|
|---------|-------------------------------------|-----------|-------------------|
|CPU Registers|1.1|Stuck-at|Static memory test|
|CPU Program Counter|1.3|Stuck-at|Static memory test|
|Interrupts|2|No interrupt / too frequent interrupt|Functional test|
|CPU Clock|3|Wrong frequency|Frequency monitoring|
|Flash|4.1|All single bit faults|Modified checksum|
|SRAM|4.2|DC fault|Static memory test|
|SRAM data path|5.1|Stuck-at|Static memory test|
|SRAM data path|5.2|Wrong address|Static memory test|
|Digital I/O|7.1|Abnormal operation|Input comparison or output verification|

**Class B Peripheral Library Usage**

This topic describes the basic architecture of the Class B library and provides information and examples on how to use it. APIs defined by the Class B library can be used either by the start-up code or by the application code. The application may use PLIBs, drivers or middleware from the Harmony 3 software framework along with the Class B library code.

Abstraction Model

The following picture shows positioning of Class B library in a Harmony 3 based application.

![](GUID-BD4FA7EA-2F48-49B1-8ECD-A0D6DD26E6AA-low.png)

**Start-up vs. Run-time**

The Class B library contains many self-test routines those can be executed at startup and run-time. If a self-test is executed at startup, it is called as a Start-up Self-test \(SST\) and if it is executed at run-time, then it is called a Run-time Self-test \(RST\). There are a few self-tests which can be used only as SST or as RST, such self-tests have SST or RST in the API name eg: `CLASSB_RST_IOTest()`, `CLASSB_SST_InterruptTest()`. If a self-test API does not have SST or RST in its name, then it can be used at startup as well as runtime.

**Start-up Self-test \(SST\)**

SSTs are used to test a component inside the microcontroller before it is initialized and used. When the Class B library is added via MHC, the selected SSTs are inserted into the `_on_reset()` function which is called from the `Reset_Handler()`. This means that none of the data initialization could have happened before running SSTs. So, the Class B library initializes necessary variables before using them. It is not mandatory to test all the components during startup. The SRAM can be tested partially if a faster startup is needed by the application. In this case, modify the corresponding configuration macro \(`CLASSB_SRAM_STARTUP_TEST_SIZE`\) present in `classb.h` file to change the size of the tested area.

**Run-time Self-test \(RST\)**

RSTs can be used by the application during run-time to check safe operation of different components in the microcontroller. These tests are non-destructive. In the case of run-time tests, the application shall decide which test to execute when.

**Components in the Library**

The Class B library contains self-test routines for different components inside the CPU.

![](GUID-D61B9480-4947-4B9D-8FDB-2D5A1FF963C9-low.png)

**Critical and Non-critical Components**

Based on the impact of failure, different components inside this Class B library are categorized as critical or non-critical.

If the self-test for CPU registers, PC or Flash detects a failure, the code execution is stopped, and it remains in an infinite loop. This is to prevent unsafe code execution. In the case of non-critical components, a failsafe function \(`CLASSB_SelfTest_FailSafe`\) is called when a failure is detected. This function contains a software break point and an infinite loop. Further code shall be added into this function as per the application need. The failsafe function must not return to the Class B library, since it is called due to a self-test failure. Avoid use of features which depend on the failed component. For example, if self-test for clock is failed, it is not advisable to use UART for error reporting as BAUD rate may not be accurate. In the case of SRAM failure, avoid the use of function calls or use of variables in SRAM. A simple error reporting mechanism in this case of SRAM failure can be toggling of an IO pin.

**Critical Components**

1.  CPU registers including the Program Counter

2.  Internal Flash program memory


Generic Flow of Critical Tests

![](GUID-AD12EDAD-04FD-428E-806B-C3F7A4687561-low.png)

**Non-critical Components**

1.  CPU clock

2.  IO pins

3.  Internal SRAM

4.  Interrupts


Generic Flow of Non-Critical Tests

![](GUID-49F0B095-BB12-4C40-BC92-F71C6F7BFEB7-low.png)

**Self-tests for Components in the Library**

**CPU Registers**

The ARM® Cortex®-M7 is the CPU on the ATSAME70/S70/V70/V71 devices. The Class B library checks the processor core registers for stuck-at faults. The stuck-at condition causes register bit to remain at logic 0 or logic 1. Code execution should be stopped if this error condition is detected in any of the CPU registers.

This self-test follows the register save/restore convention specified by AAPCS. It can be used at startup as well as run-time. The Program Counter \(PC\) self-test is designed as a separate test since this register cannot be checked with usual test data patterns.

Flow chart of the self-test for CPU registers

![](GUID-34919206-849C-416D-9D0A-D00F19430A9B-low.png)

**Program Counter \(PC\)**

The self-test for PC checks whether a stuck-at condition is present in the PC register. The stuck-at condition causes register bit to remain at logic 0 or logic 1. Code execution should be stopped if this error condition is detected.

The self-test for PC calls multiple functions in predefined order and verifies that each function is executed and returns the expected value. If the return values of all test functions are correct, the Program Counter is assumed to be working fine. This self-test can be used at startup as well as run-time.

Flow chart of the self-test for Program Counter \(PC\)

![](GUID-49CBDF9D-5872-4BC7-BF17-98C1E7083633-low.png)

**Flash**

The internal flash memory of the device needs to be checked for proper functionality. The self-test for internal flash performs CRC check on the internal flash memory of the device. The address range is configurable for this self-test. It runs CRC-32 algorithm with reversed representation of the polynomial 0x04C11DB7 and compares the generated checksum with the expected checksum. It uses table-based approach where the table is generated during the execution.

This self-test uses a CRC-32 generation function. This function is used inside the Class B library to generate CRC-32 of the internal Flash memory but it can be used on any contiguous memory area. The flash self-test can be used at startup as well as run-time. If this self-test is used during start up, it must be ensured that the CRC of the application area is precalculated and stored at a specific memory address which is passed as an argument for the Flash self-test. If this self-test detects a failure, it remains in an infinite loop.

Flow chart of the self-test for internal flash program memory

![](GUID-471E402C-70E3-4F93-98B3-3613B80F2DAC-low.png)

**SRAM**

Self-test for the SRAM element looks for stuck-at faults, DC faults and addressing faults with the help of RAM March algorithms. One of the input arguments to this self-test selects the algorithm. This self-test copies the data from the tested area of the SRAM into the reserved area in the SRAM and restore the data after the test. Refer to section Configuring the Library for the details on reserving the SRAM. The stack pointer is moved to the reserved area in the SRAM before running this self-test. The SRAM self-test can be used at startup as well as run-time.

It provides three standard tests to detect error conditions,

1.  March C

2.  March C minus

3.  March B


Fault Coverage for March Algorithms

|Name|Fault Coverage|
|----|--------------|
|March C|Addressing faults, Stuck-at faults, Transition faults, all coupling faults|
|March C-|Unlinked addressing faults, Stuck-at faults, Transition faults, all coupling faults|
|March B|Addressing faults, Stuck-at faults, Transition faults, Linked idempotent coupling faults, Linked inversion coupling faults|

Flow chart of the self-test for SRAM

![](GUID-7F140AD5-0EC8-4D47-8E54-383D465AB78F-low.png)

Flow chart of the internal routine for SRAM self-test

![](GUID-FBB95D23-F9E6-4CDB-BBDE-1D23D2BC9D8E-low.png)

**Clock**

The self-test for CPU clock checks whether the CPU clock frequency is within the permissible range. It uses RTT and SysTick to measure the CPU clock frequency. The RTT is clocked at 32.768 kHz from the 32 kHz External Crystal Oscillator and CPU clock can be from any other high frequency oscillator. If the CPU clock frequency is within specified error limit, it returns PASS. The test duration is defined by one of the input arguments. The clock self-test can be used at startup as well as run-time.

Note

1.  This self-test uses the RTT peripheral. Thus, if it is used during run-time, the RTT shall not be used by the application for continuous modes such as real time clock or calendar. If the RTT is used for some other purpose, it must be reconfigured after running the clock self-test.

2.  Keep the clock test duration lesser than the WDT timeout period, to avoid the WDT resetting the device.


Flow chart of the self-test for CPU clock frequency

![](GUID-C0697D99-5B8D-4563-B320-F0AEA6B9F86E-low.png)

**Interrupt**

The self-test for this element checks the interrupts functionality of the microcontroller. It configures the Nested Vectored Interrupt Controller \(NVIC\), the Real-Time Timer \(RTT\) and the Timer Counter 0 \(TC0\) peripherals to test the interrupt handling mechanism. It verifies that at least one interrupt is generated and handled properly. This self-test also checks whether the number of interrupts generated are too many within a given time period. It reports a PASS if the RTT has generated at least one interrupt and the total number of interrupts generated by the TC0 is greater than one and less than the specified upper limit. The clock used for RTT is 32.768 kHz from the internal OSCULP32K and for TC0 the clock is same as the default CPU clock \(12MHz from the MAINCK\). The interrupt self-test can be used only at startup.

Note

1.  This startup self-test utilizes the interrupts generated by RTT and TC0. For run-time testing of interrupts, a separate self-test need to be developed.


Flow chart of the self-test for interrupts

![](GUID-3489B948-6EC8-4A93-8D8B-7650E0715215-low.png)

**IO pin**

The self-test for IO pins verifies that any output pin is able to keep the configured logic state on the pin and any input pin is able to read the logic state present on the pin.

As the exact use of an IO pin is decide by the application, it is the responsibility of the application to configure the IO pin direction and drive the pin to the expected state before calling this self-test. After setting and before testing an output pin, call CLASSB\_IO\_InputSamplingEnable\(\) function to assure the approriate number of bus cycles will pass between write and read. When testing an input pin, ensure that the IO pin is externally kept at a defined logic state. The IO pin self-test can be used only at run-time.

Flow chart of the self-test for IO pins

![](GUID-4585376F-D8F7-468A-991F-CAA627901E70-low.png)

**Class B Peripheral Library - Timing of self-tests**

Peripherals other than Flash and SRAM

|Name|Time \(in miroseconds\)|
|----|-----------------------|
|CLASSB\_CPU\_RegistersTest|3.37|
|CLASSB\_CPU\_PCTest|1.53|
|CLASSB\_RST\_IOTest|3.66|
|CLASSB\_ClockTest|15600|
|CLASSB\_SST\_InterruptTest|2.114|

Flash and SRAM

|Name|Time \(in miroseconds\)|Tested size|
|----|-----------------------|-----------|
|CLASSB\_FlashCRCTest|24560|320 KB|
|CLASSB\_SRAM\_MarchTestInit|2200|512|

Note

1.  Timing is measured using onchip peripherals \(SysTick and TC\) at optimization level -O1 with CPU running at 300MHz from the internal 12MHz oscillator and PLLACK. The Interrupt Test was conducted at 12MHz during startup.Before using these self-tests in an application it is recommended to check self-test timings with the required configuration \(CPU clock, compiler optimization, memory size\).

2.  Timing measured for CLASSB\_SRAM\_MarchTestInit using the ‘March C’ algorithm.

3.  Following IDE and toolchain are used for timing measurements

    1.  MPLAB X v6.00

    2.  XC32 Compiler v4.00


**Configuring the Library \(MPLAB X\)**

This section provides details necessary to integrate the Class B library with other software components.

**Optimization Requirements**

The self-test routines provides by the Class B software has specific optimization requirements which are listed in the following table. If the optimization level for the project is different from what is listed in this table, file level optimization has to be applied as per this table.

|File|Optimization Level|
|----|------------------|
|classb\_cpu\_pc\_test.c, classb\_sram\_algorithm.c|-O0|
|All other files \(.h, .c, .S\)|-O1 or -O2 or -O3|

**Applying File Level Optimization MPLAB X**

![](GUID-4A21516F-F93E-4993-BCAD-85F131491629-low.png)

**Reserved SRAM area for the Class B library**

It is required to reserve 1kB of SRAM for exclusive use by the Class B library. This reserved SRAM must not be accessed from outside the Class B library. To check or update test results, use the corresponding interface APIs. When the Class B library is added into the project with the help of MHC, the linker setting is modified by MHC as shown below. In this example, the ATSAME70Q21B device with 384kB of SRAM is used.

`-DRAM_ORIGIN=0x20400400,-DRAM_LENGTH=0x4fc00`

When one of the allowed portions \(32kB/64kB/128kB each\) is allocated to the TCM regions, the following option must be added to the 'Additional driver options':

`-mitcm`

![](GUID-98F29F58-0CBA-4D51-AA08-BF88535C5E2B-low.png)

**Modified Startup Sequence**

When generating project with help of MPLAB Harmony 3, the startup code is present in a file named `startup_xc32`. This file contains the `Reset_Handler` which has all startup code that runs before the `main()` function. Initialization of the Class B library is done from the `_on_reset` function which is the first function to be executed from the `Reset_Handler`. The function named `CLASSB_Startup_Tests` executes all startup self-tests inserted into `classb.c` file by the MHC. If none of the self-tests are failed, this function returns `CLASSB_STARTUP_TEST_PASSED`. If any of the startup self-tests are failed, this function does not return becasue of the following reason. In the case of critical failures \(CPU registers or internal flash\), the corresponding self-test remains in an infinite loop to avoid unsafe execution of code. The self-tests for SRAM, Clock and Interrupt are considered non-critical since it may be possible to execute a fail-safe function after detecting a failure. In such case, the `CLASSB_SelfTest_FailSafe()` function is called when a failure is detected. Since the default implementation of `CLASSB_SelfTest_FailSafe` routine contains an infinite loop, it won't return to the caller.

Note

1.  The library defines the `_on_reset` function and handles some of the reset causes. The application developer shall insert functions to handle the rest of the reset causes.


**WDT Test and Timeout**

The Watchdog timer is used as a recovery mechanism in case of software failures. The Class B library enables the WDT and checks whether a WDT reset is issued if the timer is not cleared. In `CLASSB_Startup_Tests` \(in file `classb.c`\), before performing startup self-tests, the WDT timeout period is configured. It can be modified after generating the project. During startup, the device runs from 32 kHz slow internal clock. Before running any self-test which takes longer time \(SRAM or Flash test\), configuring the CPU clock at 12 MHz will help to recude the exection time. If any of these self-tests takes more time than the WDT timeout period, it results in a WDT reset. Thus, properly configuring the WDT period is essential during startup as well as runtime.

![](GUID-644F4D35-7989-440A-BA05-3A118A079CFC-low.png) ![](GUID-78AC7604-1749-41B5-9840-A0FF0ED9865A-low.png)

**Configuring Startup Tests via MHC**

Clone the `classb_sam_e70_s70_v70_v71` repo. When an MPLAB Harmony 3 project is created, the MHC lists all available components that can be added to the project. The self-tests which need to run during startup can be configured via MHC. The `Configuration Options` menu appears with a mouse click on the `Class B Library` component inside the `Project Graph`. The configurations done via MHC does not configure the library, instead it helps to modify the input arguments and to decide whether to run a specific test during startup.

**Class B Library Interface**

|Name|Description|
|----|-----------|
|**Constants Summary**| |
|CLASSB\_CLOCK\_DEFAULT\_CLOCK\_FREQ|Default CPU clock speed.|
|CLASSB\_CLOCK\_ERROR\_PERCENT|Clock error percentage selected for startup test.|
|CLASSB\_CLOCK\_MAX\_CLOCK\_FREQ|Maximum CPU clock speed.|
|CLASSB\_CLOCK\_MAX\_SYSTICK\_VAL|Upper limit of SysTick counter.|
|CLASSB\_CLOCK\_MAX\_TEST\_ACCURACY|Maximum detectable accuracy for clock self-test.|
|CLASSB\_CLOCK\_MUL\_FACTOR|Multiplication factor used in clock test.|
|CLASSB\_CLOCK\_RTC\_CLK\_FREQ|RTC clock frequency.|
|CLASSB\_CLOCK\_TEST\_RTC\_RATIO\_NS|Duration of RTC clock in nano seconds.|
|CLASSB\_CLOCK\_TEST\_RATIO\_NS\_MS|Ratio of milli second to nano second.|
|CLASSB\_COMPL\_RESULT\_ADDR|Address of one's complement test results.|
|CLASSB\_DTCM\_APP\_AREA\_START|Defines the start address of the DTCM for the application.|
|CLASSB\_DTCM\_FINAL\_WORD\_ADDRESS|Final word address in the DTCM.|
|CLASSB\_FLASH\_CRC32\_POLYNOMIAL|CRC-32 polynomial.|
|CLASSB\_INTERRUPT\_COUNT\_VAR\_ADDR|Address of the variable which keeps interrupt count.|
|CLASSB\_INTERRUPT\_TEST\_VAR\_ADDR|Address of the variable which keeps interrupt test internal status.|
|CLASSB\_INTR\_DEVICE\_VECT\_OFFSET|Defines the offset for first device specific interrupt.|
|CLASSB\_INTR\_MAX\_INT\_COUNT|Defines the upper limit for interrupt count.|
|CLASSB\_INTR\_TEST\_RTC\_COUNT|Defines the counter value for RTC peripheral.|
|CLASSB\_INTR\_TEST\_TC\_COUNT|Defines the counter value for TC0 peripheral.|
|CLASSB\_INTR\_VECTOR\_TABLE\_SIZE|Defines the size of the vector table.|
|CLASSB\_INVALID\_TEST\_ID|Invalid test ID.|
|CLASSB\_ITCM\_APP\_AREA\_START|Defines the start address of the ITCM for the application.|
|CLASSB\_ITCM\_FINAL\_WORD\_ADDRESS|Final word address in the ITCM.|
|CLASSB\_ONGOING\_TEST\_VAR\_ADDR|Address at which the ID of ongoing test is stored.|
|CLASSB\_RESULT\_ADDR|Address of test results.|
|CLASSB\_SRAM\_ALL\_32BITS\_HIGH|Defines name for max 32-bit unsigned value.|
|CLASSB\_SRAM\_APP\_AREA\_START|Defines the start address of the SRAM for the application.|
|CLASSB\_SRAM\_BUFF\_START\_ADDRESS|SRAM test buffer start address.|
|CLASSB\_SRAM\_FINAL\_WORD\_ADDRESS|Final word address in the SRAM.|
|CLASSB\_SRAM\_STARTUP\_TEST\_SIZE|Size of the SRAM tested during startup.|
|CLASSB\_SRAM\_TEST\_BUFFER\_SIZE|Defines the size of the buffer used for SRAM test.|
|CLASSB\_SRAM\_TEMP\_STACK\_ADDRESS|Address of the temporary stack.|
|CLASSB\_TEST\_IN\_PROG\_VAR\_ADDR|Address of the variable which indicates that a Class B test is in progress.|
|CLASSB\_WDT\_TEST\_IN\_PROG\_VAR\_ADDR|Address of the variable which indicates that a WDT test is in progress.|
|**Data types Summary**| |
|\*CLASSB\_SST\_RESULT\_BF|Pointer to the structure for the Class B library startup self-test result. This can be used to point to the result address 0x20000000. It will help to see the result in text form via watch window|
|\*CLASSB\_RST\_RESULT\_BF|Pointer to the structure for the Class B library run-time self-test result. This can be used to point to the result address 0x20000004. It will help to see the result in text form via watch window|
|CLASSB\_CPU\_PC\_TEST\_VALUES|Data type for PC Test input and output values.|
|CLASSB\_INIT\_STATUS|Identifies Class B initialization status.|
|CLASSB\_MEM\_REGION|Identifies the memory region to be tested.|
|CLASSB\_PORT\_INDEX|PORT index definitions for Class B library I/O pin test.|
|CLASSB\_PORT\_PIN|PIN definitions for Class B library I/O pin test.|
|CLASSB\_PORT\_PIN\_STATE|PORT pin state.|
|CLASSB\_SRAM\_MARCH\_ALGO|Selects the RAM March algorithm to run.|
|CLASSB\_STARTUP\_STATUS|Identifies startup test status.|
|CLASSB\_TEST\_ID|Identifies Class B library tests.|
|CLASSB\_TEST\_STATUS|Identifies result from Class B library test.|
|CLASSB\_TEST\_STATE|Identifies Class B library test state.|
|CLASSB\_TEST\_TYPE|Identifies type of the Class B library test.|
|**Interface Routines Summary**| |
|CLASSB\_App\_WDT\_Recovery|This function is called if a WDT reset has happened during run-time.|
|CLASSB\_CPU\_RegistersTest|This self-test checks the processor core registers.|
|CLASSB\_CPU\_PCTest|This self-test checks the Program Counter register \(PC\).|
|CLASSB\_ClearTestResults|Clears the results of SSTs or RSTs.|
|CLASSB\_ClockTest|This self-test checks whether the CPU clock frequency is within the permissible limit.|
|CLASSB\_GetTestResult|Returns the result of the specified self-test.|
|CLASSB\_FlashCRCGenerate|Generates CRC-32 checksum for a given memory area.|
|CLASSB\_FlashCRCTest|This self-test checks the internal Flash program memory to detect single bit faults.|
|CLASSB\_GlobalsInit|This function initializes the global variables for the classb library.|
|CLASSB\_IO\_InputSamplingEnable|Enable input sampling for an IO pin.|
|CLASSB\_Init|This function is executed on every device reset. This shall be called right after the reset, before any other initialization is performed.|
|CLASSB\_SelfTest\_FailSafe|This function is called if any of the non-critical tests detects a failure.|
|CLASSB\_SRAM\_MarchTestInit|This self-test checks the SRAM with the help of RAM March algorithm.|
|CLASSB\_SST\_InterruptTest|This self-test checks basic functionality of the interrupt handling mechanism.|
|CLASSB\_SST\_WDT\_Recovery|This function is called if a WDT reset has happened during the execution of an SST.|
|CLASSB\_Startup\_Tests|This function executes all startup self-tests inserted into classb.c file.|
|CLASSB\_RST\_IOTest|This self-test can be used to perform plausibility checks on IO pins.|
|CLASSB\_TestWDT|This function tests the WatchDog Timer \(WDT\).|

-   **[CLASSB\_App\_WDT\_Recovery](GUID-A1B756A6-4DE2-4F45-9B90-26331CAA883D.md)**  

-   **[CLASSB\_ClearTestResults](GUID-9CE5DABF-42B9-45D5-BCDC-B9B38835FB90.md)**  

-   **[CLASSB\_CLOCK\_DEFAULT\_CLOCK\_FREQ](GUID-30678899-D8EA-485A-8FC0-FF39676E009B.md)**  

-   **[CLASSB\_CLOCK\_ERROR\_PERCENT](GUID-FCA4B9BE-2E4E-4D4A-A41A-E126DE67BA51.md)**  

-   **[CLASSB\_CLOCK\_MAX\_CLOCK\_FREQ](GUID-836959F5-3B38-485F-807B-783B742890AA.md)**  

-   **[CLASSB\_CLOCK\_MAX\_SYSTICK\_VAL](GUID-1EC58123-4B6C-4F61-AC81-5DE7FC2EFFAD.md)**  

-   **[CLASSB\_CLOCK\_MAX\_TEST\_ACCURACY](GUID-EBEFEB1B-5B1D-4024-9065-9794094CEC38.md)**  

-   **[CLASSB\_CLOCK\_MUL\_FACTOR](GUID-A1234126-DB1F-4D0A-9EE9-F8D6935C7095.md)**  

-   **[CLASSB\_CLOCK\_RTC\_CLK\_FREQ](GUID-3D4A0B34-BA2A-45FD-A5D4-BA50577FF657.md)**  

-   **[CLASSB\_CLOCK\_TEST\_RATIO\_NS\_MS](GUID-FB4C391D-D80A-451B-8701-D4FA720E76BF.md)**  

-   **[CLASSB\_CLOCK\_TEST\_RTC\_CYCLES](GUID-B71AB403-A2F2-45AB-8D31-7BA426FDE6FD.md)**  

-   **[CLASSB\_CLOCK\_TEST\_RTC\_RATIO\_NS](GUID-AB757290-79E9-4B94-BE6E-B4C8368E113A.md)**  

-   **[CLASSB\_ClockTest](GUID-7341B826-37B0-4B20-97F9-8D1874007B44.md)**  

-   **[CLASSB\_COMPL\_RESULT\_ADDR](GUID-67B7A395-9FA1-4226-9FB8-B55DEFF444D6.md)**  

-   **[CLASSB\_CPU\_PC\_TEST\_VALUES](GUID-6B9DE9CC-FF19-4566-9E32-12BAA1A06EAE.md)**  

-   **[CLASSB\_CPU\_PCTest](GUID-7FAF1433-8EA5-4116-BEC9-1CA399EB17CB.md)**  

-   **[CLASSB\_CPU\_RegistersTest](GUID-4B5C2FC7-A0C1-49D0-89BC-DEA35C0E5CBD.md)**  

-   **[CLASSB\_DTCM\_APP\_AREA\_START](GUID-50051D1C-DE89-4AA8-B171-9A6487946AFE.md)**  

-   **[CLASSB\_DTCM\_FINAL\_WORD\_ADDRESS](GUID-AC760342-6904-4063-BCAE-1F96747EAF98.md)**  

-   **[CLASSB\_DTCM\_STARTUP\_TEST\_SIZE](GUID-A577E4ED-7150-43BB-BDC9-F0904CC88CB9.md)**  

-   **[CLASSB\_FLASH\_CRC32\_POLYNOMIAL](GUID-CD3F8BD6-A6E2-4615-94B6-A5E53C4C5A2F.md)**  

-   **[CLASSB\_FlashCRCGenerate](GUID-A83E37BC-A346-4606-BF82-D081A6A6A803.md)**  

-   **[CLASSB\_FlashCRCTest](GUID-56F1AC64-0883-4762-8FB9-AD3B02298B7D.md)**  

-   **[CLASSB\_GetTestResult](GUID-E9DE2934-438A-4E95-8D76-8368AE9110FC.md)**  

-   **[CLASSB\_GlobalsInit](GUID-8588C903-037F-43D1-B354-EC5CA3C78E9F.md)**  

-   **[CLASSB\_Init](GUID-B3B60259-6A62-4197-8B57-8CCC5467289E.md)**  

-   **[CLASSB\_INIT\_STATUS](GUID-D8178641-68D3-42B2-9FEC-3269BF0CA107.md)**  

-   **[CLASSB\_INTERRUPT\_COUNT\_VAR\_ADDR](GUID-A5DD508B-A7B3-4F54-BB5C-B760A7FB3442.md)**  

-   **[CLASSB\_INTERRUPT\_TEST\_VAR\_ADDR](GUID-5DA80DB3-4F96-42D1-8B3F-00856638BBB2.md)**  

-   **[CLASSB\_INTR\_DEVICE\_VECT\_OFFSET](GUID-706268F2-CADC-4BAB-9FD2-F4714DFF0E30.md)**  

-   **[CLASSB\_INTR\_MAX\_INT\_COUNT](GUID-61D21311-7862-4052-ADBE-8273DB7228CB.md)**  

-   **[CLASSB\_INTR\_TEST\_RTC\_COUNT](GUID-ADD6E25F-FE36-488D-A36C-EBF0F24B3236.md)**  

-   **[CLASSB\_INTR\_TEST\_TC\_COUNT](GUID-33F7065E-59A8-449E-9C2A-0BC64418B38B.md)**  

-   **[CLASSB\_INTR\_VECTOR\_TABLE\_SIZE](GUID-E10A7030-D116-45F0-A49E-8649496A7B3C.md)**  

-   **[CLASSB\_INVALID\_TEST\_ID](GUID-905BD8D9-BC1D-43C8-8959-F0DE6730B530.md)**  

-   **[CLASSB\_IO\_InputSamplingEnable](GUID-A2A9C2A3-75BE-4C02-AEE5-B18775D603EC.md)**  

-   **[CLASSB\_ITCM\_APP\_AREA\_START](GUID-BD3B7071-E1E8-428B-AAF6-42CDD00531E7.md)**  

-   **[CLASSB\_ITCM\_FINAL\_WORD\_ADDRESS](GUID-C96F6E8A-272C-4E55-83AB-D49198FCFD57.md)**  

-   **[CLASSB\_ITCM\_STARTUP\_TEST\_SIZE](GUID-A34A9617-D6EA-4EB1-8250-3607AB1D4D0D.md)**  

-   **[CLASSB\_MEM\_REGION](GUID-E03C41F9-CCB3-4351-9FBA-FE835C170010.md)**  

-   **[CLASSB\_ONGOING\_TEST\_VAR\_ADDR](GUID-42E93773-90D4-4146-A7BD-3E36D57BB9AC.md)**  

-   **[CLASSB\_PORT\_INDEX](GUID-9BACD070-3A49-4B29-B805-753EB36B45F3.md)**  

-   **[CLASSB\_PORT\_PIN](GUID-CBFD921E-2E63-46F6-8E7D-17926A9EE92F.md)**  

-   **[CLASSB\_PORT\_PIN\_STATE](GUID-DC0D9491-A928-4A46-92C4-D0428412FFE0.md)**  

-   **[CLASSB\_RESULT\_ADDR](GUID-3BFAB09B-3FB8-4096-A8C7-52208D4FE669.md)**  

-   **[CLASSB\_RST\_IOTest](GUID-8FBB5816-C44D-4010-A97F-5ED7FBA8B4F8.md)**  

-   **[CLASSB\_RST\_RESULT\_BF](GUID-0A3C3F9B-2758-48C8-98C4-8F707A4A4FA0.md)**  

-   **[CLASSB\_SelfTest\_FailSafe](GUID-07C67220-A77F-4255-B1F7-AE53D4736CEF.md)**  

-   **[CLASSB\_SRAM\_ALL\_32BITS\_HIGH](GUID-63F3143F-9D4D-45A0-9E50-8C4F54E3A1CB.md)**  

-   **[CLASSB\_SRAM\_APP\_AREA\_START](GUID-E9B5707E-C043-4842-A9B3-4AC81105D6C1.md)**  

-   **[CLASSB\_SRAM\_BUFF\_START\_ADDRESS](GUID-059EE26E-6971-4E8D-9054-3C7CE6A6257C.md)**  

-   **[CLASSB\_SRAM\_FINAL\_WORD\_ADDRESS](GUID-0596FE42-7858-48C4-ADCB-E0361BF1DBE0.md)**  

-   **[CLASSB\_SRAM\_MARCH\_ALGO](GUID-92614495-99C6-4E57-9715-A343D3F7D961.md)**  

-   **[CLASSB\_SRAM\_MarchTestInit](GUID-14EB38E8-AD4B-4A3C-A2D4-297EF780A71A.md)**  

-   **[CLASSB\_SRAM\_STARTUP\_TEST\_SIZE](GUID-617FF25F-3192-40BC-8C27-B5A3CF863114.md)**  

-   **[CLASSB\_SRAM\_TEMP\_STACK\_ADDRESS](GUID-221B5186-10CB-493C-8653-30105BDEDE53.md)**  

-   **[CLASSB\_SRAM\_TEST\_BUFFER\_SIZE](GUID-9A879EBA-3E73-4580-B960-413F7DCE3FDC.md)**  

-   **[CLASSB\_SST\_InterruptTest](GUID-60CCA062-E390-4E89-ABDF-7DC119C16E93.md)**  

-   **[CLASSB\_SST\_RESULT\_BF](GUID-5554DD7C-8186-4D9D-A6A9-A7C5FB39D0D7.md)**  

-   **[CLASSB\_SST\_WDT\_Recovery](GUID-609689B1-0232-465E-A9CC-DF393D82CAFC.md)**  

-   **[CLASSB\_STARTUP\_STATUS](GUID-3544164F-356A-4947-B45E-44967EAE3471.md)**  

-   **[CLASSB\_Startup\_Tests](GUID-82D74717-004F-426B-92B3-C8991E5EB075.md)**  

-   **[CLASSB\_TEST\_ID](GUID-06A5F6D9-9D40-4794-8044-1B98385B92B9.md)**  

-   **[CLASSB\_TEST\_IN\_PROG\_VAR\_ADDR](GUID-0E75295D-92A0-44E0-8175-347109722C3A.md)**  

-   **[CLASSB\_TEST\_STATE](GUID-F2302AEB-CC77-461C-8EC1-684F4658C1F0.md)**  

-   **[CLASSB\_TEST\_STATUS](GUID-DFC41D99-EEC8-497E-82BF-83A75CB9E6F7.md)**  

-   **[CLASSB\_TEST\_TYPE](GUID-12B5D5A5-5B4B-4844-97FC-1889846CF842.md)**  

-   **[CLASSB\_TestWDT](GUID-9CA22CBE-1FFC-47AE-8BDA-957354E6CA45.md)**  

-   **[CLASSB\_WDT\_TEST\_IN\_PROG\_VAR\_ADDR](GUID-82C948E1-D59B-47FA-8EC9-3BE5D016E536.md)**  


