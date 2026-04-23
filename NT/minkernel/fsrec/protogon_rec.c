#pragma warning (disable:4996)

#include "fs_rec.h"
#include "protogon_rec.h"

// Check if volume is ReFS (supports both ReFS and ReFSv1)
BOOLEAN
IsReFSVolume(
    IN PREFS_BOOT_SECTOR BootSector,
    IN ULONG BytesPerSector,
    IN PLARGE_INTEGER NumberOfSectors,
    IN UCHAR Flag
)
{
    USHORT length;
    USHORT checksum;
    USHORT calculatedChecksum;
    USHORT i;
    PUCHAR data;
    ULONG bytesPerSector;
    ULONG sectorsPerCluster;
    ULONG bytesPerCluster;
    ULONG clusterSize;
    ULONG flags1;
    ULONG flags2;
    ULONG versionMajor;
    ULONG unused;
    ULONG identifier;
    
    // Check OEM ID for "ReFS\0\0\0"
    if (BootSector->Oem[0] == 'R' && BootSector->Oem[1] == 'e' &&
        BootSector->Oem[2] == 'F' && BootSector->Oem[3] == 'S' &&
        BootSector->Oem[4] == 0 && BootSector->Oem[5] == 0 &&
        BootSector->Oem[6] == 0 && BootSector->Oem[7] == 0) {
        
        // Check version - Flag 0 means ReFS (>= v2), Flag 1 means ReFSv1 (== v1)
        if (Flag == 0) {
            // ReFS requires major version >= 2
            if (BootSector->Version.Major < 2) {
                return FALSE;
            }
        } else {
            // ReFSv1 requires major version == 1
            if (BootSector->Version.Major != 1) {
                return FALSE;
            }
        }
        
        length = BootSector->Length;
        if (length > 0x57 && length <= BytesPerSector) {
            // Calculate checksum
            calculatedChecksum = 3;
            data = (PUCHAR)BootSector + 3;
            for (i = 3; i < length; i++) {
                if (i != 0x16 && i != 0x17) {
                    calculatedChecksum = (USHORT)((calculatedChecksum >> 1) + *data +
                                                   ((calculatedChecksum & 1) << 15));
                }
                data++;
            }
            
            checksum = BootSector->Checksum;
            bytesPerSector = BootSector->BytesPerSector;
            sectorsPerCluster = BootSector->SectorsPerCluster;
            bytesPerCluster = bytesPerSector * sectorsPerCluster;
            
            if (checksum == calculatedChecksum &&
                bytesPerCluster > 0x1FF && bytesPerCluster < 0x1001) {
                
                // Check flags based on version
                if (Flag == 0) {
                    // ReFS requires Flags1 == 1
                    if (BootSector->Flags1 != 1) {
                        return FALSE;
                    }
                } else {
                    // ReFSv1 allows specific Flags1 values
                    unused = BootSector->Flags1;
                    if (unused != 4 && unused != 8 && unused != 0x10 &&
                        unused != 0x20 && unused != 0x40 && unused != 0x80 &&
                        unused != 0x100 && unused != 0x200) {
                        return FALSE;
                    }
                }
                
                clusterSize = BootSector->Flags2;
                identifier = BootSector->Identifier;
                
                if ((clusterSize != 0 || BootSector->SectorsPerCluster != 0) &&
                    clusterSize <= identifier &&
                    (clusterSize < identifier || BootSector->SectorsPerCluster <= identifier)) {
                    return TRUE;
                }
            }
        }
    }
    
    return FALSE;
}

// ReFS Recognizer FsControl dispatch routine
NTSTATUS
ReFSRecFsControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp,
    UCHAR Flag
)
{
    PIO_STACK_LOCATION stack;
    UCHAR controlCode;
    NTSTATUS status;
    ULONG sectorSize;
    PULONG sectorSizeBuffer;
    PVOID buffer = NULL;
    ULONG bufferSize;
    REFS_BOOT_SECTOR bootSector;
    LARGE_INTEGER volumeSize;
    BOOLEAN isReFS;
    PDEVICE_OBJECT targetDevice;
    PIRP newIrp;
    KEVENT event;
    IO_STATUS_BLOCK ioStatus;
    LARGE_INTEGER byteOffset;
    ULONG ioControlCode;
    DISK_GEOMETRY diskGeometry;
    PWCHAR registryPath;
    
    stack = IoGetCurrentIrpStackLocation(Irp);
    controlCode = (UCHAR)(stack->Parameters.DeviceIoControl.IoControlCode);
    
    switch (controlCode) {
        case IRP_MN_MOUNT_VOLUME:
            sectorSizeBuffer = (PULONG)Irp->AssociatedIrp.SystemBuffer;
            status = FsRecGetDeviceSectorSize(DeviceObject, sectorSizeBuffer);
            if (!NT_SUCCESS(status)) {
                break;
            }
            
            sectorSize = *sectorSizeBuffer;
            
            // Get device geometry
            KeInitializeEvent(&event, NotificationEvent, FALSE);
            newIrp = IoBuildDeviceIoControlRequest(
                IOCTL_DISK_GET_DRIVE_GEOMETRY,
                DeviceObject,
                NULL,
                0,
                &diskGeometry,
                sizeof(DISK_GEOMETRY),
                FALSE,
                &event,
                &ioStatus
            );
            
            if (!newIrp) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            
            status = IoCallDriver(DeviceObject, newIrp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }
            
            if (!NT_SUCCESS(status)) {
                break;
            }
            
            // Allocate buffer for boot sector
            buffer = ExAllocatePoolWithTag(NonPagedPool, sectorSize, 'sFeR');
            if (!buffer) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            
            // Read boot sector
            KeInitializeEvent(&event, NotificationEvent, FALSE);
            byteOffset.QuadPart = 0;
            newIrp = IoBuildSynchronousFsdRequest(
                IRP_MJ_READ,
                DeviceObject,
                buffer,
                sectorSize,
                &byteOffset,
                &event,
                &ioStatus
            );
            
            if (!newIrp) {
                ExFreePoolWithTag(buffer, 'sFeR');
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }
            
            status = IoCallDriver(DeviceObject, newIrp);
            if (status == STATUS_PENDING) {
                KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                status = ioStatus.Status;
            }
            
            if (!NT_SUCCESS(status)) {
                ExFreePoolWithTag(buffer, 'sFeR');
                break;
            }
            
            // Check if it's ReFS/ReFSv1
            volumeSize.QuadPart = sectorSize;
            isReFS = IsReFSVolume((PREFS_BOOT_SECTOR)buffer, sectorSize, &volumeSize, Flag);
            
            if (isReFS) {
                status = STATUS_FS_DRIVER_REQUIRED;
            } else {
                // Try reading from mirror location (sector at offset sectorSize)
                byteOffset.QuadPart = sectorSize;
                newIrp = IoBuildSynchronousFsdRequest(
                    IRP_MJ_READ,
                    DeviceObject,
                    buffer,
                    sectorSize,
                    &byteOffset,
                    &event,
                    &ioStatus
                );
                
                if (!newIrp) {
                    ExFreePoolWithTag(buffer, 'sFeR');
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }
                
                status = IoCallDriver(DeviceObject, newIrp);
                if (status == STATUS_PENDING) {
                    KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
                    status = ioStatus.Status;
                }
                
                if (NT_SUCCESS(status)) {
                    isReFS = IsReFSVolume((PREFS_BOOT_SECTOR)buffer, sectorSize, &volumeSize, Flag);
                    if (isReFS) {
                        status = STATUS_FS_DRIVER_REQUIRED;
                    } else {
                        status = STATUS_UNRECOGNIZED_MEDIA;
                    }
                } else {
                    status = STATUS_UNRECOGNIZED_MEDIA;
                }
            }
            
            ExFreePoolWithTag(buffer, 'sFeR');
            break;
            
        case IRP_MN_LOAD_FILE_SYSTEM:
            // Use appropriate registry path based on Flag
            if (Flag == 0) {
                // ReFS
                registryPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ReFS";
            } else {
                // ReFSv1
                registryPath = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\ReFSv1";
            }
            status = FsRecLoadFileSystem(DeviceObject, registryPath);
            break;
            
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
    
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return status;
}

