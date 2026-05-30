/*++ BUILD Version: 0001    // Increment this if a change has global effects

   Copyright (c) Microsoft Corporation

   Module Name:
       errlog.mc

   Abstract:
       Constant definitions for the log error code values.

   Revision History:
--*/

#ifndef _ERRLOGMC_
#define _ERRLOGMC_

//
//  Status values are 32 bit values layed out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-------------------------+-------------------------------+
//  |Sev|C|       Facility          |               Code            |
//  +---+-+-------------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//

//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_RPC_RUNTIME             0x2
#define FACILITY_RPC_STUBS               0x3
#define FACILITY_IO_ERROR_CODE           0x4
#define FACILITY_DEVICE_ERROR_CODE       0x5


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: ERRLOG_NOT_SUPPORTED
//
// MessageText:
//
// The request is not supported.
//
#define ERRLOG_NOT_SUPPORTED             ((NTSTATUS)0xC0050001L)

//
// MessageId: ERRLOG_INSUFFICIENT_RESOURCES
//
// MessageText:
//
// Failed allocating requested system resources.
//
#define ERRLOG_INSUFFICIENT_RESOURCES    ((NTSTATUS)0xC0050002L)

//
// MessageId: ERRLOG_DEVICE_REMOVED
//
// MessageText:
//
// The device has been removed.
//
#define ERRLOG_DEVICE_REMOVED            ((NTSTATUS)0xC0050003L)

//
// MessageId: ERRLOG_DEVICE_NOT_STARTED
//
// MessageText:
//
// The device has not been started.
//
#define ERRLOG_DEVICE_NOT_STARTED        ((NTSTATUS)0xC0050004L)

//
// MessageId: ERRLOG_BUFFER_TOO_SMALL
//
// MessageText:
//
// The buffer is too small for the requested data.
//
#define ERRLOG_BUFFER_TOO_SMALL          ((NTSTATUS)0xC0050005L)

//
// MessageId: ERRLOG_INVALID_BUFFER_SIZE
//
// MessageText:
//
// The size of the buffer is invalid for the specified operation.
//
#define ERRLOG_INVALID_BUFFER_SIZE       ((NTSTATUS)0xC0050006L)

//
// MessageId: ERRLOG_INVALID_PARAMETER
//
// MessageText:
//
// An invalid parameter was passed to a function.
//
#define ERRLOG_INVALID_PARAMETER         ((NTSTATUS)0xC0050007L)

//
// MessageId: ERRLOG_LOWERDRV_IRP_FAILED
//
// MessageText:
//
// Lower driver has failed the request.
//
#define ERRLOG_LOWERDRV_IRP_FAILED       ((NTSTATUS)0xC0050008L)

//
// MessageId: ERRLOG_OPENREG_FAILED
//
// MessageText:
//
// Failed opening registry key.
//
#define ERRLOG_OPENREG_FAILED            ((NTSTATUS)0xC0050009L)

//
// MessageId: ERRLOG_READREG_FAILED
//
// MessageText:
//
// Failed reading registry value.
//
#define ERRLOG_READREG_FAILED            ((NTSTATUS)0xC005000AL)

//
// MessageId: ERRLOG_WRITEREG_FAILED
//
// MessageText:
//
// Failed writing registry value.
//
#define ERRLOG_WRITEREG_FAILED           ((NTSTATUS)0xC005000BL)

//
// MessageId: ERRLOG_REGTYPE_MISMATCH
//
// MessageText:
//
// Registry data type mismatch.
//
#define ERRLOG_REGTYPE_MISMATCH          ((NTSTATUS)0xC005000CL)

//
// MessageId: ERRLOG_MINIDRV_REG_FAILED
//
// MessageText:
//
// This minidriver failed to register with HID class driver.
//
#define ERRLOG_MINIDRV_REG_FAILED        ((NTSTATUS)0xC005000DL)

//
// MessageId: ERRLOG_SERIAL_ACCESS_FAILED
//
// MessageText:
//
// Access serial port failed.
//
#define ERRLOG_SERIAL_ACCESS_FAILED      ((NTSTATUS)0xC0050100L)

//
// MessageId: ERRLOG_OPEN_SERIAL_FAILED
//
// MessageText:
//
// Open serial port failed.
//
#define ERRLOG_OPEN_SERIAL_FAILED        ((NTSTATUS)0xC0050101L)

//
// MessageId: ERRLOG_INIT_SERIAL_FAILED
//
// MessageText:
//
// Initialize serial port failed.
//
#define ERRLOG_INIT_SERIAL_FAILED        ((NTSTATUS)0xC0050102L)

//
// MessageId: ERRLOG_INIT_DIGITIZER_FAILED
//
// MessageText:
//
// Initialize digitizer tablet failed.
//
#define ERRLOG_INIT_DIGITIZER_FAILED     ((NTSTATUS)0xC0050103L)

//
// MessageId: ERRLOG_QUERY_DEVINFO_FAILED
//
// MessageText:
//
// Query digitizer information failed.
//
#define ERRLOG_QUERY_DEVINFO_FAILED      ((NTSTATUS)0xC0050104L)

//
// MessageId: ERRLOG_RESTORE_SERIAL_FAILED
//
// MessageText:
//
// Restore serial port settings failed.
//
#define ERRLOG_RESTORE_SERIAL_FAILED     ((NTSTATUS)0xC0050105L)

//
// MessageId: ERRLOG_WAKE_DIGITIZER_FAILED
//
// MessageText:
//
// Failed waking the digitizer.
//
#define ERRLOG_WAKE_DIGITIZER_FAILED     ((NTSTATUS)0xC0050106L)

//
// MessageId: ERRLOG_SLEEP_DIGITIZER_FAILED
//
// MessageText:
//
// Failed putting the digitizer to sleep.
//
#define ERRLOG_SLEEP_DIGITIZER_FAILED    ((NTSTATUS)0xC0050107L)

#ifdef DEBUG
//
// MessageId: ERRLOG_DEBUG_INFORMATION
//
// MessageText:
//
// The driver has reported the following debug information: %2.
//
#define ERRLOG_DEBUG_INFORMATION         ((NTSTATUS)0x40058000L)

//
// MessageId: ERRLOG_DEBUG_WARNING
//
// MessageText:
//
// The driver has reported the following debug warning: %2.
//
#define ERRLOG_DEBUG_WARNING             ((NTSTATUS)0x80058001L)

//
// MessageId: ERRLOG_DEBUG_ERROR
//
// MessageText:
//
// The driver has reported the following debug error: %2.
//
#define ERRLOG_DEBUG_ERROR               ((NTSTATUS)0xC0058002L)

#endif
#endif /* _ERRLOGMC_ */
