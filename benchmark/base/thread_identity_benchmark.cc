//

#include <benchmark/benchmark.h>
#include <abel/thread/internal/thread_identity.h>
#include <abel/thread/internal/create_thread_identity.h>
#include <abel/thread/internal/per_thread_sem.h>

namespace {

    void BM_SafeCurrentThreadIdentity(benchmark::State &state) {
        for (auto _ : state) {
            benchmark::DoNotOptimize(
                    abel::thread_internal::GetOrCreateCurrentThreadIdentity());
        }
    }

    BENCHMARK(BM_SafeCurrentThreadIdentity);

    void BM_UnsafeCurrentThreadIdentity(benchmark::State &state) {
        for (auto _ : state) {
            benchmark::DoNotOptimize(
                    abel::thread_internal::CurrentThreadIdentityIfPresent());
        }
    }

    BENCHMARK(BM_UnsafeCurrentThreadIdentity);

}  // namespace
