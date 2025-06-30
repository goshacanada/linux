# CXI InfiniBand Driver

This directory contains the CXI (Cassini) InfiniBand driver implementation, modeled after the EFA driver architecture.

## Overview

The CXI InfiniBand driver provides RDMA capabilities for CXI (Cassini) devices from Hewlett Packard Enterprise. It integrates with the existing shs-cxi-driver infrastructure to provide InfiniBand verbs operations.

## Architecture

The driver follows the standard InfiniBand driver architecture with the following components:

### Core Files

- **cxi.h** - Main header file with device structures and function prototypes
- **cxi_main.c** - Main driver file with PCI registration and device management
- **cxi_verbs.c** - InfiniBand verbs operations implementation
- **cxi_com.c** - Communication layer for hardware interaction
- **cxi_com_cmd.c** - Command operations implementation
- **cxi_com_cmd.h** - Command structures and definitions
- **cxi_com.h** - Communication layer header
- **Kconfig** - Kernel configuration options
- **Makefile** - Build configuration

### Key Components

#### Device Structure (`struct cxi_ib_dev`)
- Contains the InfiniBand device (`struct ib_device`)
- References to the underlying CXI device (`struct cxi_dev`)
- Device information and statistics
- Event queues and completion queues management
- Client registration with CXI core

#### Supported Operations
- **Queue Pairs (QP)**: Create, destroy, modify, query
- **Completion Queues (CQ)**: Create, destroy with interrupt support
- **Memory Regions (MR)**: Register, deregister user memory
- **Address Handles (AH)**: Create, destroy for addressing
- **Protection Domains (PD)**: Allocate, deallocate
- **User Context**: Allocate, deallocate for user-space access

#### Vendor-Specific Extensions
The driver implements three vendor-specific ioctl handlers for advanced CXI functionality:

1. **CXI Device Query** (`CXI_IB_METHOD_QUERY_DEVICE`)
   - Retrieves CXI-specific device information
   - Returns NIC address, PID granule, PID count, PID bits, and min_free_shift
   - Accessible via `cxidv_query_device()` user-space API

2. **CXI Memory Region Query** (`CXI_IB_METHOD_MR_QUERY`)
   - Queries CXI-specific memory region attributes
   - Returns MD handle, IOVA, length, and access flags
   - Accessible via `cxidv_query_mr()` user-space API

3. **CXI Queue Pair Query** (`CXI_IB_METHOD_QP_QUERY`)
   - Retrieves CXI-specific queue pair information
   - Returns TXQ handle, TGQ handle, command queue handle, event queue handle, and state
   - Accessible via `cxidv_query_qp()` user-space API

#### Communication Layer
- Interfaces with the CXI core driver
- Manages Logical Network Interface (LNI) allocation
- Handles Communication Profile (CP) management
- Provides memory mapping and event queue management

## Integration with shs-cxi-driver

The driver integrates with the existing CXI infrastructure:

1. **Client Registration**: Registers as a CXI client to receive device notifications
2. **Device Discovery**: Automatically discovers CXI devices through the client interface
3. **Hardware Abstraction**: Uses CXI core APIs for hardware operations
4. **Event Handling**: Receives asynchronous events from CXI devices

## Configuration

### Kernel Configuration
```
CONFIG_INFINIBAND_CXI=m
```

### Dependencies
- `CONFIG_INFINIBAND_USER_ACCESS` - For user-space access
- `CONFIG_CXI_CORE` - CXI core driver
- `CONFIG_PCI_MSI` - MSI interrupt support
- 64-bit architecture support

## Device Support

The driver supports CXI (Cassini) devices with the following characteristics:
- **Vendor ID**: 0x1590 (HPE) / 0x17db (Cray)
- **Device IDs**: 0x0371 (Cassini 2) / 0x0501 (Cassini 1)
- **Interface**: InfiniBand verbs API
- **Transport**: Reliable Connection (RC) queue pairs
- **Memory**: User memory registration with DMA mapping

## Usage

Once loaded, the driver will:
1. Register with the CXI subsystem as a client
2. Automatically detect CXI devices
3. Create InfiniBand devices (e.g., `cxi_0`, `cxi_1`)
4. Provide standard InfiniBand verbs interface

Applications can use standard RDMA libraries (libibverbs, libfabric) to access the devices.

For vendor-specific functionality, applications can use the CXI Direct Verbs API:

```c
#include "cxidv.h"

// Query CXI device information
struct cxidv_device_attr dev_attr;
int ret = cxidv_query_device(context, &dev_attr, sizeof(dev_attr));

// Query CXI memory region information
struct cxidv_mr_attr mr_attr;
ret = cxidv_query_mr(mr, &mr_attr, sizeof(mr_attr));

// Query CXI queue pair information
struct cxidv_qp_attr qp_attr;
ret = cxidv_query_qp(qp, &qp_attr, sizeof(qp_attr));
```

## Testing

To test the driver:

1. **Compilation**: Build as part of kernel or as module
   ```bash
   make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
   ```

2. **Loading**: Load the module
   ```bash
   modprobe cxi_ib
   ```

3. **Verification**: Check device registration
   ```bash
   ls /sys/class/infiniband/
   ibv_devices
   ```

4. **Basic Testing**: Use RDMA utilities
   ```bash
   ibv_devinfo
   ibv_rc_pingpong
   ```

## Implementation Notes

### Current Status
- Basic driver framework implemented
- All major InfiniBand verbs operations defined
- Integration with CXI core driver established
- Simple command implementations provided

### Future Enhancements
- Complete hardware command implementations
- Performance optimizations
- Advanced features (SRQ, atomic operations)
- Comprehensive error handling
- Hardware statistics reporting

### Known Limitations
- Simplified command implementations (placeholders)
- Limited error handling in some paths
- No SRQ (Shared Receive Queue) support
- Basic statistics implementation

## Development

For development and debugging:

1. Enable kernel debugging options
2. Use dynamic debug for detailed logging
3. Monitor CXI core driver logs
4. Test with simple RDMA applications first

The driver is designed to be extensible and can be enhanced with additional features as needed.
