//

// The OS-specific header included below must provide two calls:
// AbelInternalSpinLockDelay() and AbelInternalSpinLockWake().
// See spinlock_wait.h for the specs.

#include <atomic>
#include <cstdint>

#include <abel/threading/internal/spinlock_wait.h>

#if defined(_WIN32)
#include <abel/threading/internal/spinlock_win32.inc>
#elif defined(__linux__)
#include <abel/threading/internal/spinlock_linux.inc>
#elif defined(__akaros__)
#include <abel/threading/internal/spinlock_akaros.inc>
#else

#include <abel/threading/internal/spinlock_posix.inc>

#endif

namespace abel {

    namespace threading_internal {

// See spinlock_wait.h for spec.
        uint32_t SpinLockWait(std::atomic<uint32_t> *w, int n,
                              const SpinLockWaitTransition trans[],
                              threading_internal::SchedulingMode scheduling_mode) {
            int loop = 0;
            for (;;) {
                uint32_t v = w->load(std::memory_order_acquire);
                int i;
                for (i = 0; i != n && v != trans[i].from; i++) {
                }
                if (i == n) {
                    SpinLockDelay(w, v, ++loop, scheduling_mode);  // no matching transition
                } else if (trans[i].to == v ||                   // null transition
                           w->compare_exchange_strong(v, trans[i].to,
                                                      std::memory_order_acquire,
                                                      std::memory_order_relaxed)) {
                    if (trans[i].done) return v;
                }
            }
        }

        static std::atomic<uint64_t> delay_rand;

// Return a suggested delay in nanoseconds for iteration number "loop"
        int SpinLockSuggestedDelayNS(int loop) {
            // Weak pseudo-random number generator to get some spread between threads
            // when many are spinning.
            uint64_t r = delay_rand.load(std::memory_order_relaxed);
            r = 0x5deece66dLL * r + 0xb;   // numbers from nrand48()
            delay_rand.store(r, std::memory_order_relaxed);

            if (loop < 0 || loop > 32) {   // limit loop to 0..32
                loop = 32;
            }
            const int kMinDelay = 128 << 10;  // 128us
            // Double delay every 8 iterations, up to 16x (2ms).
            int delay = kMinDelay << (loop / 8);
            // Randomize in delay..2*delay range, for resulting 128us..4ms range.
            return delay | ((delay - 1) & static_cast<int>(r));
        }

    }  // namespace threading_internal

}  // namespace abel
