/*--------------------------------------------------------------------------
*
*   Copyright (C) Cyclades Corporation, 1997-2001.
*   All rights reserved.
*
*   Cyclades-Z Port Driver
*	
*   This file:      cyzwrite.c
*
*   Description:    This module contains the code related to write
*                   operations in the Cyclades-Z Port driver.
*
*   Notes:          This code supports Windows 2000 and Windows XP,
*                   x86 and IA64 processors.
*
*   Complies with Cyclades SW Coding Standard rev 1.3.
*
*--------------------------------------------------------------------------
*/

/*-------------------------------------------------------------------------
*
*   Change History
*
*--------------------------------------------------------------------------
*
*
*--------------------------------------------------------------------------
*/

#include "precomp.h"


BOOLEAN
CyzGiveWriteToIsr(
    IN PVOID Context
    );

VOID
CyzCancelCurrentWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
CyzGrabWriteFromIsr(
    IN PVOID Context
    );

BOOLEAN
CyzGrabXoffFromIsr(
    IN PVOID Context
    );

VOID
CyzCancelCurrentXoff(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    );

BOOLEAN
CyzGiveXoffToIsr(
    IN PVOID Context
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGESER,CyzProcessEmptyTransmit)
#pragma alloc_text(PAGESER,CyzWrite)
#pragma alloc_text(PAGESER,CyzStartWrite)
#pragma alloc_text(PAGESER,CyzGetNextWrite)
#pragma alloc_text(PAGESER,CyzGiveWriteToIsr)
#pragma alloc_text(PAGESER,CyzCancelCurrentWrite)
#pragma alloc_text(PAGESER,CyzGrabWriteFromIsr)
#pragma alloc_text(PAGESER,CyzGrabXoffFromIsr)
#pragma alloc_text(PAGESER,CyzCancelCurrentXoff)
#pragma alloc_text(PAGESER,CyzGiveXoffToIsr)
#endif


NTSTATUS
CyzWrite(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )
/*--------------------------------------------------------------------------
    CyzWrite()
    
    Routine Description: This is the dispatch routine for write. It
    validates the parameters for the write request and if all is ok
    then it places the request on the work queue.

    Arguments:

    DeviceObject - Pointer to the device object for this device
    Irp - Pointer to the IRP for the current request

    Return Value: If the io is zero length then it will return STATUS_SUCCESS,
    otherwise this routine will return STATUS_PENDING.
--------------------------------------------------------------------------*/
{

    PCYZ_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    NTSTATUS status;

    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzWrite(%X, %X)\n", DeviceObject,
                  Irp);

    LOGENTRY(LOG_MISC, ZSIG_WRITE, 
                       Extension->PortIndex+1,
                       IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length, 
                       0);


    if ((status = CyzIRPPrologue(Irp, Extension)) != STATUS_SUCCESS) {
      CyzCompleteRequest(Extension, Irp, IO_NO_INCREMENT);
      CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzWrite (1) %X\n", status);
      return status;
    }

    CyzDbgPrintEx(CYZIRPPATH, "Dispatch entry for: %x\n", Irp);

    if (CyzCompleteIfError(DeviceObject,Irp) != STATUS_SUCCESS) {
       CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzWrite (2) %X\n",
                     STATUS_CANCELLED);
       return STATUS_CANCELLED;
    }

    Irp->IoStatus.Information = 0L;

    //
    // Quick check for a zero length write.  If it is zero length
    // then we are already done!
    //

    if (IoGetCurrentIrpStackLocation(Irp)->Parameters.Write.Length) {

        //
        // Well it looks like we actually have to do some
        // work.  Put the write on the queue so that we can
        // process it when our previous writes are done.
        //

       
       status = CyzStartOrQueue(Extension, Irp, &Extension->WriteQueue,
                                   &Extension->CurrentWriteIrp,
                                   CyzStartWrite);

       CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzWrite (3) %X\n", status);

       return status;

    } else {

        Irp->IoStatus.Status = STATUS_SUCCESS;

        CyzCompleteRequest(Extension, Irp, 0);

        CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzWrite (4) %X\n",
                      STATUS_SUCCESS);

        return STATUS_SUCCESS;

    }

}

NTSTATUS
CyzStartWrite(
    IN PCYZ_DEVICE_EXTENSION Extension
    )
/*--------------------------------------------------------------------------
    CyzStartWrite()

    Routine Description: This routine is used to start off any write.
    It initializes the Iostatus fields of the irp.  It will set up any
    timers that are used to control the write.

    Arguments:

    Extension - Points to the serial device extension

    Return Value: This routine will return STATUS_PENDING for all writes
    other than those that we find are cancelled.
--------------------------------------------------------------------------*/
{
    PIRP NewIrp;
    KIRQL OldIrql;
    #ifdef POLL
    KIRQL pollIrql;
    #endif
    LARGE_INTEGER TotalTime;
    BOOLEAN UseATimer;
    SERIAL_TIMEOUTS Timeouts;
    BOOLEAN SetFirstStatus = FALSE;
    NTSTATUS FirstStatus;

    CYZ_LOCKED_PAGED_CODE();

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzStartWrite(%X)\n", Extension);

//    LOGENTRY(LOG_MISC, ZSIG_START_WRITE, 
//                       Extension->PortIndex+1,
//                       0, 
//                       0);

    do {
        // If there is an xoff counter then complete it.
        IoAcquireCancelSpinLock(&OldIrql);

        // We see if there is a actually an Xoff counter irp.
        //
        // If there is, we put the write irp back on the head
        // of the write list.  We then kill the xoff counter.
        // The xoff counter killing code will actually make the
        // xoff counter back into the current write irp, and
        // in the course of completing the xoff (which is now
        // the current write) we will restart this irp.

        if (Extension->CurrentXoffIrp) {
            if (SERIAL_REFERENCE_COUNT(Extension->CurrentXoffIrp)) {
                // The reference count is non-zero.  This implies that
                // the xoff irp has not made it through the completion
                // path yet.  We will increment the reference count
                // and attempt to complete it ourseleves.

                SERIAL_SET_REFERENCE(Extension->CurrentXoffIrp,
						SERIAL_REF_XOFF_REF);

                Extension->CurrentXoffIrp->IoStatus.Information = 0; // Added in build 2128

                // The following call will actually release the
                // cancel spin lock.

                CyzTryToCompleteCurrent(
                    Extension,
                    CyzGrabXoffFromIsr,
                    OldIrql,
                    STATUS_SERIAL_MORE_WRITES,
                    &Extension->CurrentXoffIrp,
                    NULL,
                    NULL,
                    &Extension->XoffCountTimer,
                    NULL,
                    NULL,
                    SERIAL_REF_XOFF_REF
                    );
            } else {
                // The irp is well on its way to being finished.
                // We can let the regular completion code do the
                // work.  Just release the spin lock.
                IoReleaseCancelSpinLock(OldIrql);
            }
        } else {
            IoReleaseCancelSpinLock(OldIrql);
        }

        UseATimer = FALSE;

        // Calculate the timeout value needed for the
        // request.  Note that the values stored in the
        // timeout record are in milliseconds.  Note that
        // if the timeout values are zero then we won't start
        // the timer.

        KeAcquireSpinLock(&Extension->ControlLock,&OldIrql);

        Timeouts = Extension->Timeouts;

        KeReleaseSpinLock(&Extension->ControlLock,OldIrql);

        if (Timeouts.WriteTotalTimeoutConstant ||
            Timeouts.WriteTotalTimeoutMultiplier) {

            PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation(
                                           Extension->CurrentWriteIrp
                                           );
            UseATimer = TRUE;

            // We have some timer values to calculate.
            // Take care, we might have an xoff counter masquerading
            // as a write.
            TotalTime.QuadPart =
                ((LONGLONG)((UInt32x32To64(
                                 (IrpSp->MajorFunction == IRP_MJ_WRITE)?
                                     (IrpSp->Parameters.Write.Length):
                                     (1),
                                 Timeouts.WriteTotalTimeoutMultiplier
                                 )
                                 + Timeouts.WriteTotalTimeoutConstant)))
                * -10000;
        }

        // The irp may be going to the isr shortly.  Now
        // is a good time to initialize its reference counts.

        SERIAL_INIT_REFERENCE(Extension->CurrentWriteIrp);

        // We need to see if this irp should be canceled.

        IoAcquireCancelSpinLock(&OldIrql);
        if (Extension->CurrentWriteIrp->Cancel) {
            IoReleaseCancelSpinLock(OldIrql);
            Extension->CurrentWriteIrp->IoStatus.Status = STATUS_CANCELLED;

            if (!SetFirstStatus) {
                FirstStatus = STATUS_CANCELLED;
                SetFirstStatus = TRUE;
            }
        } else {
            if (!SetFirstStatus) {
                // If we haven't set our first status, then
                // this is the only irp that could have possibly
                // not been on the queue.  (It could have been
                // on the queue if this routine is being invoked
                // from the completion routine.)  Since this
                // irp might never have been on the queue we
                // should mark it as pending.

                IoMarkIrpPending(Extension->CurrentWriteIrp);
                SetFirstStatus = TRUE;
                FirstStatus = STATUS_PENDING;
            }

            // We give the irp to to the isr to write out.
            // We set a cancel routine that knows how to
            // grab the current write away from the isr.
            //
            // Since the cancel routine has an implicit reference
            // to this irp up the reference count.

            IoSetCancelRoutine(
                Extension->CurrentWriteIrp,
                CyzCancelCurrentWrite
                );

            SERIAL_SET_REFERENCE(Extension->CurrentWriteIrp,SERIAL_REF_CANCEL);

            if (UseATimer) {
                CyzSetTimer(
                    &Extension->WriteRequestTotalTimer,
                    TotalTime,
                    &Extension->TotalWriteTimeoutDpc,
                    Extension
                    );

                // This timer now has a reference to the irp.

                SERIAL_SET_REFERENCE(
                    Extension->CurrentWriteIrp,
                    SERIAL_REF_TOTAL_TIMER
                    );
            }

            #ifdef POLL			
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzGiveWriteToIsr(Extension);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzGiveWriteToIsr,
                Extension
                );
            #endif

            IoReleaseCancelSpinLock(OldIrql);
            break;
        }

        // Well the write was canceled before we could start it up.
        // Try to get another.

        CyzGetNextWrite(&Extension->CurrentWriteIrp, &Extension->WriteQueue,
                        &NewIrp, TRUE, Extension);

    } while (NewIrp);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzStartWrite %X\n", FirstStatus);

    return FirstStatus;
}

VOID
CyzGetNextWrite(
    IN PIRP *CurrentOpIrp,
    IN PLIST_ENTRY QueueToProcess,
    IN PIRP *NewIrp,
    IN BOOLEAN CompleteCurrent,
    IN PCYZ_DEVICE_EXTENSION Extension
    )

/*++

Routine Description:

    This routine completes the old write as well as getting
    a pointer to the next write.

    The reason that we have have pointers to the current write
    queue as well as the current write irp is so that this
    routine may be used in the common completion code for
    read and write.

Arguments:

    CurrentOpIrp - Pointer to the pointer that points to the
                   current write irp.

    QueueToProcess - Pointer to the write queue.

    NewIrp - A pointer to a pointer to the irp that will be the
             current irp.  Note that this could end up pointing
             to a null pointer.  This does NOT necessaryly mean
             that there is no current write.  What could occur
             is that while the cancel lock is held the write
             queue ended up being empty, but as soon as we release
             the cancel spin lock a new irp came in from
             CyzStartWrite.

    CompleteCurrent - Flag indicates whether the CurrentOpIrp should
                      be completed.

Return Value:

    None.

--*/

{
   CYZ_LOCKED_PAGED_CODE();

   CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzGetNextWrite(XXX)\n");


    do {


        //
        // We could be completing a flush.
        //

        if (IoGetCurrentIrpStackLocation(*CurrentOpIrp)->MajorFunction
            == IRP_MJ_WRITE) {

            KIRQL OldIrql;

            ASSERT(Extension->TotalCharsQueued >=
                   (IoGetCurrentIrpStackLocation(*CurrentOpIrp)
                    ->Parameters.Write.Length));

            IoAcquireCancelSpinLock(&OldIrql);
            Extension->TotalCharsQueued -=
                IoGetCurrentIrpStackLocation(*CurrentOpIrp)
                ->Parameters.Write.Length;
            IoReleaseCancelSpinLock(OldIrql);

        } else if (IoGetCurrentIrpStackLocation(*CurrentOpIrp)->MajorFunction
                   == IRP_MJ_DEVICE_CONTROL) {

            KIRQL OldIrql;
            #ifdef POLL
            KIRQL pollIrql;
            #endif
            PIRP Irp;
            PSERIAL_XOFF_COUNTER Xc;

            IoAcquireCancelSpinLock(&OldIrql);

            Irp = *CurrentOpIrp;
            Xc = Irp->AssociatedIrp.SystemBuffer;

            //
            // We should never have a xoff counter when we
            // get to this point.
            //

            ASSERT(!Extension->CurrentXoffIrp);

            //
            // We absolutely shouldn't have a cancel routine
            // at this point.
            //

            ASSERT(!Irp->CancelRoutine);

            //
            // This could only be a xoff counter masquerading as
            // a write irp.
            //

            Extension->TotalCharsQueued--;

            //
            // Check to see of the xoff irp has been set with success.
            // This means that the write completed normally.  If that
            // is the case, and it hasn't been set to cancel in the
            // meanwhile, then go on and make it the CurrentXoffIrp.
            //

            if (Irp->IoStatus.Status != STATUS_SUCCESS) {

                //
                // Oh well, we can just finish it off.
                //
                NOTHING;

            } else if (Irp->Cancel) {

                Irp->IoStatus.Status = STATUS_CANCELLED;

            } else {

                //
                // Give it a new cancel routine, and increment the
                // reference count because the cancel routine has
                // a reference to it.
                //

                IoSetCancelRoutine(
                    Irp,
                    CyzCancelCurrentXoff
                    );

                SERIAL_SET_REFERENCE(
                    Irp,
                    SERIAL_REF_CANCEL
                    );

                //
                // We don't want to complete the current irp now.  This
                // will now get completed by the Xoff counter code.
                //

                CompleteCurrent = FALSE;

                //
                // Give the counter to the isr.
                //

                Extension->CurrentXoffIrp = Irp;
                #ifdef POLL
                KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
                CyzGiveXoffToIsr(Extension);
                KeReleaseSpinLock(&Extension->PollLock,pollIrql);
                #else
                KeSynchronizeExecution(
                    Extension->Interrupt,
                    CyzGiveXoffToIsr,
                    Extension
                    );
                #endif
				
                //
                // Start the timer for the counter and increment
                // the reference count since the timer has a
                // reference to the irp.
                //

                if (Xc->Timeout) {

                    LARGE_INTEGER delta;

                    delta.QuadPart = -((LONGLONG)UInt32x32To64(
                                                     1000,
                                                     Xc->Timeout
                                                     ));

                    CyzSetTimer(
                        &Extension->XoffCountTimer,
                        delta,
                        &Extension->XoffCountTimeoutDpc,
                        Extension
                        );

                    SERIAL_SET_REFERENCE(
                        Irp,
                        SERIAL_REF_TOTAL_TIMER
                        );

                }

            }

            IoReleaseCancelSpinLock(OldIrql);

        }

        //
        // Note that the following call will (probably) also cause
        // the current irp to be completed.
        //

        CyzGetNextIrp(
            CurrentOpIrp,
            QueueToProcess,
            NewIrp,
            CompleteCurrent,
            Extension
            );

        if (!*NewIrp) {

            KIRQL OldIrql;
            #ifdef POLL
            KIRQL pollIrql;
            #endif

            IoAcquireCancelSpinLock(&OldIrql);
            #ifdef POLL
            KeAcquireSpinLock(&Extension->PollLock,&pollIrql);
            CyzProcessEmptyTransmit(Extension);
            KeReleaseSpinLock(&Extension->PollLock,pollIrql);
            #else
            KeSynchronizeExecution(
                Extension->Interrupt,
                CyzProcessEmptyTransmit,
                Extension
                );
            #endif				
            IoReleaseCancelSpinLock(OldIrql);

            break;

        } else if (IoGetCurrentIrpStackLocation(*NewIrp)->MajorFunction
                   == IRP_MJ_FLUSH_BUFFERS) {

            //
            // If we encounter a flush request we just want to get
            // the next irp and complete the flush.
            //
            // Note that if NewIrp is non-null then it is also
            // equal to CurrentWriteIrp.
            //


            ASSERT((*NewIrp) == (*CurrentOpIrp));
            (*NewIrp)->IoStatus.Status = STATUS_SUCCESS;

        } else {

            break;

        }

    } while (TRUE);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzGetNextWrite\n");
}

VOID
CyzCompleteWrite(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is merely used to complete any write.  It
    assumes that the status and the information fields of
    the irp are already correctly filled in.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);
    
    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzCompleteWrite(%X)\n",
                  Extension);

//    LOGENTRY(LOG_MISC, ZSIG_WRITE_COMPLETE,
//                       Extension->PortIndex+1,
//                       0, 
//                       0);

    IoAcquireCancelSpinLock(&OldIrql);

    CyzTryToCompleteCurrent(Extension, NULL, OldIrql, STATUS_SUCCESS,
                            &Extension->CurrentWriteIrp,
                            &Extension->WriteQueue, NULL,
                            &Extension->WriteRequestTotalTimer,
                            CyzStartWrite, CyzGetNextWrite,
                            SERIAL_REF_ISR);

    CyzDpcEpilogue(Extension, Dpc);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzCompleteWrite\n");

}

BOOLEAN
CyzProcessEmptyTransmit(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to determine if conditions are appropriate
    to satisfy a wait for transmit empty event, and if so to complete
    the irp that is waiting for that event.  It also call the code
    that checks to see if we should lower the RTS line if we are
    doing transmit toggling.

    NOTE: This routine is called by KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the cancel
          spinlock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = Context;
    CYZ_LOCKED_PAGED_CODE();

    if (Extension->IsrWaitMask && (Extension->IsrWaitMask & SERIAL_EV_TXEMPTY) &&
        Extension->EmptiedTransmit && (!Extension->TransmitImmediate) &&
        (!Extension->CurrentWriteIrp) && IsListEmpty(&Extension->WriteQueue)) {

        Extension->HistoryMask |= SERIAL_EV_TXEMPTY;
        if (Extension->IrpMaskLocation) {

            *Extension->IrpMaskLocation = Extension->HistoryMask;
            Extension->IrpMaskLocation = NULL;
            Extension->HistoryMask = 0;

            Extension->CurrentWaitIrp->IoStatus.Information = sizeof(ULONG);
            CyzInsertQueueDpc(
                &Extension->CommWaitDpc,
                NULL,
                NULL,
                Extension
                );

        }

        Extension->CountOfTryingToLowerRTS++;
        CyzPerhapsLowerRTS(Extension);

    }

    return FALSE;

}

BOOLEAN
CyzGiveWriteToIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    Try to start off the write by slipping it in behind
    a transmit immediate char, or if that isn't available
    and the transmit holding register is empty, "tickle"
    the UART into interrupting with a transmit buffer
    empty.

    NOTE: This routine is called by KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the
          cancel spin lock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = Context;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //
    PIO_STACK_LOCATION IrpSp;

    CYZ_LOCKED_PAGED_CODE();

//    LOGENTRY(LOG_MISC, ZSIG_GIVE_WRITE_TO_ISR,
//                       Extension->PortIndex+1,
//                       0, 
//                       0);

    IrpSp = IoGetCurrentIrpStackLocation(Extension->CurrentWriteIrp);

    //
    // We might have a xoff counter request masquerading as a
    // write.  The length of these requests will always be one
    // and we can get a pointer to the actual character from
    // the data supplied by the user.
    //

    if (IrpSp->MajorFunction == IRP_MJ_WRITE) {

        Extension->WriteLength = IrpSp->Parameters.Write.Length;
        Extension->WriteCurrentChar =
            Extension->CurrentWriteIrp->AssociatedIrp.SystemBuffer;

    } else {

        Extension->WriteLength = 1;
        Extension->WriteCurrentChar =
            ((PUCHAR)Extension->CurrentWriteIrp->AssociatedIrp.SystemBuffer) +
            FIELD_OFFSET(
                SERIAL_XOFF_COUNTER,
                XoffChar
                );

    }

    //
    // The isr now has a reference to the irp.
    //

    
    SERIAL_SET_REFERENCE(
        Extension->CurrentWriteIrp,
        SERIAL_REF_ISR
        );

    //
    // Check first to see if an immediate char is transmitting.
    // If it is then we'll just slip in behind it when its
    // done.
    //

//Removed at 02/07/00 by Fanny. Polling routine will do the transmission.
    if (!Extension->TransmitImmediate) {
//
//        //
//        // If there is no immediate char transmitting then we
//        // will "re-enable" the transmit holding register empty
//        // interrupt.  The 8250 family of devices will always
//        // signal a transmit holding register empty interrupt
//        // *ANY* time this bit is set to one.  By doing things
//        // this way we can simply use the normal interrupt code
//        // to start off this write.
//        //
//        // We've been keeping track of whether the transmit holding
//        // register is empty so it we only need to do this
//        // if the register is empty.
//        //
//

//#if DBG 
//{
//    PUCHAR writeptr = Extension->WriteCurrentChar;
//    int ivar;
//    for (ivar=0; ivar< Extension->WriteLength; ivar++) {
//        //DbgPrint("%c",writeptr[ivar]);
//        LOGENTRY(LOG_MISC,ZSIG_TRANSMIT,0,0,writeptr[ivar]);
//    }
//    //DbgPrint("\n");
//}
//#endif

        
        //if (Extension->HoldingEmpty) {
            // enable transmit intr
            CyzTxStart(Extension);
        //}

    }

    //
    // The rts line may already be up from previous writes,
    // however, it won't take much additional time to turn
    // on the RTS line if we are doing transmit toggling.
    //

    if ((Extension->HandFlow.FlowReplace & SERIAL_RTS_MASK) ==
        SERIAL_TRANSMIT_TOGGLE) {

        CyzSetRTS(Extension);

    }

    return FALSE;

}

VOID
CyzCancelCurrentWrite(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel the current write.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    CYZ_LOCKED_PAGED_CODE();

    CyzTryToCompleteCurrent(
        Extension,
        CyzGrabWriteFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &Extension->CurrentWriteIrp,
        &Extension->WriteQueue,
        NULL,
        &Extension->WriteRequestTotalTimer,
        CyzStartWrite,
        CyzGetNextWrite,
        SERIAL_REF_CANCEL
        );

}

VOID
CyzWriteTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine will try to timeout the current write.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzWriteTimeout(%X)\n",
                     Extension);

    IoAcquireCancelSpinLock(&OldIrql);

    CyzTryToCompleteCurrent(Extension, CyzGrabWriteFromIsr, OldIrql,
                               STATUS_TIMEOUT, &Extension->CurrentWriteIrp,
                               &Extension->WriteQueue, NULL,
                               &Extension->WriteRequestTotalTimer,
                               CyzStartWrite, CyzGetNextWrite,
                               SERIAL_REF_TOTAL_TIMER);

    CyzDpcEpilogue(Extension, Dpc);


    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzWriteTimeout\n");
}

BOOLEAN
CyzGrabWriteFromIsr(
    IN PVOID Context
    )

/*++

Routine Description:


    This routine is used to grab the current irp, which could be timing
    out or canceling, from the ISR

    NOTE: This routine is being called from KeSynchronizeExecution.

    NOTE: This routine assumes that the cancel spin lock is held
          when this routine is called.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always false.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = Context;
    CYZ_LOCKED_PAGED_CODE();

    //
    // Check if the write length is non-zero.  If it is non-zero
    // then the ISR still owns the irp. We calculate the the number
    // of characters written and update the information field of the
    // irp with the characters written.  We then clear the write length
    // the isr sees.
    //

    if (Extension->WriteLength) {

        //
        // We could have an xoff counter masquerading as a
        // write irp.  If so, don't update the write length.
        //

        if (IoGetCurrentIrpStackLocation(Extension->CurrentWriteIrp)
            ->MajorFunction == IRP_MJ_WRITE) {

            Extension->CurrentWriteIrp->IoStatus.Information =
                IoGetCurrentIrpStackLocation(
                    Extension->CurrentWriteIrp
                    )->Parameters.Write.Length -
                Extension->WriteLength;

        } else {

            Extension->CurrentWriteIrp->IoStatus.Information = 0;

        }

        //
        // Since the isr no longer references this irp, we can
        // decrement it's reference count.
        //

        SERIAL_CLEAR_REFERENCE(
            Extension->CurrentWriteIrp,
            SERIAL_REF_ISR
            );

        Extension->WriteLength = 0;

        if (Extension->ReturnStatusAfterFwEmpty) {

            struct BUF_CTRL *buf_ctrl;
            ULONG tx_get, tx_put;

            Extension->ReturnWriteStatus = FALSE;

            // Flush fw buffer and Startech FIFO.
            CyzIssueCmd(Extension,C_CM_FLUSH_TX,0L,TRUE); 

            // Flush transmission buffer in the firmware
            //buf_ctrl = Extension->BufCtrl;		
            //tx_put = CYZ_READ_ULONG(&buf_ctrl->tx_put);
            //tx_get = CYZ_READ_ULONG(&buf_ctrl->tx_get);	
	
            //while (tx_put != tx_get) {
            //    tx_put = tx_get;
            //    CYZ_WRITE_ULONG(&buf_ctrl->tx_put,tx_put);
            //    tx_get = CYZ_READ_ULONG(&buf_ctrl->tx_get);	
            //}	
        }
    }

    return FALSE;

}

BOOLEAN
CyzGrabXoffFromIsr(
    IN PVOID Context
    )

/*++

Routine Description:

    This routine is used to grab an xoff counter irp from the
    isr when it is no longer masquerading as a write irp.  This
    routine is called by the cancel and timeout code for the
    xoff counter ioctl.


    NOTE: This routine is being called from KeSynchronizeExecution.

    NOTE: This routine assumes that the cancel spin lock is held
          when this routine is called.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    Always false.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = Context;
    CYZ_LOCKED_PAGED_CODE();

    if (Extension->CountSinceXoff) {

        //
        // This is only non-zero when there actually is a Xoff ioctl
        // counting down.
        //

        Extension->CountSinceXoff = 0;

        //
        // We decrement the count since the isr no longer owns
        // the irp.
        //

        SERIAL_CLEAR_REFERENCE(
            Extension->CurrentXoffIrp,
            SERIAL_REF_ISR
            );

    }

    return FALSE;

}

VOID
CyzCompleteXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is merely used to truely complete an xoff counter irp.  It
    assumes that the status and the information fields of the irp are
    already correctly filled in.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzCompleteXoff(%X)\n",
                  Extension);

    IoAcquireCancelSpinLock(&OldIrql);

    CyzTryToCompleteCurrent(Extension, NULL, OldIrql, STATUS_SUCCESS,
                            &Extension->CurrentXoffIrp, NULL, NULL,
                            &Extension->XoffCountTimer, NULL, NULL,
                            SERIAL_REF_ISR);

    CyzDpcEpilogue(Extension, Dpc);


    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzCompleteXoff\n");

}

VOID
CyzTimeoutXoff(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemContext1,
    IN PVOID SystemContext2
    )

/*++

Routine Description:

    This routine is merely used to truely complete an xoff counter irp,
    if its timer has run out.

Arguments:

    Dpc - Not Used.

    DeferredContext - Really points to the device extension.

    SystemContext1 - Not Used.

    SystemContext2 - Not Used.

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = DeferredContext;
    KIRQL OldIrql;

    UNREFERENCED_PARAMETER(SystemContext1);
    UNREFERENCED_PARAMETER(SystemContext2);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, ">CyzTimeoutXoff(%X)\n", Extension);

    IoAcquireCancelSpinLock(&OldIrql);

    CyzTryToCompleteCurrent(Extension, CyzGrabXoffFromIsr, OldIrql,
                            STATUS_SERIAL_COUNTER_TIMEOUT,
                            &Extension->CurrentXoffIrp, NULL, NULL, NULL,
                            NULL, NULL, SERIAL_REF_TOTAL_TIMER);

    CyzDpcEpilogue(Extension, Dpc);

    CyzDbgPrintEx(DPFLTR_TRACE_LEVEL, "<CyzTimeoutXoff\n");
}

VOID
CyzCancelCurrentXoff(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
    )

/*++

Routine Description:

    This routine is used to cancel the current write.

Arguments:

    DeviceObject - Pointer to the device object for this device

    Irp - Pointer to the IRP to be canceled.

Return Value:

    None.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = DeviceObject->DeviceExtension;
    CYZ_LOCKED_PAGED_CODE();

    CyzTryToCompleteCurrent(
        Extension,
        CyzGrabXoffFromIsr,
        Irp->CancelIrql,
        STATUS_CANCELLED,
        &Extension->CurrentXoffIrp,
        NULL,
        NULL,
        &Extension->XoffCountTimer,
        NULL,
        NULL,
        SERIAL_REF_CANCEL
        );

}

BOOLEAN
CyzGiveXoffToIsr(
    IN PVOID Context
    )

/*++

Routine Description:


    This routine starts off the xoff counter.  It merely
    has to set the xoff count and increment the reference
    count to denote that the isr has a reference to the irp.

    NOTE: This routine is called by KeSynchronizeExecution.

    NOTE: This routine assumes that it is called with the
          cancel spin lock held.

Arguments:

    Context - Really a pointer to the device extension.

Return Value:

    This routine always returns FALSE.

--*/

{

    PCYZ_DEVICE_EXTENSION Extension = Context;

    //
    // The current stack location.  This contains all of the
    // information we need to process this particular request.
    //

    PSERIAL_XOFF_COUNTER Xc =
        Extension->CurrentXoffIrp->AssociatedIrp.SystemBuffer;

    CYZ_LOCKED_PAGED_CODE();

    ASSERT(Extension->CurrentXoffIrp);
    Extension->CountSinceXoff = Xc->Counter;

    //
    // The isr now has a reference to the irp.
    //

    SERIAL_SET_REFERENCE(
        Extension->CurrentXoffIrp,
        SERIAL_REF_ISR
        );

    return FALSE;

}
