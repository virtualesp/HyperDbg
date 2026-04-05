/**
 * @file Tracing.c
 * @author Hari Mishal (harimishal6@gmail.com)
 * @brief Tracing routines for HyperTrace module
 * @details
 * @version 0.18
 * @date 2025-12-02
 *
 * @copyright This project is released under the GNU Public License v3.
 */
#include "pch.h"

/**
 * @brief The flag indicating whether the hypertrace module is initialized or not
 *
 */
BOOLEAN g_HyperTraceInitialized = FALSE;

/**
 * @brief Hide debugger on transparent-mode (activate transparent-mode)
 *
 * @param HypertraceCallbacks
 *
 * @return BOOLEAN
 */
VOID
PerformLbrTraceAfterEnable()
{
    LBR_IOCTL_REQUEST Request;
    RtlZeroMemory(&Request, sizeof(LBR_IOCTL_REQUEST));

    KAFFINITY Affinity = 1;
    KeSetSystemAffinityThread(Affinity);

    LbrInitialize();

    if (!LbrCheck())
        return;

    Request.LbrConfig.Pid       = 0;
    Request.LbrConfig.LbrSelect = LBR_SELECT;

    if (LbrEnableLbr(&Request))
    {
        for (volatile int i = 0; i < 50; i++)
        {
            if (i % 2)
            {
                int a = i * 2;
                a += 5;
            }
            else
            {
                __nop();
                __nop();
            }
        }
        LBR_STATE * State = LbrFindLbrState(0);
        if (State)
            LbrGetLbr(State);

        LogInfo("Dumping LBR Buffer...\n");
        LbrDumpLbr(&Request);

        LbrDisableLbr(&Request);
    }

    KeRevertToUserAffinityThread();
}

/**
 * @brief Initialize the hyper trace module
 *
 * @param HypertraceCallbacks
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTraceInit(HYPERTRACE_CALLBACKS * HypertraceCallbacks)
{
    //
    // Check if the LBR is supported on this CPU before initializing the hypertrace module,
    //
    if (!LbrCheck())
    {
        return FALSE;
    }

    //
    // Check if any of the required callbacks are NULL
    //
    for (UINT32 i = 0; i < sizeof(HYPERTRACE_CALLBACKS) / sizeof(UINT64); i++)
    {
        if (((PVOID *)HypertraceCallbacks)[i] == NULL)
        {
            //
            // The callback has null entry, so we cannot proceed
            //
            return FALSE;
        }
    }

    //
    // Save the callbacks
    //
    RtlCopyMemory(&g_Callbacks, HypertraceCallbacks, sizeof(HYPERTRACE_CALLBACKS));

    //
    // Enable LBR
    //
    g_HyperTraceInitialized = TRUE;

    return TRUE;
}

/**
 * @brief Uninitialize the hyper trace module
 *
 * @return VOID
 */
VOID
HyperTraceUninit()
{
    g_HyperTraceInitialized = FALSE;
}

/**
 * @brief Perform actions related to HyperTrace
 *
 * @param HyperTraceOperationRequest
 * @param ApplyFromVmxRootMode
 *
 * @return BOOLEAN
 */
BOOLEAN
HyperTracePerformOperation(HYPERTRACE_OPERATION_PACKETS * HyperTraceOperationRequest,
                           BOOLEAN                        ApplyFromVmxRootMode)
{
    BOOLEAN Status = TRUE;
    UNREFERENCED_PARAMETER(ApplyFromVmxRootMode);

    //
    // Check if the hypertrace module is initialized before performing any operation
    //
    if (!g_HyperTraceInitialized)
    {
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_HYPERTRACE_NOT_INITIALIZED;
        return FALSE;
    }

    //
    // Perform the requested operation
    //
    switch (HyperTraceOperationRequest->HyperTraceOperationType)
    {
    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_ENABLE:
        LogInfo("HyperTrace: Enabling LBR tracing...\n");
        break;

    case HYPERTRACE_LBR_OPERATION_REQUEST_TYPE_DISABLE:
        LogInfo("HyperTrace: Disabling LBR tracing...\n");
        break;

    default:
        Status                                   = FALSE;
        HyperTraceOperationRequest->KernelStatus = DEBUGGER_ERROR_INVALID_HYPERTRACE_OPERATION_TYPE;
        break;
    }

    return Status;
}
