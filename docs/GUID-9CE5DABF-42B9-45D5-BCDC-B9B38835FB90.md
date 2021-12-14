# CLASSB\_ClearTestResults

**Function**

```c
void CLASSB_ClearTestResults(CLASSB_TEST_TYPE test_type);
```

**Summary**

Clears the results of SSTs or RSTs.

**Description**

This function clears all the test results of a given type of test.

**Precondition**

None.

**Parameters**

*test\_type* - Can be CLASSB\_TEST\_TYPE\_SST or CLASSB\_TEST\_TYPE\_RST.

**Returns**

None.

**Example**

```c
CLASSB_ClearTestResults(CLASSB_TEST_TYPE_SST);
CLASSB_ClearTestResults(CLASSB_TEST_TYPE_RST);
```

**Remarks**

This function is called from CLASSB\_Init\(\).

**Parent topic:**[MPLAB® Harmony Class B Library for SAM E70 S70 V70 V71 devices](GUID-85C09776-46F4-43A4-9FA5-26997226A3EA.md)

