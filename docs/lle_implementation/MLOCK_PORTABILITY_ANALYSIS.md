# mlock Portability Analysis for Spec 15 Minimal Secure Mode

**Date**: 2025-10-30  
**Question**: Is mlock portable across Linux, macOS, BSD, etc.?  
**Context**: Evaluating minimal secure mode implementation for Spec 15

---

## Summary

**Answer**: **YES, but with caveats**

`mlock()` is **POSIX.1-2008 standard** and available on:
- ✅ Linux (all distributions)
- ✅ macOS (all versions)
- ✅ FreeBSD, OpenBSD, NetBSD
- ✅ Solaris, AIX, HP-UX
- ❌ Windows (requires VirtualLock instead)

**Secure wipe** is more complicated - no POSIX standard, but solutions exist.

---

## Part 1: mlock/munlock Portability

### POSIX Standard

From `man mlock`:
```
STANDARDS
       mlock()
       munlock()
       mlockall()
       munlockall()
              POSIX.1-2008.
```

**Conclusion**: ✅ Fully portable across POSIX systems

### Platform-Specific Details

#### Linux
- ✅ Full support since kernel 2.0
- Requires `CAP_IPC_LOCK` capability OR appropriate rlimit
- Default rlimit often allows small amounts (~64KB) without privilege
- Header: `<sys/mman.h>`

#### macOS
- ✅ Full support (BSD-derived)
- Generally works without special privileges for reasonable amounts
- Same header: `<sys/mman.h>`
- Behavior identical to BSD

#### FreeBSD/OpenBSD/NetBSD
- ✅ Full support (native BSD implementation)
- OpenBSD is particularly security-focused, excellent mlock support
- Same POSIX interface

#### Windows
- ❌ `mlock()` does not exist
- Must use `VirtualLock()` instead (different API)
- Requires different implementation path

### Usage Requirements

**POSIX Detection**:
```c
#if defined(_POSIX_MEMLOCK) || defined(_POSIX_MEMLOCK_RANGE)
    // mlock is available
#endif
```

**Privilege Requirements**:
- Small allocations (<64KB typically): No special privileges needed
- Large allocations: May require elevated privileges or rlimit adjustment
- Lusush use case (password buffers): Usually <4KB, should work everywhere

---

## Part 2: Secure Memory Wipe Portability

### The Problem

**Standard `memset()` can be optimized away**:
```c
char password[256];
// ... use password ...
memset(password, 0, sizeof(password));  // Compiler may optimize this out!
// Compiler sees: "This memory is never read after memset, skip it"
```

**Security issue**: Password remains in memory after "wipe"

### Portable Solutions

#### Option 1: `explicit_bzero()` (Best - BSD/Linux)

**Availability**:
- ✅ OpenBSD (origin, since OpenBSD 5.5)
- ✅ FreeBSD (since 11.0)
- ✅ Linux glibc (since 2.25)
- ✅ macOS (since 10.13 High Sierra)
- ❌ Older systems

**Header**: `<strings.h>` (BSD) or `<string.h>` (glibc)

**Pros**: Guaranteed not to be optimized away, clean API
**Cons**: Not universally available on older systems

#### Option 2: `memset_s()` (C11 Annex K)

**Availability**:
- ⚠️ C11 Annex K (optional extension)
- ✅ Some implementations (Microsoft, some embedded)
- ❌ **NOT in glibc** (rejected by maintainers)
- ❌ **NOT in musl** (rejected)
- ❌ **NOT in BSD libc**
- ❌ **NOT in macOS**

**Conclusion**: ❌ **Not portable** - avoid

#### Option 3: Volatile Pointer Trick (Portable Fallback)

**Implementation**:
```c
static void secure_wipe(void *ptr, size_t len) {
    volatile unsigned char *p = ptr;
    while (len--) {
        *p++ = 0;
    }
}
```

**Availability**: ✅ Universal (works on all C compilers)
**Guarantee**: Volatile prevents optimization
**Downside**: Slightly slower than optimized memset

#### Option 4: Memory Barrier (Modern Compilers)

**GCC/Clang**:
```c
memset(password, 0, sizeof(password));
__asm__ __volatile__("" ::: "memory");  // Memory barrier
```

**MSVC**:
```c
memset(password, 0, sizeof(password));
_ReadWriteBarrier();
```

**Availability**: Compiler-specific
**Guarantee**: Prevents reordering/optimization

---

## Part 3: Recommended Portable Implementation

### Strategy: Feature Detection + Fallback

```c
// In a header (e.g., src/lle/secure_memory.h)

#ifndef LLE_SECURE_MEMORY_H
#define LLE_SECURE_MEMORY_H

#include <stddef.h>
#include <stdbool.h>

// Platform detection
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__) || \
    defined(__OpenBSD__) || defined(__NetBSD__)
    #define LLE_HAVE_MLOCK 1
    #include <sys/mman.h>
#else
    #define LLE_HAVE_MLOCK 0
#endif

// Secure wipe detection
#if defined(__OpenBSD__) || \
    (defined(__FreeBSD__) && __FreeBSD__ >= 11) || \
    (defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 25) || \
    (defined(__APPLE__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && \
     __MAC_OS_X_VERSION_MIN_REQUIRED >= 101300)
    #define LLE_HAVE_EXPLICIT_BZERO 1
    #if defined(__OpenBSD__) || defined(__FreeBSD__)
        #include <strings.h>
    #else
        #include <string.h>
    #endif
#else
    #define LLE_HAVE_EXPLICIT_BZERO 0
#endif

// Secure wipe implementation
static inline void lle_secure_wipe(void *ptr, size_t len) {
#if LLE_HAVE_EXPLICIT_BZERO
    explicit_bzero(ptr, len);
#else
    // Portable fallback using volatile
    volatile unsigned char *p = ptr;
    while (len--) {
        *p++ = 0;
    }
#endif
}

// Memory locking
static inline bool lle_memory_lock(void *addr, size_t len) {
#if LLE_HAVE_MLOCK
    return mlock(addr, len) == 0;
#else
    (void)addr;
    (void)len;
    return false;  // Not available on this platform
#endif
}

static inline bool lle_memory_unlock(void *addr, size_t len) {
#if LLE_HAVE_MLOCK
    return munlock(addr, len) == 0;
#else
    (void)addr;
    (void)len;
    return true;  // No-op, but return success
#endif
}

#endif // LLE_SECURE_MEMORY_H
```

### Usage in Spec 15 Minimal Secure Mode

```c
// In buffer management or wherever secure mode is needed

#include "secure_memory.h"

lle_result_t lle_buffer_enable_secure_mode(lle_buffer_t *buffer) {
    if (!buffer) return LLE_ERROR_NULL_PARAMETER;
    
    // Attempt to lock buffer memory
    if (!lle_memory_lock(buffer->data, buffer->capacity)) {
        // mlock failed - not fatal, continue with warning
        // (may fail due to privilege or platform)
        lle_log_warning("mlock failed - sensitive data may be swapped");
    }
    
    buffer->secure_mode = true;
    return LLE_SUCCESS;
}

lle_result_t lle_buffer_secure_clear(lle_buffer_t *buffer) {
    if (!buffer) return LLE_ERROR_NULL_PARAMETER;
    
    // Secure wipe (always works)
    lle_secure_wipe(buffer->data, buffer->used);
    
    buffer->used = 0;
    buffer->cursor = 0;
    
    return LLE_SUCCESS;
}

lle_result_t lle_buffer_disable_secure_mode(lle_buffer_t *buffer) {
    if (!buffer) return LLE_ERROR_NULL_PARAMETER;
    
    // Secure wipe before unlocking
    lle_secure_wipe(buffer->data, buffer->capacity);
    
    // Unlock memory
    lle_memory_unlock(buffer->data, buffer->capacity);
    
    buffer->secure_mode = false;
    return LLE_SUCCESS;
}
```

---

## Part 4: Platform Testing Matrix

| Platform | mlock | explicit_bzero | Fallback Needed? |
|----------|-------|----------------|------------------|
| Linux (glibc 2.25+) | ✅ Yes | ✅ Yes | No |
| Linux (glibc <2.25) | ✅ Yes | ❌ No | Yes (volatile) |
| macOS 10.13+ | ✅ Yes | ✅ Yes | No |
| macOS <10.13 | ✅ Yes | ❌ No | Yes (volatile) |
| FreeBSD 11+ | ✅ Yes | ✅ Yes | No |
| FreeBSD <11 | ✅ Yes | ❌ No | Yes (volatile) |
| OpenBSD 5.5+ | ✅ Yes | ✅ Yes | No |
| NetBSD | ✅ Yes | ❌ No | Yes (volatile) |
| Windows | ❌ No | ❌ No | Different API |

---

## Part 5: Limitations and Considerations

### mlock Limitations

1. **Privilege Requirements**
   - Small buffers (<64KB): Usually works without privilege
   - Large buffers: May require root or rlimit adjustment
   - Lusush passwords: ~256 bytes max, should always work

2. **Memory Limits**
   - System has limits on locked memory (RLIMIT_MEMLOCK)
   - Default on Linux: 64KB per process (non-root)
   - Lusush should stay well under this limit

3. **Not a Security Panacea**
   - Prevents swap to disk ✅
   - Does NOT prevent root from reading memory ❌
   - Does NOT prevent core dumps (need additional flags) ❌
   - Does NOT prevent debugger attachment ❌

### Secure Wipe Limitations

1. **Compiler Optimizations**
   - `explicit_bzero()`: Guaranteed safe
   - Volatile trick: Safe but slower
   - Plain `memset()`: May be optimized out ❌

2. **Memory Still Exists**
   - CPU caches may retain data
   - Memory controller buffers may retain data
   - Only zeroes RAM contents, not hardware caches

3. **Core Dump Issue**
   - Even with mlock, core dumps may contain locked memory
   - Need to disable core dumps for full protection:
   ```c
   #include <sys/resource.h>
   struct rlimit rlim = {0, 0};
   setrlimit(RLIMIT_CORE, &rlim);
   ```

---

## Part 6: Recommended Implementation Plan

### Minimal Secure Mode (Portable)

**3 Functions**:
1. `lle_buffer_enable_secure_mode()` - Lock memory with mlock (best effort)
2. `lle_buffer_secure_clear()` - Secure wipe with explicit_bzero/fallback
3. `lle_buffer_disable_secure_mode()` - Wipe + unlock

**Portability**:
- ✅ Linux: Full support
- ✅ macOS: Full support  
- ✅ FreeBSD/OpenBSD/NetBSD: Full support
- ⚠️ Windows: mlock gracefully fails, secure wipe still works
- ⚠️ Older systems: mlock works, volatile fallback for wipe

**Graceful Degradation**:
- If mlock fails: Log warning, continue (still get secure wipe)
- If explicit_bzero unavailable: Use volatile fallback
- If on Windows: Skip mlock, still do secure wipe

**Time to Implement**: 1-2 days
- Create secure_memory.h with feature detection
- Implement 3 functions with proper error handling
- Test on Linux (primary target)
- Document graceful degradation behavior

---

## Recommendations

### Answer to User's Question

**"Is mlock portable?"**

**YES** - `mlock()` is POSIX.1-2008 standard and works on:
- Linux ✅
- macOS ✅
- All BSDs ✅
- Other POSIX systems ✅
- Windows ❌ (but can gracefully degrade)

**"Will minimal secure mode work correctly?"**

**YES** - With proper feature detection and fallbacks:
- mlock: POSIX standard, works everywhere except Windows
- Secure wipe: Use explicit_bzero where available, volatile fallback elsewhere
- Graceful degradation: System still works if features unavailable

### Implementation Strategy

1. **Use feature detection** - Check for mlock and explicit_bzero at compile time
2. **Provide fallbacks** - Volatile-based secure wipe always works
3. **Graceful failure** - If mlock fails, log warning but continue (still get wipe benefit)
4. **Document behavior** - Clearly document what works on which platforms

### Comparison to Full Encryption

| Feature | Minimal Secure Mode | Full Encryption |
|---------|-------------------|-----------------|
| Portability | ✅ Excellent (POSIX) | ⚠️ Poor (needs crypto lib) |
| Dependencies | ✅ None | ❌ libsodium/OpenSSL |
| Complexity | ✅ Simple (~200 lines) | ❌ Complex (~2000+ lines) |
| Performance | ✅ Minimal overhead | ⚠️ Significant overhead |
| Swap protection | ✅ Yes (mlock) | ✅ Yes (if also use mlock) |
| Memory dump protection | ⚠️ Partial (clears quickly) | ✅ Full (encrypted) |

**Conclusion**: Minimal secure mode is **significantly more portable** than full encryption while still providing meaningful protection for the common case (preventing passwords from being swapped to disk).

---

## Final Answer

**To user's question**: 

✅ **YES, minimal secure mode is portable** across Linux, macOS, BSD, etc.

**Implementation approach**:
- Use POSIX mlock() where available (gracefully fail on Windows)
- Use explicit_bzero() where available, volatile fallback elsewhere
- All major platforms supported: Linux, macOS, FreeBSD, OpenBSD, NetBSD
- Graceful degradation on platforms without mlock (still get secure wipe)
- No external dependencies
- Self-contained and simple

**Time**: 1-2 days to implement properly with all feature detection and fallbacks
