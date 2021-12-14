# CLASSB\_CPU\_PC\_TEST\_VALUES

**Summary**

Data type for PC Test input and output values.

**Description**

The PC tests performs logical left-shift of the input value and returns it. Values from this enum can be used as arguments.

**Remarks**

None.

```c
typedef enum classb_pc_test_val
{
CLASSB_CPU_PC_TEST_ROUTINE_A_INPUT = 1U,
CLASSB_CPU_PC_ROUTINE_A_RET_VAL = 2U,
CLASSB_CPU_PC_ROUTINE_B_RET_VAL = 4U,
CLASSB_CPU_PC_ROUTINE_C_RET_VAL = 8U,
CLASSB_CPU_PC_TEST_INIT_VAL = 0U
} CLASSB_CPU_PC_TEST_VALUES;
```

**Parent topic:**[MPLAB® Harmony Class B Library for SAM E70 S70 V70 V71 devices](GUID-85C09776-46F4-43A4-9FA5-26997226A3EA.md)

## Interface Routines

