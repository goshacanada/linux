# CXI InfiniBand Driver Integration Guide

This document describes how to integrate the CXI InfiniBand driver with the existing Linux kernel InfiniBand subsystem and the shs-cxi-driver.

## Integration Steps

### 1. Kernel Configuration Integration

Add the CXI driver to the InfiniBand hardware drivers configuration:

**File: `drivers/infiniband/hw/Kconfig`**
```kconfig
# Add after existing drivers
source "drivers/infiniband/hw/cxi/Kconfig"
```

**File: `drivers/infiniband/hw/Makefile`**
```makefile
# Add to the list of subdirectories
obj-$(CONFIG_INFINIBAND_CXI)    += cxi/
```

### 2. RDMA Core Integration

Register a new driver ID with the RDMA core subsystem:

**File: `include/rdma/ib_verbs.h`**
```c
enum rdma_driver_id {
    // ... existing drivers ...
    RDMA_DRIVER_CXI,
    RDMA_DRIVER_MAX
};
```

Update the driver in `cxi_main.c`:
```c
static const struct ib_device_ops cxi_ib_dev_ops = {
    .driver_id = RDMA_DRIVER_CXI,
    // ... rest of operations
};
```

### 3. CXI Core Driver Dependencies

Ensure the CXI InfiniBand driver properly depends on the CXI core:

**Required CXI Core APIs:**
- `cxi_register_client()` / `cxi_unregister_client()`
- `cxi_lni_alloc()` / `cxi_lni_free()`
- `cxi_cp_alloc()` / `cxi_cp_free()`
- `cxi_eq_alloc()` / `cxi_eq_free()`
- `cxi_cq_alloc()` / `cxi_cq_free()`
- `cxi_map()` / `cxi_unmap()`
- `cxi_dev_info_get()`

### 4. Device Tree Integration

For platforms using device tree, add CXI device nodes:

```dts
cxi0: cxi@<address> {
    compatible = "hpe,cassini";
    reg = <...>;
    interrupts = <...>;
    // Additional properties
};
```

### 5. User-Space Integration

#### libibverbs Provider
Create a user-space provider library for libibverbs:

**Directory structure:**
```
providers/cxi/
├── cxi.h
├── cxi_verbs.c
├── cxi_qp.c
├── cxi_cq.c
└── CMakeLists.txt
```

#### libfabric Provider
Integrate with libfabric for broader application support:

```c
struct fi_provider cxi_prov = {
    .name = "cxi",
    .version = FI_VERSION(1, 0),
    .fi_version = FI_VERSION(1, 19),
    // ... provider operations
};
```

## Testing Integration

### 1. Basic Functionality Tests

```bash
# Load the driver
modprobe cxi_ib

# Verify device registration
ls /sys/class/infiniband/
cat /sys/class/infiniband/cxi_*/node_type

# Test with ibverbs
ibv_devices
ibv_devinfo -d cxi_0
```

### 2. Performance Tests

```bash
# Bandwidth test
ib_send_bw -d cxi_0

# Latency test
ib_send_lat -d cxi_0

# Multi-threaded tests
ib_send_bw -d cxi_0 -T 4
```

### 3. Application Integration

Test with common RDMA applications:
- MPI implementations (OpenMPI, MPICH)
- Storage applications (NVMe-oF)
- Networking applications (RoCE)

## Debugging Integration

### 1. Kernel Debugging

Enable debugging options:
```bash
echo 'module cxi_ib +p' > /sys/kernel/debug/dynamic_debug/control
echo 'module cxi_core +p' > /sys/kernel/debug/dynamic_debug/control
```

### 2. Tracing

Use kernel tracing for performance analysis:
```bash
echo 1 > /sys/kernel/debug/tracing/events/rdma/enable
echo 1 > /sys/kernel/debug/tracing/events/cxi/enable
```

### 3. Statistics

Monitor driver statistics:
```bash
cat /sys/class/infiniband/cxi_*/ports/1/counters/*
cat /sys/class/infiniband/cxi_*/hw_counters/*
```

## Common Integration Issues

### 1. Symbol Resolution
Ensure all CXI core symbols are properly exported:
```c
EXPORT_SYMBOL_GPL(cxi_lni_alloc);
EXPORT_SYMBOL_GPL(cxi_cp_alloc);
// ... other symbols
```

### 2. Memory Management
Verify proper DMA mapping and unmapping:
- Use `dma_map_single()` / `dma_unmap_single()`
- Handle DMA mapping errors
- Ensure proper cache coherency

### 3. Interrupt Handling
Ensure proper MSI-X interrupt setup:
- Request appropriate number of vectors
- Handle interrupt registration/deregistration
- Implement proper interrupt handlers

### 4. Device Lifecycle
Handle device hotplug scenarios:
- Proper cleanup on device removal
- Resource deallocation
- User-space notification

## Performance Considerations

### 1. Memory Registration
- Implement efficient memory registration caching
- Use on-demand paging where supported
- Optimize for large memory regions

### 2. Queue Management
- Implement efficient queue pair scheduling
- Use appropriate queue depths
- Optimize completion processing

### 3. Interrupt Coalescing
- Implement interrupt coalescing for better performance
- Balance latency vs. throughput
- Provide tunable parameters

## Security Considerations

### 1. User-Space Access
- Validate all user-space inputs
- Implement proper permission checks
- Prevent privilege escalation

### 2. Memory Protection
- Ensure proper memory isolation
- Validate memory region access rights
- Implement secure key management

### 3. Network Security
- Support for secure communication protocols
- Implement proper authentication mechanisms
- Handle security events appropriately

## Maintenance

### 1. Version Compatibility
- Maintain backward compatibility with existing applications
- Document API changes
- Provide migration guides

### 2. Bug Reporting
- Implement comprehensive error reporting
- Provide debugging information
- Maintain issue tracking

### 3. Updates
- Regular security updates
- Performance improvements
- Feature enhancements based on user feedback
