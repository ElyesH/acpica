
/******************************************************************************
 *
 * Module Name: exmonad - ACPI AML execution for monadic (1 operand) operators
 *              $Revision: 1.115 $
 *
 *****************************************************************************/

/******************************************************************************
 *
 * 1. Copyright Notice
 *
 * Some or all of this work - Copyright (c) 1999, 2000, 2001, Intel Corp.
 * All rights reserved.
 *
 * 2. License
 *
 * 2.1. This is your license from Intel Corp. under its intellectual property
 * rights.  You may have additional license terms from the party that provided
 * you this software, covering your right to use that party's intellectual
 * property rights.
 *
 * 2.2. Intel grants, free of charge, to any person ("Licensee") obtaining a
 * copy of the source code appearing in this file ("Covered Code") an
 * irrevocable, perpetual, worldwide license under Intel's copyrights in the
 * base code distributed originally by Intel ("Original Intel Code") to copy,
 * make derivatives, distribute, use and display any portion of the Covered
 * Code in any form, with the right to sublicense such rights; and
 *
 * 2.3. Intel grants Licensee a non-exclusive and non-transferable patent
 * license (with the right to sublicense), under only those claims of Intel
 * patents that are infringed by the Original Intel Code, to make, use, sell,
 * offer to sell, and import the Covered Code and derivative works thereof
 * solely to the minimum extent necessary to exercise the above copyright
 * license, and in no event shall the patent license extend to any additions
 * to or modifications of the Original Intel Code.  No other license or right
 * is granted directly or by implication, estoppel or otherwise;
 *
 * The above copyright and patent license is granted only if the following
 * conditions are met:
 *
 * 3. Conditions
 *
 * 3.1. Redistribution of Source with Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification with rights to further distribute source must include
 * the above Copyright Notice, the above License, this list of Conditions,
 * and the following Disclaimer and Export Compliance provision.  In addition,
 * Licensee must cause all Covered Code to which Licensee contributes to
 * contain a file documenting the changes Licensee made to create that Covered
 * Code and the date of any change.  Licensee must include in that file the
 * documentation of any changes made by any predecessor Licensee.  Licensee
 * must include a prominent statement that the modification is derived,
 * directly or indirectly, from Original Intel Code.
 *
 * 3.2. Redistribution of Source with no Rights to Further Distribute Source.
 * Redistribution of source code of any substantial portion of the Covered
 * Code or modification without rights to further distribute source must
 * include the following Disclaimer and Export Compliance provision in the
 * documentation and/or other materials provided with distribution.  In
 * addition, Licensee may not authorize further sublicense of source of any
 * portion of the Covered Code, and must include terms to the effect that the
 * license from Licensee to its licensee is limited to the intellectual
 * property embodied in the software Licensee provides to its licensee, and
 * not to intellectual property embodied in modifications its licensee may
 * make.
 *
 * 3.3. Redistribution of Executable. Redistribution in executable form of any
 * substantial portion of the Covered Code or modification must reproduce the
 * above Copyright Notice, and the following Disclaimer and Export Compliance
 * provision in the documentation and/or other materials provided with the
 * distribution.
 *
 * 3.4. Intel retains all right, title, and interest in and to the Original
 * Intel Code.
 *
 * 3.5. Neither the name Intel nor any other trademark owned or controlled by
 * Intel shall be used in advertising or otherwise to promote the sale, use or
 * other dealings in products derived from or relating to the Covered Code
 * without prior written authorization from Intel.
 *
 * 4. Disclaimer and Export Compliance
 *
 * 4.1. INTEL MAKES NO WARRANTY OF ANY KIND REGARDING ANY SOFTWARE PROVIDED
 * HERE.  ANY SOFTWARE ORIGINATING FROM INTEL OR DERIVED FROM INTEL SOFTWARE
 * IS PROVIDED "AS IS," AND INTEL WILL NOT PROVIDE ANY SUPPORT,  ASSISTANCE,
 * INSTALLATION, TRAINING OR OTHER SERVICES.  INTEL WILL NOT PROVIDE ANY
 * UPDATES, ENHANCEMENTS OR EXTENSIONS.  INTEL SPECIFICALLY DISCLAIMS ANY
 * IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGEMENT AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 * 4.2. IN NO EVENT SHALL INTEL HAVE ANY LIABILITY TO LICENSEE, ITS LICENSEES
 * OR ANY OTHER THIRD PARTY, FOR ANY LOST PROFITS, LOST DATA, LOSS OF USE OR
 * COSTS OF PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES, OR FOR ANY INDIRECT,
 * SPECIAL OR CONSEQUENTIAL DAMAGES ARISING OUT OF THIS AGREEMENT, UNDER ANY
 * CAUSE OF ACTION OR THEORY OF LIABILITY, AND IRRESPECTIVE OF WHETHER INTEL
 * HAS ADVANCE NOTICE OF THE POSSIBILITY OF SUCH DAMAGES.  THESE LIMITATIONS
 * SHALL APPLY NOTWITHSTANDING THE FAILURE OF THE ESSENTIAL PURPOSE OF ANY
 * LIMITED REMEDY.
 *
 * 4.3. Licensee shall not export, either directly or indirectly, any of this
 * software or system incorporating such software without first obtaining any
 * required license or other approval from the U. S. Department of Commerce or
 * any other agency or department of the United States Government.  In the
 * event Licensee exports any such software from the United States or
 * re-exports any such software from a foreign destination, Licensee shall
 * ensure that the distribution and export/re-export of the software is in
 * compliance with all laws, regulations, orders, or other restrictions of the
 * U.S. Export Administration Regulations. Licensee agrees that neither it nor
 * any of its subsidiaries will export/re-export any technical data, process,
 * software, or service, directly or indirectly, to any country for which the
 * United States government or any agency thereof requires an export license,
 * other governmental approval, or letter of assurance, without first obtaining
 * such license, approval or letter.
 *
 *****************************************************************************/

#define __EXMONAD_C__

#include "acpi.h"
#include "acparser.h"
#include "acdispat.h"
#include "acinterp.h"
#include "amlcode.h"
#include "acnamesp.h"


#define _COMPONENT          ACPI_EXECUTER
        MODULE_NAME         ("exmonad")


/*******************************************************************************
 *
 * FUNCTION:    AcpiExGetObjectReference
 *
 * PARAMETERS:  ObjDesc         - Create a reference to this object
 *              RetDesc         - Where to store the reference
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Obtain and return a "reference" to the target object
 *              Common code for the RefOfOp and the CondRefOfOp.
 *
 ******************************************************************************/

static ACPI_STATUS
AcpiExGetObjectReference (
    ACPI_OPERAND_OBJECT     *ObjDesc,
    ACPI_OPERAND_OBJECT     **RetDesc,
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;


    FUNCTION_TRACE_PTR ("ExGetObjectReference", ObjDesc);


    if (VALID_DESCRIPTOR_TYPE (ObjDesc, ACPI_DESC_TYPE_INTERNAL))
    {
        if (ObjDesc->Common.Type != INTERNAL_TYPE_REFERENCE)
        {
            *RetDesc = NULL;
            Status = AE_TYPE;
            goto Cleanup;
        }

        /*
         * Not a Name -- an indirect name pointer would have
         * been converted to a direct name pointer in AcpiExResolveOperands
         */
        switch (ObjDesc->Reference.Opcode)
        {
        case AML_LOCAL_OP:
        case AML_ARG_OP:

            *RetDesc = (void *) AcpiDsMethodDataGetNode (ObjDesc->Reference.Opcode,
                                        ObjDesc->Reference.Offset, WalkState);
            break;

        default:

            ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "(Internal) Unknown Ref subtype %02x\n",
                ObjDesc->Reference.Opcode));
            *RetDesc = NULL;
            Status = AE_AML_INTERNAL;
            goto Cleanup;
        }

    }

    else if (VALID_DESCRIPTOR_TYPE (ObjDesc, ACPI_DESC_TYPE_NAMED))
    {
        /* Must be a named object;  Just return the Node */

        *RetDesc = ObjDesc;
    }

    else
    {
        *RetDesc = NULL;
        Status = AE_TYPE;
    }


Cleanup:

    ACPI_DEBUG_PRINT ((ACPI_DB_EXEC, "Obj=%p Ref=%p\n", ObjDesc, *RetDesc));
    return_ACPI_STATUS (Status);
}

#define ObjDesc             Operand[0]
#define ResDesc             Operand[1]


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_1A_0T_0R
 *
 * PARAMETERS:  WalkState           - Current state (contains AML opcode)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute Type 1 monadic operator with numeric operand on
 *              object stack
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_1A_0T_0R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_STATUS             Status;


    FUNCTION_TRACE_PTR ("ExOpcode_1A_0T_0R", WALK_OPERANDS);


    /* Examine the opcode */

    switch (WalkState->Opcode)  
    {
    case AML_RELEASE_OP:    /*  Release (MutexObject) */

        Status = AcpiExReleaseMutex (ObjDesc, WalkState);
        break;


    case AML_RESET_OP:      /*  Reset (EventObject) */

        Status = AcpiExSystemResetEvent (ObjDesc);
        break;


    case AML_SIGNAL_OP:     /*  Signal (EventObject) */

        Status = AcpiExSystemSignalEvent (ObjDesc);
        break;


    case AML_SLEEP_OP:      /*  Sleep (MsecTime) */

        AcpiExSystemDoSuspend ((UINT32) ObjDesc->Integer.Value);
        break;


    case AML_STALL_OP:      /*  Stall (UsecTime) */

        AcpiExSystemDoStall ((UINT32) ObjDesc->Integer.Value);
        break;


    default:                /*  Unknown opcode  */

        REPORT_ERROR (("AcpiExOpcode_1A_0T_0R: Unknown opcode %X\n",
            WalkState->Opcode));
        Status = AE_AML_BAD_OPCODE;
        break;
    }


    /* Always delete the operand */

    AcpiUtRemoveReference (ObjDesc);

    return_ACPI_STATUS (AE_OK);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_1A_1T_1R
 *
 * PARAMETERS:  WalkState           - Current state (contains AML opcode)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with one argument, one target, and a
 *              return value.
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_1A_1T_1R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_STATUS             Status = AE_OK;
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *RetDesc = NULL;
    ACPI_OPERAND_OBJECT     *RetDesc2 = NULL;
    UINT32                  Temp32;
    UINT32                  i;
    UINT32                  j;
    ACPI_INTEGER            Digit;


    FUNCTION_TRACE_PTR ("ExOpcode_1A_1T_1R", WALK_OPERANDS);




    /* Create a return object of type Integer for most opcodes */

    switch (WalkState->Opcode)
    {
    case AML_BIT_NOT_OP:
    case AML_FIND_SET_LEFT_BIT_OP:
    case AML_FIND_SET_RIGHT_BIT_OP:
    case AML_FROM_BCD_OP:
    case AML_TO_BCD_OP:
    case AML_COND_REF_OF_OP:

        RetDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!RetDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        break;
    }


    switch (WalkState->Opcode)
    {

    case AML_BIT_NOT_OP:            /* Not (Operand, Result)  */

        RetDesc->Integer.Value = ~ObjDesc->Integer.Value;
        break;


    case AML_FIND_SET_LEFT_BIT_OP:  /* FindSetLeftBit (Operand, Result) */


        RetDesc->Integer.Value = ObjDesc->Integer.Value;

        /*
         * Acpi specification describes Integer type as a little
         * endian unsigned value, so this boundary condition is valid.
         */
        for (Temp32 = 0; RetDesc->Integer.Value && Temp32 < ACPI_INTEGER_BIT_SIZE; ++Temp32)
        {
            RetDesc->Integer.Value >>= 1;
        }

        RetDesc->Integer.Value = Temp32;
        break;


    case AML_FIND_SET_RIGHT_BIT_OP: /* FindSetRightBit (Operand, Result)  */


        RetDesc->Integer.Value = ObjDesc->Integer.Value;

        /*
         * The Acpi specification describes Integer type as a little
         * endian unsigned value, so this boundary condition is valid.
         */
        for (Temp32 = 0; RetDesc->Integer.Value && Temp32 < ACPI_INTEGER_BIT_SIZE; ++Temp32)
        {
            RetDesc->Integer.Value <<= 1;
        }

        /* Since the bit position is one-based, subtract from 33 (65) */

        RetDesc->Integer.Value = Temp32 == 0 ? 0 : (ACPI_INTEGER_BIT_SIZE + 1) - Temp32;
        break;


    case AML_FROM_BCD_OP:           /* FromBcd (BCDValue, Result)  */

        /*
         * The 64-bit ACPI integer can hold 16 4-bit BCD integers
         */
        RetDesc->Integer.Value = 0;
        for (i = 0; i < ACPI_MAX_BCD_DIGITS; i++)
        {
            /* Get one BCD digit */

            Digit = (ACPI_INTEGER) ((ObjDesc->Integer.Value >> (i * 4)) & 0xF);

            /* Check the range of the digit */

            if (Digit > 9)
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "BCD digit too large: \n",
                    Digit));
                Status = AE_AML_NUMERIC_OVERFLOW;
                goto Cleanup;
            }

            if (Digit > 0)
            {
                /* Sum into the result with the appropriate power of 10 */

                for (j = 0; j < i; j++)
                {
                    Digit *= 10;
                }

                RetDesc->Integer.Value += Digit;
            }
        }
        break;


    case AML_TO_BCD_OP:             /* ToBcd (Operand, Result)  */

        if (ObjDesc->Integer.Value > ACPI_MAX_BCD_VALUE)
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "BCD overflow: %d\n",
                ObjDesc->Integer.Value));
            Status = AE_AML_NUMERIC_OVERFLOW;
            goto Cleanup;
        }

        RetDesc->Integer.Value = 0;
        for (i = 0; i < ACPI_MAX_BCD_DIGITS; i++)
        {
            /* Divide by nth factor of 10 */

            Temp32 = 0;
            Digit = ObjDesc->Integer.Value;
            for (j = 0; j < i; j++)
            {
                AcpiUtShortDivide (&Digit, 10, &Digit, &Temp32);
            }

            /* Create the BCD digit from the remainder above */

            if (Digit > 0)
            {
                RetDesc->Integer.Value += (Temp32 << (i * 4));
            }
        }
        break;


    case AML_COND_REF_OF_OP:        /* CondRefOf (SourceObject, Result)  */

        /*
         * This op is a little strange because the internal return value is
         * different than the return value stored in the result descriptor
         * (There are really two return values)
         */
        if ((ACPI_NAMESPACE_NODE *) ObjDesc == AcpiGbl_RootNode)
        {
            /*
             * This means that the object does not exist in the namespace,
             * return FALSE
             */
            RetDesc->Integer.Value = 0;

            /*
             * Must delete the result descriptor since there is no reference
             * being returned
             */
            AcpiUtRemoveReference (ResDesc);
            goto Cleanup;
        }

        /* Get the object reference and store it */

        Status = AcpiExGetObjectReference (ObjDesc, &RetDesc2, WalkState);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }

        Status = AcpiExStore (RetDesc2, ResDesc, WalkState);

        /* The object exists in the namespace, return TRUE */

        RetDesc->Integer.Value = ACPI_INTEGER_MAX;
        goto Cleanup;
        break;


    case AML_STORE_OP:              /* Store (Source, Target) */

        /*
         * A store operand is typically a number, string, buffer or lvalue
         * Be careful about deleting the source object,
         * since the object itself may have been stored.
         */
        Status = AcpiExStore (ObjDesc, ResDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            /* On failure, just delete the ObjDesc */

            AcpiUtRemoveReference (ObjDesc);
            return_ACPI_STATUS (Status);
        }

        /*
         * Normally, we would remove a reference on the ObjDesc parameter;
         * But since it is being used as the internal return object
         * (meaning we would normally increment it), the two cancel out,
         * and we simply don't do anything.
         */
        WalkState->ResultObj = ObjDesc;
        return_ACPI_STATUS (Status);
        break;


    /*
     * ACPI 2.0 Opcodes
     */
    case AML_TO_DECSTRING_OP:       /* ToDecimalString (Data, Result) */

        Status = AcpiExConvertToString (ObjDesc, &RetDesc, 10, ACPI_UINT32_MAX, WalkState);
        break;


    case AML_TO_HEXSTRING_OP:       /* ToHexString (Data, Result) */

        Status = AcpiExConvertToString (ObjDesc, &RetDesc, 16, ACPI_UINT32_MAX, WalkState);
        break;


    case AML_TO_BUFFER_OP:          /* ToBuffer (Data, Result) */

        Status = AcpiExConvertToBuffer (ObjDesc, &RetDesc, WalkState);
        break;


    case AML_TO_INTEGER_OP:         /* ToInteger (Data, Result) */

        Status = AcpiExConvertToInteger (ObjDesc, &RetDesc, WalkState);
        break;


    /*
     * These are two obsolete opcodes
     */
    case AML_SHIFT_LEFT_BIT_OP:     /*  ShiftLeftBit (Source, BitNum)  */
    case AML_SHIFT_RIGHT_BIT_OP:    /*  ShiftRightBit (Source, BitNum) */


        ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "%s is obsolete and not implemented\n",
                        AcpiPsGetOpcodeName (WalkState->Opcode)));
        Status = AE_SUPPORT;
        goto Cleanup;
        break;


    default:                        /* Unknown opcode */

        REPORT_ERROR (("AcpiExOpcode_1A_1T_1R: Unknown opcode %X\n",
            WalkState->Opcode));
        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
    }


    /*
     * Store the return value computed above into the result object 
     */
    Status = AcpiExStore (RetDesc, ResDesc, WalkState);


Cleanup:
    /* Always delete the operand object */

    AcpiUtRemoveReference (ObjDesc);

    /* Delete return object(s) on error */

    if (ACPI_FAILURE (Status))
    {
        AcpiUtRemoveReference (ResDesc);     /* Result descriptor */
        if (RetDesc)
        {
            AcpiUtRemoveReference (RetDesc);
            RetDesc = NULL;
        }
    }

    /* Set the return object and exit */

    WalkState->ResultObj = RetDesc;
    return_ACPI_STATUS (Status);
}


/*******************************************************************************
 *
 * FUNCTION:    AcpiExOpcode_1A_0T_1R
 *
 * PARAMETERS:  WalkState           - Current state (contains AML opcode)
 *
 * RETURN:      Status
 *
 * DESCRIPTION: Execute opcode with one argument, no target, and a return value
 *
 ******************************************************************************/

ACPI_STATUS
AcpiExOpcode_1A_0T_1R (
    ACPI_WALK_STATE         *WalkState)
{
    ACPI_OPERAND_OBJECT     **Operand = &WalkState->Operands[0];
    ACPI_OPERAND_OBJECT     *TmpDesc;
    ACPI_OPERAND_OBJECT     *RetDesc = NULL;
    ACPI_STATUS             Status = AE_OK;
    UINT32                  Type;
    ACPI_INTEGER            Value;


    FUNCTION_TRACE_PTR ("ExOpcode_1A_0T_1R", WALK_OPERANDS);




    /* Get the operand and decode the opcode */

    switch (WalkState->Opcode)
    {

    case AML_LNOT_OP:               /* LNot (Operand) */

        RetDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!RetDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        RetDesc->Integer.Value = !ObjDesc->Integer.Value;
        break;


    case AML_DECREMENT_OP:          /* Decrement (Operand)  */
    case AML_INCREMENT_OP:          /* Increment (Operand)  */

        /*
         * Since we are expecting a Reference operand, it
         * can be either an Node or an internal object.
         *
         * TBD: [Future] This may be the prototype code for all cases where
         * a Reference is expected!! 10/99
         */
        if (VALID_DESCRIPTOR_TYPE (ObjDesc, ACPI_DESC_TYPE_NAMED))
        {
            RetDesc = ObjDesc;
        }

        else
        {
            /*
             * Duplicate the Reference in a new object so that we can resolve it
             * without destroying the original Reference object
             */
            RetDesc = AcpiUtCreateInternalObject (INTERNAL_TYPE_REFERENCE);
            if (!RetDesc)
            {
                Status = AE_NO_MEMORY;
                goto Cleanup;
            }

            RetDesc->Reference.Opcode = ObjDesc->Reference.Opcode;
            RetDesc->Reference.Offset = ObjDesc->Reference.Offset;
            RetDesc->Reference.Object = ObjDesc->Reference.Object;
        }


        /*
         * Convert the RetDesc Reference to a Number
         * (This deletes the original RetDesc object)
         */
        Status = AcpiExResolveOperands (AML_LNOT_OP, &RetDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "%s: bad operand(s) %s\n",
                AcpiPsGetOpcodeName (WalkState->Opcode), AcpiFormatException(Status)));

            goto Cleanup;
        }

        /* Do the actual increment or decrement */

        if (AML_INCREMENT_OP == WalkState->Opcode)
        {
            RetDesc->Integer.Value++;
        }
        else
        {
            RetDesc->Integer.Value--;
        }

        /* Store the result back in the original descriptor */

        Status = AcpiExStore (RetDesc, ObjDesc, WalkState);

        /* Objdesc was just deleted (because it is an Reference) */

        ObjDesc = NULL;

        break;


    case AML_TYPE_OP:               /* ObjectType (SourceObject) */

        if (INTERNAL_TYPE_REFERENCE == ObjDesc->Common.Type)
        {
            /*
             * Not a Name -- an indirect name pointer would have
             * been converted to a direct name pointer in ResolveOperands
             */
            switch (ObjDesc->Reference.Opcode)
            {
            case AML_ZERO_OP:
            case AML_ONE_OP:
            case AML_ONES_OP:
            case AML_REVISION_OP:

                /* Constants are of type Integer */

                Type = ACPI_TYPE_INTEGER;
                break;


            case AML_DEBUG_OP:

                /* Per 1.0b spec, Debug object is of type "DebugObject" */

                Type = ACPI_TYPE_DEBUG_OBJECT;
                break;


            case AML_INDEX_OP:

                /* Get the type of this reference (index into another object) */

                Type = ObjDesc->Reference.TargetType;
                if (Type == ACPI_TYPE_PACKAGE)
                {
                    /*
                     * The main object is a package, we want to get the type
                     * of the individual package element that is referenced by
                     * the index.
                     */
                    Type = (*(ObjDesc->Reference.Where))->Common.Type;
                }

                break;


            case AML_LOCAL_OP:
            case AML_ARG_OP:

                Type = AcpiDsMethodDataGetType (ObjDesc->Reference.Opcode,
                                ObjDesc->Reference.Offset, WalkState);
                break;


            default:

                REPORT_ERROR (("AcpiExOpcode_1A_0T_1R/TypeOp: Internal error - Unknown Reference subtype %X\n",
                    ObjDesc->Reference.Opcode));
                Status = AE_AML_INTERNAL;
                goto Cleanup;
            }
        }

        else
        {
            /*
             * It's not a Reference, so it must be a direct name pointer.
             */
            Type = AcpiNsGetType ((ACPI_NAMESPACE_NODE *) ObjDesc);

            /* Convert internal types to external types */

            switch (Type)
            {
            case INTERNAL_TYPE_REGION_FIELD:
            case INTERNAL_TYPE_BANK_FIELD:
            case INTERNAL_TYPE_INDEX_FIELD:

                Type = ACPI_TYPE_FIELD_UNIT;
            }

        }

        /* Allocate a descriptor to hold the type. */

        RetDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!RetDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        RetDesc->Integer.Value = Type;
        break;


    case AML_SIZE_OF_OP:            /* SizeOf (SourceObject)  */

        TmpDesc = ObjDesc;
        if (VALID_DESCRIPTOR_TYPE (ObjDesc, ACPI_DESC_TYPE_NAMED))
        {
            TmpDesc = AcpiNsGetAttachedObject ((ACPI_NAMESPACE_NODE *) ObjDesc);
        }

        if (!TmpDesc)
        {
            Value = 0;
        }

        else
        {
            switch (TmpDesc->Common.Type)
            {
            case ACPI_TYPE_BUFFER:
                Value = TmpDesc->Buffer.Length;
                break;

            case ACPI_TYPE_STRING:
                Value = TmpDesc->String.Length;
                break;

            case ACPI_TYPE_PACKAGE:
                Value = TmpDesc->Package.Count;
                break;

            case INTERNAL_TYPE_REFERENCE:

                /* TBD: this must be a ref to a buf/str/pkg?? */

                Value = 4;
                break;

            default:
                ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "Not Buf/Str/Pkg - found type %X\n",
                    TmpDesc->Common.Type));
                Status = AE_AML_OPERAND_TYPE;
                goto Cleanup;
            }
        }

        /*
         * Now that we have the size of the object, create a result
         * object to hold the value
         */
        RetDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
        if (!RetDesc)
        {
            Status = AE_NO_MEMORY;
            goto Cleanup;
        }

        RetDesc->Integer.Value = Value;
        break;


    case AML_REF_OF_OP:             /* RefOf (SourceObject) */

        Status = AcpiExGetObjectReference (ObjDesc, &RetDesc, WalkState);
        if (ACPI_FAILURE (Status))
        {
            goto Cleanup;
        }
        break;


    case AML_DEREF_OF_OP:           /* DerefOf (ObjReference) */

        /* Check for a method local or argument */

        if (!VALID_DESCRIPTOR_TYPE (ObjDesc, ACPI_DESC_TYPE_NAMED))
        {
            /*
             * Must resolve/dereference the local/arg reference first
             */
            switch (ObjDesc->Reference.Opcode)
            {
            /* Set ObjDesc to the value of the local/arg */

            case AML_LOCAL_OP:
            case AML_ARG_OP:

                AcpiDsMethodDataGetValue (ObjDesc->Reference.Opcode,
                        ObjDesc->Reference.Offset, WalkState, &TmpDesc);

                /*
                 * Delete our reference to the input object and
                 * point to the object just retrieved
                 */
                AcpiUtRemoveReference (ObjDesc);
                ObjDesc = TmpDesc;
                break;

            default:

                /* Index op - handled below */
                break;
            }
        }


        /* ObjDesc may have changed from the code above */

        if (VALID_DESCRIPTOR_TYPE (ObjDesc, ACPI_DESC_TYPE_NAMED))
        {
            /* Get the actual object from the Node (This is the dereference) */

            RetDesc = ((ACPI_NAMESPACE_NODE *) ObjDesc)->Object;

            /* Returning a pointer to the object, add another reference! */

            AcpiUtAddReference (RetDesc);
        }

        else
        {
            /*
             * This must be a reference object produced by the Index
             * ASL operation -- check internal opcode
             */
            if ((ObjDesc->Reference.Opcode != AML_INDEX_OP) &&
                (ObjDesc->Reference.Opcode != AML_REF_OF_OP))
            {
                ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "Unknown opcode in ref(%p) - %X\n",
                    ObjDesc, ObjDesc->Reference.Opcode));

                Status = AE_TYPE;
                goto Cleanup;
            }


            switch (ObjDesc->Reference.Opcode)
            {
            case AML_INDEX_OP:

                /*
                 * Supported target types for the Index operator are
                 * 1) A Buffer
                 * 2) A Package
                 */
                if (ObjDesc->Reference.TargetType == ACPI_TYPE_BUFFER_FIELD)
                {
                    /*
                     * The target is a buffer, we must create a new object that
                     * contains one element of the buffer, the element pointed
                     * to by the index.
                     *
                     * NOTE: index into a buffer is NOT a pointer to a
                     * sub-buffer of the main buffer, it is only a pointer to a
                     * single element (byte) of the buffer!
                     */
                    RetDesc = AcpiUtCreateInternalObject (ACPI_TYPE_INTEGER);
                    if (!RetDesc)
                    {
                        Status = AE_NO_MEMORY;
                        goto Cleanup;
                    }

                    TmpDesc = ObjDesc->Reference.Object;
                    RetDesc->Integer.Value =
                        TmpDesc->Buffer.Pointer[ObjDesc->Reference.Offset];

                    /* TBD: [Investigate] (see below) Don't add an additional
                     * ref!
                     */
                }

                else if (ObjDesc->Reference.TargetType == ACPI_TYPE_PACKAGE)
                {
                    /*
                     * The target is a package, we want to return the referenced
                     * element of the package.  We must add another reference to
                     * this object, however.
                     */
                    RetDesc = *(ObjDesc->Reference.Where);
                    if (!RetDesc)
                    {
                        /*
                         * We can't return a NULL dereferenced value.  This is
                         * an uninitialized package element and is thus a
                         * severe error.
                         */

                        ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "NULL package element obj %p\n",
                            ObjDesc));
                        Status = AE_AML_UNINITIALIZED_ELEMENT;
                        goto Cleanup;
                    }

                    AcpiUtAddReference (RetDesc);
                }

                else
                {
                    ACPI_DEBUG_PRINT ((ACPI_DB_ERROR, "Unknown TargetType %X in obj %p\n",
                        ObjDesc->Reference.TargetType, ObjDesc));
                    Status = AE_AML_OPERAND_TYPE;
                    goto Cleanup;
                }

                break;


            case AML_REF_OF_OP:

                RetDesc = ObjDesc->Reference.Object;

                /* Add another reference to the object! */

                AcpiUtAddReference (RetDesc);
                break;
            }
        }

        break;


    default:

        REPORT_ERROR (("AcpiExOpcode_1A_0T_1R: Unknown opcode %X\n",
            WalkState->Opcode));
        Status = AE_AML_BAD_OPCODE;
        goto Cleanup;
    }


Cleanup:

    if (ObjDesc)
    {
        AcpiUtRemoveReference (ObjDesc);
    }

    /* Delete return object on error */

    if (ACPI_FAILURE (Status) &&
        (RetDesc))
    {
        AcpiUtRemoveReference (RetDesc);
        RetDesc = NULL;
    }

    WalkState->ResultObj = RetDesc;
    return_ACPI_STATUS (Status);
}

