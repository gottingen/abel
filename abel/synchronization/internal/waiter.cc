//

#include <abel/synchronization/internal/waiter.h>

#include <abel/base/profile.h>

#ifdef _WIN32
#include <windows.h>
#else

#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

#endif

#ifdef __linux__
#include <linux/futex.h>
#include <sys/syscall.h>
#endif

#ifdef ABEL_HAVE_SEMAPHORE_H

#include <semaphore.h>

#endif

#include <errno.h>
#include <stdio.h>
#include <time.h>

#include <atomic>
#include <cassert>
#include <cstdint>
#include <new>
#include <type_traits>

#include <abel/log/raw_logging.h>
#include <abel/threading/internal/thread_identity.h>
#include <abel/base/profile.h>
#include <abel/synchronization/internal/kernel_timeout.h>

namespace abel {

    namespace synchronization_internal {

        static void MaybeBecomeIdle() {
            threading_internal::ThreadIdentity *identity =
                    threading_internal::CurrentThreadIdentityIfPresent();
            assert(identity != nullptr);
            const bool is_idle = identity->is_idle.load(std::memory_order_relaxed);
            const int ticker = identity->ticker.load(std::memory_order_relaxed);
            const int wait_start = identity->wait_start.load(std::memory_order_relaxed);
            if (!is_idle && ticker - wait_start > Waiter::kIdlePeriods) {
                identity->is_idle.store(true, std::memory_order_relaxed);
            }
        }

#if ABEL_WAITER_MODE == ABEL_WAITER_MODE_FUTEX

        // Some Android headers are missing these definitions even though they
        // support these futex operations.
#ifdef __BIONIC__
#ifndef SYS_futex
#define SYS_futex __NR_futex
#endif
#ifndef FUTEX_WAIT_BITSET
#define FUTEX_WAIT_BITSET 9
#endif
#ifndef FUTEX_PRIVATE_FLAG
#define FUTEX_PRIVATE_FLAG 128
#endif
#ifndef FUTEX_CLOCK_REALTIME
#define FUTEX_CLOCK_REALTIME 256
#endif
#ifndef FUTEX_BITSET_MATCH_ANY
#define FUTEX_BITSET_MATCH_ANY 0xFFFFFFFF
#endif
#endif

        class Futex {
         public:
          static int WaitUntil(std::atomic<int32_t> *v, int32_t val,
                               KernelTimeout t) {
            int err = 0;
            if (t.has_timeout()) {
              // https://locklessinc.com/articles/futex_cheat_sheet/
              // Unlike FUTEX_WAIT, FUTEX_WAIT_BITSET uses absolute time.
              struct timespec abs_timeout = t.MakeAbsTimespec();
              // Atomically check that the futex value is still 0, and if it
              // is, sleep until abs_timeout or until woken by FUTEX_WAKE.
              err = syscall(
                  SYS_futex, reinterpret_cast<int32_t *>(v),
                  FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG | FUTEX_CLOCK_REALTIME, val,
                  &abs_timeout, nullptr, FUTEX_BITSET_MATCH_ANY);
            } else {
              // Atomically check that the futex value is still 0, and if it
              // is, sleep until woken by FUTEX_WAKE.
              err = syscall(SYS_futex, reinterpret_cast<int32_t *>(v),
                            FUTEX_WAIT | FUTEX_PRIVATE_FLAG, val, nullptr);
            }
            if (err != 0) {
              err = -errno;
            }
            return err;
          }

          static int Wake(std::atomic<int32_t> *v, int32_t count) {
            int err = syscall(SYS_futex, reinterpret_cast<int32_t *>(v),
                              FUTEX_WAKE | FUTEX_PRIVATE_FLAG, count);
            if (ABEL_UNLIKELY(err < 0)) {
              err = -errno;
            }
            return err;
          }
        };

        Waiter::Waiter() {
          futex_.store(0, std::memory_order_relaxed);
        }

        Waiter::~Waiter() = default;

        bool Waiter::wait(KernelTimeout t) {
          // Loop until we can atomically decrement futex from a positive
          // value, waiting on a futex while we believe it is zero.
          // Note that, since the thread ticker is just reset, we don't need to check
          // whether the thread is idle on the very first pass of the loop.
          bool first_pass = true;
          while (true) {
            int32_t x = futex_.load(std::memory_order_relaxed);
            while (x != 0) {
              if (!futex_.compare_exchange_weak(x, x - 1,
                                                std::memory_order_acquire,
                                                std::memory_order_relaxed)) {
                continue;  // Raced with someone, retry.
              }
              return true;  // Consumed a wakeup, we are done.
            }


            if (!first_pass) MaybeBecomeIdle();
            const int err = Futex::WaitUntil(&futex_, 0, t);
            if (err != 0) {
              if (err == -EINTR || err == -EWOULDBLOCK) {
                // Do nothing, the loop will retry.
              } else if (err == -ETIMEDOUT) {
                return false;
              } else {
                ABEL_RAW_LOG(FATAL, "Futex operation failed with error %d\n", err);
              }
            }
            first_pass = false;
          }
        }

        void Waiter::Post() {
          if (futex_.fetch_add(1, std::memory_order_release) == 0) {
            // We incremented from 0, need to wake a potential waiter.
            Poke();
          }
        }

        void Waiter::Poke() {
          // Wake one thread waiting on the futex.
          const int err = Futex::Wake(&futex_, 1);
          if (ABEL_UNLIKELY(err < 0)) {
            ABEL_RAW_LOG(FATAL, "Futex operation failed with error %d\n", err);
          }
        }

#elif ABEL_WAITER_MODE == ABEL_WAITER_MODE_CONDVAR

        class PthreadMutexHolder {
        public:
            explicit PthreadMutexHolder(pthread_mutex_t *mu) : mu_(mu) {
                const int err = pthread_mutex_lock(mu_);
                if (err != 0) {
                    ABEL_RAW_LOG(FATAL, "pthread_mutex_lock failed: %d", err);
                }
            }

            PthreadMutexHolder(const PthreadMutexHolder &rhs) = delete;

            PthreadMutexHolder &operator=(const PthreadMutexHolder &rhs) = delete;

            ~PthreadMutexHolder() {
                const int err = pthread_mutex_unlock(mu_);
                if (err != 0) {
                    ABEL_RAW_LOG(FATAL, "pthread_mutex_unlock failed: %d", err);
                }
            }

        private:
            pthread_mutex_t *mu_;
        };

        Waiter::Waiter() {
            const int err = pthread_mutex_init(&mu_, 0);
            if (err != 0) {
                ABEL_RAW_LOG(FATAL, "pthread_mutex_init failed: %d", err);
            }

            const int err2 = pthread_cond_init(&cv_, 0);
            if (err2 != 0) {
                ABEL_RAW_LOG(FATAL, "pthread_cond_init failed: %d", err2);
            }

            waiter_count_ = 0;
            wakeup_count_ = 0;
        }

        Waiter::~Waiter() {
            const int err = pthread_mutex_destroy(&mu_);
            if (err != 0) {
                ABEL_RAW_LOG(FATAL, "pthread_mutex_destroy failed: %d", err);
            }

            const int err2 = pthread_cond_destroy(&cv_);
            if (err2 != 0) {
                ABEL_RAW_LOG(FATAL, "pthread_cond_destroy failed: %d", err2);
            }
        }

        bool Waiter::wait(KernelTimeout t) {
            struct timespec abs_timeout;
            if (t.has_timeout()) {
                abs_timeout = t.MakeAbsTimespec();
            }

            PthreadMutexHolder h(&mu_);
            ++waiter_count_;
            // Loop until we find a wakeup to consume or timeout.
            // Note that, since the thread ticker is just reset, we don't need to check
            // whether the thread is idle on the very first pass of the loop.
            bool first_pass = true;
            while (wakeup_count_ == 0) {
                if (!first_pass)
                    MaybeBecomeIdle();
                // No wakeups available, time to wait.
                if (!t.has_timeout()) {
                    const int err = pthread_cond_wait(&cv_, &mu_);
                    if (err != 0) {
                        ABEL_RAW_LOG(FATAL, "pthread_cond_wait failed: %d", err);
                    }
                } else {
                    const int err = pthread_cond_timedwait(&cv_, &mu_, &abs_timeout);
                    if (err == ETIMEDOUT) {
                        --waiter_count_;
                        return false;
                    }
                    if (err != 0) {
                        ABEL_RAW_LOG(FATAL, "pthread_cond_timedwait failed: %d", err);
                    }
                }
                first_pass = false;
            }
            // Consume a wakeup and we're done.
            --wakeup_count_;
            --waiter_count_;
            return true;
        }

        void Waiter::Post() {
            PthreadMutexHolder h(&mu_);
            ++wakeup_count_;
            InternalCondVarPoke();
        }

        void Waiter::Poke() {
            PthreadMutexHolder h(&mu_);
            InternalCondVarPoke();
        }

        void Waiter::InternalCondVarPoke() {
            if (waiter_count_ != 0) {
                const int err = pthread_cond_signal(&cv_);
                if (ABEL_UNLIKELY(err != 0)) {
                    ABEL_RAW_LOG(FATAL, "pthread_cond_signal failed: %d", err);
                }
            }
        }

#elif ABEL_WAITER_MODE == ABEL_WAITER_MODE_SEM

        Waiter::Waiter () {
            if (sem_init(&sem_, 0, 0) != 0) {
                ABEL_RAW_LOG(FATAL, "sem_init failed with errno %d\n", errno);
            }
            wakeups_.store(0, std::memory_order_relaxed);
        }

        Waiter::~Waiter () {
            if (sem_destroy(&sem_) != 0) {
                ABEL_RAW_LOG(FATAL, "sem_destroy failed with errno %d\n", errno);
            }
        }

        bool Waiter::wait (KernelTimeout t) {
            struct timespec abs_timeout;
            if (t.has_timeout()) {
                abs_timeout = t.MakeAbsTimespec();
            }

            // Loop until we timeout or consume a wakeup.
            // Note that, since the thread ticker is just reset, we don't need to check
            // whether the thread is idle on the very first pass of the loop.
            bool first_pass = true;
            while (true) {
                int x = wakeups_.load(std::memory_order_relaxed);
                while (x != 0) {
                    if (!wakeups_.compare_exchange_weak(x, x - 1,
                                                        std::memory_order_acquire,
                                                        std::memory_order_relaxed)) {
                        continue;  // Raced with someone, retry.
                    }
                    // Successfully consumed a wakeup, we're done.
                    return true;
                }

                if (!first_pass)
                    MaybeBecomeIdle();
                // Nothing to consume, wait (looping on EINTR).
                while (true) {
                    if (!t.has_timeout()) {
                        if (sem_wait(&sem_) == 0)
                            break;
                        if (errno == EINTR)
                            continue;
                        ABEL_RAW_LOG(FATAL, "sem_wait failed: %d", errno);
                    } else {
                        if (sem_timedwait(&sem_, &abs_timeout) == 0)
                            break;
                        if (errno == EINTR)
                            continue;
                        if (errno == ETIMEDOUT)
                            return false;
                        ABEL_RAW_LOG(FATAL, "sem_timedwait failed: %d", errno);
                    }
                }
                first_pass = false;
            }
        }

        void Waiter::Post () {
            // Post a wakeup.
            if (wakeups_.fetch_add(1, std::memory_order_release) == 0) {
                // We incremented from 0, need to wake a potential waiter.
                Poke();
            }
        }

        void Waiter::Poke () {
            if (sem_post(&sem_) != 0) {  // Wake any semaphore waiter.
                ABEL_RAW_LOG(FATAL, "sem_post failed with errno %d\n", errno);
            }
        }

#elif ABEL_WAITER_MODE == ABEL_WAITER_MODE_WIN32

        class Waiter::WinHelper {
         public:
          static SRWLOCK *GetLock(Waiter *w) {
            return reinterpret_cast<SRWLOCK *>(&w->mu_storage_);
          }

          static CONDITION_VARIABLE *GetCond(Waiter *w) {
            return reinterpret_cast<CONDITION_VARIABLE *>(&w->cv_storage_);
          }

          static_assert(sizeof(SRWLOCK) == sizeof(Waiter::SRWLockStorage),
                        "SRWLockStorage does not have the same size as SRWLOCK");
          static_assert(
              alignof(SRWLOCK) == alignof(Waiter::SRWLockStorage),
              "SRWLockStorage does not have the same alignment as SRWLOCK");

          static_assert(sizeof(CONDITION_VARIABLE) ==
                            sizeof(Waiter::ConditionVariableStorage),
                        "ABEL_CONDITION_VARIABLE_STORAGE does not have the same size "
                        "as CONDITION_VARIABLE");
          static_assert(alignof(CONDITION_VARIABLE) ==
                            alignof(Waiter::ConditionVariableStorage),
                        "ConditionVariableStorage does not have the same "
                        "alignment as CONDITION_VARIABLE");

          // The SRWLOCK and CONDITION_VARIABLE types must be trivially constructible
          // and destructible because we never call their constructors or destructors.
          static_assert(std::is_trivially_constructible<SRWLOCK>::value,
                        "The SRWLOCK type must be trivially constructible");
          static_assert(std::is_trivially_constructible<CONDITION_VARIABLE>::value,
                        "The CONDITION_VARIABLE type must be trivially constructible");
          static_assert(std::is_trivially_destructible<SRWLOCK>::value,
                        "The SRWLOCK type must be trivially destructible");
          static_assert(std::is_trivially_destructible<CONDITION_VARIABLE>::value,
                        "The CONDITION_VARIABLE type must be trivially destructible");
        };

        class LockHolder {
         public:
          explicit LockHolder(SRWLOCK* mu) : mu_(mu) {
            AcquireSRWLockExclusive(mu_);
          }

          LockHolder(const LockHolder&) = delete;
          LockHolder& operator=(const LockHolder&) = delete;

          ~LockHolder() {
            ReleaseSRWLockExclusive(mu_);
          }

         private:
          SRWLOCK* mu_;
        };

        Waiter::Waiter() {
          auto *mu = ::new (static_cast<void *>(&mu_storage_)) SRWLOCK;
          auto *cv = ::new (static_cast<void *>(&cv_storage_)) CONDITION_VARIABLE;
          InitializeSRWLock(mu);
          InitializeConditionVariable(cv);
          waiter_count_ = 0;
          wakeup_count_ = 0;
        }

        // SRW locks and condition variables do not need to be explicitly destroyed.
        // https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializesrwlock
        // https://stackoverflow.com/questions/28975958/why-does-windows-have-no-deleteconditionvariable-function-to-go-together-with
        Waiter::~Waiter() = default;

        bool Waiter::wait(KernelTimeout t) {
          SRWLOCK *mu = WinHelper::GetLock(this);
          CONDITION_VARIABLE *cv = WinHelper::GetCond(this);

          LockHolder h(mu);
          ++waiter_count_;

          // Loop until we find a wakeup to consume or timeout.
          // Note that, since the thread ticker is just reset, we don't need to check
          // whether the thread is idle on the very first pass of the loop.
          bool first_pass = true;
          while (wakeup_count_ == 0) {
            if (!first_pass) MaybeBecomeIdle();
            // No wakeups available, time to wait.
            if (!SleepConditionVariableSRW(cv, mu, t.InMillisecondsFromNow(), 0)) {
              // GetLastError() returns a Win32 DWORD, but we assign to
              // unsigned long to simplify the ABEL_RAW_LOG case below.  The uniform
              // initialization guarantees this is not a narrowing conversion.
              const unsigned long err{GetLastError()};  // NOLINT(runtime/int)
              if (err == ERROR_TIMEOUT) {
                --waiter_count_;
                return false;
              } else {
                ABEL_RAW_LOG(FATAL, "SleepConditionVariableSRW failed: %lu", err);
              }
            }
            first_pass = false;
          }
          // Consume a wakeup and we're done.
          --wakeup_count_;
          --waiter_count_;
          return true;
        }

        void Waiter::Post() {
          LockHolder h(WinHelper::GetLock(this));
          ++wakeup_count_;
          InternalCondVarPoke();
        }

        void Waiter::Poke() {
          LockHolder h(WinHelper::GetLock(this));
          InternalCondVarPoke();
        }

        void Waiter::InternalCondVarPoke() {
          if (waiter_count_ != 0) {
            WakeConditionVariable(WinHelper::GetCond(this));
          }
        }

#else
#error Unknown ABEL_WAITER_MODE
#endif

    }  // namespace synchronization_internal

}  // namespace abel
