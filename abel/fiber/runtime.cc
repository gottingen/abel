//
// Created by liyinbin on 2021/4/5.
//


#include "abel/fiber/runtime.h"

#include <algorithm>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "abel/base/annotation.h"
#include "abel/base/random.h"
#include "abel/thread/numa.h"
#include "abel/fiber/internal/fiber_worker.h"
#include "abel/fiber/internal/scheduling_group.h"
#include "abel/fiber/internal/timer_worker.h"
#include "abel/strings/case_conv.h"
#include "abel/fiber/fiber_config.h"


namespace abel {

    namespace {

        // `scheduling_group` and its workers (both fiber worker and timer worker).
        struct scheduling_worker {
            int node_id;
            std::unique_ptr<fiber_internal::scheduling_group> scheduling_group;
            std::vector<std::unique_ptr<fiber_internal::fiber_worker>> fiber_workers;
            std::unique_ptr<fiber_internal::timer_worker> timer_worker;

            void start(bool no_cpu_migration) {
                timer_worker->start();
                for (auto &&e : fiber_workers) {
                    e->start(no_cpu_migration);
                }
            }

            void stop() {
                timer_worker->stop();
                scheduling_group->stop();
            }

            void join() {
                timer_worker->join();
                for (auto &&e : fiber_workers) {
                    e->join();
                }
            }
        };

        // Index by node ID. i.e., `scheduling_group[node][sg_index]`
        //
        // If `enable_numa_aware` is not set, `node` should always be 0.
        //
        // 64 nodes should be enough.
        std::vector<std::unique_ptr<scheduling_worker>> scheduling_groups[64];

        // This vector holds pointer to scheduling groups in `scheduling_groups`. It's
        // primarily used for randomly choosing a scheduling group or finding scheduling
        // group by ID.
        std::vector<scheduling_worker *> flatten_scheduling_groups;

        const std::vector<int> &GetFiberWorkerAccessibleCPUs();

        const std::vector<numa::numa_node> &GetFiberWorkerAccessibleNodes();

        // Call `f` in a thread with the specified affinity.
        //
        // This method helps you allocates resources from memory attached to one of the
        // CPUs listed in `affinity`, instead of the calling node (unless they're the
        // same).
        template<class F>
        void ExecuteWithAffinity(const std::vector<int> &affinity, F &&f) {
            // Dirty but works.
            //
            // TODO(yinbinli): Set & restore this thread's affinity to `affinity` (instead
            // of starting a new thread) to accomplish this.
            std::thread([&] {
                SetCurrentThreadAffinity(affinity);
                std::forward<F>(f)();
            }).join();
        }

        std::unique_ptr<scheduling_worker> CreateFullyFledgedSchedulingGroup(
                int node_id, const std::vector<int> &affinity, std::size_t size) {
            if (numa::support_affinity()) {
                DCHECK(!fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration ||
                       affinity.size() == size);
            }

            // TODO(yinbinli): Create these objects in a thread with affinity `affinity.
            auto rc = std::make_unique<scheduling_worker>();
            core_affinity aff = core_affinity::group_cores(node_id, affinity);
            rc->node_id = node_id;
            rc->scheduling_group =
                    std::make_unique<fiber_internal::scheduling_group>(aff, size);
            for (size_t i = 0; i != size; ++i) {
                rc->fiber_workers.push_back(
                        std::make_unique<fiber_internal::fiber_worker>(rc->scheduling_group.get(), i));
            }
            rc->timer_worker =
                    std::make_unique<fiber_internal::timer_worker>(rc->scheduling_group.get());
            rc->scheduling_group->set_timer_worker(rc->timer_worker.get());
            return rc;
        }

        // Add all scheduling groups in `victims` to fiber workers in `thieves`.
        //
        // Even if scheduling the thief is inside presents in `victims`, it won't be
        // added to the corresponding thief.
        void initialize_foreign_scheduling_groups(
                const std::vector<std::unique_ptr<scheduling_worker>> &thieves,
                const std::vector<std::unique_ptr<scheduling_worker>> &victims,
                std::uint64_t steal_every_n) {
            ABEL_UNUSED(steal_every_n);
            for (std::size_t thief = 0; thief != thieves.size(); ++thief) {
                for (std::size_t victim = 0; victim != victims.size(); ++victim) {
                    if (thieves[thief]->scheduling_group ==
                        victims[victim]->scheduling_group) {
                        return;
                    }
                }
            }
        }

        void StartWorkersUma() {
            DLOG_INFO(
                    "Starting {} worker threads per group, for a total of {} groups. The "
                    "system is treated as UMA.",
                    fiber_config::get_global_fiber_config().workers_per_group,
                    fiber_config::get_global_fiber_config().scheduling_groups);
            DLOG_WARN_IF(
                    fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration &&
                    GetFiberWorkerAccessibleNodes().size() > 1,
                    "CPU migration of fiber worker is disallowed, and we're trying to start "
                    "in UMA way on NUMA architecture. Performance will likely degrade.");

            for (std::size_t index = 0; index != fiber_config::get_global_fiber_config().scheduling_groups;
                 ++index) {
                if (!fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration) {
                    scheduling_groups[0].push_back(CreateFullyFledgedSchedulingGroup(
                            0 /* Not sigfinicant */, GetFiberWorkerAccessibleCPUs(),
                            fiber_config::get_global_fiber_config().workers_per_group));
                } else {
                    // Each group of processors is dedicated to a scheduling group.
                    //
                    // Later when we start the fiber workers, we'll instruct them to set their
                    // affinity to their dedicated processors.
                    auto &&cpus = GetFiberWorkerAccessibleCPUs();
                    DCHECK_LE((index + 1) * fiber_config::get_global_fiber_config().workers_per_group,
                              cpus.size());
                    scheduling_groups[0].push_back(CreateFullyFledgedSchedulingGroup(
                            0,
                            {cpus.begin() + index * fiber_config::get_global_fiber_config().workers_per_group,
                             cpus.begin() +
                             (index + 1) * fiber_config::get_global_fiber_config().workers_per_group},
                            fiber_config::get_global_fiber_config().workers_per_group));
                }
            }

            initialize_foreign_scheduling_groups(scheduling_groups[0], scheduling_groups[0],
                                                 fiber_config::get_global_fiber_config().work_stealing_ratio);
        }

        void StartWorkersNuma() {
            auto topo = GetFiberWorkerAccessibleNodes();
            DCHECK(topo.size() < std::size(scheduling_groups),
                       "Far more nodes that abel can support present on this "
                       "machine. Bail out.");

            auto groups_per_node = fiber_config::get_global_fiber_config().scheduling_groups / topo.size();
            DLOG_INFO(
                    "Starting {} worker threads per group, {} group per node, for a total of "
                    "{} nodes.",
                    fiber_config::get_global_fiber_config().workers_per_group, groups_per_node, topo.size());

            for (size_t i = 0; i != topo.size(); ++i) {
                for (size_t j = 0; j != groups_per_node; ++j) {
                    if (!fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration) {
                        auto &&affinity = topo[i].logical_cpus;
                        ExecuteWithAffinity(affinity, [&] {
                            scheduling_groups[i].push_back(CreateFullyFledgedSchedulingGroup(
                                    i, affinity, fiber_config::get_global_fiber_config().workers_per_group));
                        });
                    } else {
                        // Same as `StartWorkersUma()`, fiber worker's affinity is set upon
                        // start.
                        auto &&cpus = topo[i].logical_cpus;
                        DCHECK_LE((j + 1) * groups_per_node, cpus.size());
                        std::vector<int> affinity = {
                                cpus.begin() + j * fiber_config::get_global_fiber_config().workers_per_group,
                                cpus.begin() + (j + 1) * fiber_config::get_global_fiber_config().workers_per_group};
                        ExecuteWithAffinity(affinity, [&] {
                            scheduling_groups[i].push_back(CreateFullyFledgedSchedulingGroup(
                                    i, affinity, fiber_config::get_global_fiber_config().workers_per_group));
                        });
                    }
                }
            }

            for (size_t i = 0; i != topo.size(); ++i) {
                for (size_t j = 0; j != topo.size(); ++j) {
                    if (fiber_config::get_global_fiber_config().cross_numa_work_stealing_ratio == 0 && i != j) {
                        // Different NUMA domain.
                        //
                        // `enable_cross_numa_work_stealing` is not enabled, so we skip
                        // this pair.
                        continue;
                    }
                    initialize_foreign_scheduling_groups(
                            scheduling_groups[i], scheduling_groups[j],
                            i == j ? fiber_config::get_global_fiber_config().work_stealing_ratio
                                   : fiber_config::get_global_fiber_config().cross_numa_work_stealing_ratio);
                }
            }
        }

        std::vector<int> GetFiberWorkerAccessibleCPUsImpl() {
            DCHECK(fiber_config::get_global_fiber_config().fiber_worker_accessible_cpus.empty() ||
                               fiber_config::get_global_fiber_config().fiber_worker_inaccessible_cpus.empty(),
                       "At most one of `fiber_worker_accessible_cpus` or "
                       "`fiber_worker_inaccessible_cpus` may be specified.");
            if (!core_affinity::supported) {
                return std::vector<int>();
            }
            // If user specified accessible CPUs explicitly.
            if (!fiber_config::get_global_fiber_config().fiber_worker_accessible_cpus.empty()) {
                auto procs = TryParseProcesserList(
                        fiber_config::get_global_fiber_config().fiber_worker_accessible_cpus);
                DCHECK(procs, "Failed to parse `fiber_worker_accessible_cpus`.");
                return *procs;
            }

            // If affinity is set on the process, show our respect.
            //
            // Note that we don't have to do some dirty trick to check if processors we
            // get from affinity is accessible to us -- Inaccessible CPUs shouldn't be
            // returned to us in the first place.
            auto accessible_cpus = GetCurrentThreadAffinity();
            DCHECK(!accessible_cpus.empty());

            // If user specified inaccessible CPUs explicitly.
            if (!fiber_config::get_global_fiber_config().fiber_worker_inaccessible_cpus.empty()) {
                auto option = TryParseProcesserList(
                        fiber_config::get_global_fiber_config().fiber_worker_inaccessible_cpus);
                DCHECK(option,
                           "Failed to parse `fiber_worker_inaccessible_cpus`.");
                std::set<int> inaccessible(option->begin(), option->end());

                // Drop processors we're forbidden to access.
                accessible_cpus.erase(
                        std::remove_if(accessible_cpus.begin(), accessible_cpus.end(),
                                       [&](auto &&x) { return inaccessible.count(x) != 0; }),
                        accessible_cpus.end());
                return accessible_cpus;
            }

            // Thread affinity is respected implicitly.
            return accessible_cpus;
        }

        const std::vector<int> &GetFiberWorkerAccessibleCPUs() {
            static auto result = GetFiberWorkerAccessibleCPUsImpl();
            return result;
        }

        const std::vector<numa::numa_node> &GetFiberWorkerAccessibleNodes() {
            static std::vector<numa::numa_node> result =
#ifdef ABEL_PLATFORM_LINUX
             [] {
                std::map<int, std::vector<int>> node_to_processor;
                for (auto&& e : GetFiberWorkerAccessibleCPUs()) {
                    auto n = numa::get_node_of_processor(e);
                    node_to_processor[n].push_back(e);
                }

                std::vector<numa::numa_node> result;
                for (auto&& [k, v] : node_to_processor) {
                    result.push_back({k, v});
                }
                return result;
            }();
#else
            [] {
                return std::vector<numa::numa_node>();
            }();

#endif
            return result;
        }

    }  // namespace

    namespace fiber_internal {

        std::size_t get_current_scheduling_groupIndex_slow() {
            auto rc = fiber_internal::nearest_scheduling_group_index();
            DCHECK(rc != -1,
                       "Calling `get_current_scheduling_group_index` outside of any "
                       "scheduling group is undefined.");
            return rc;
        }

    }  // namespace fiber_internal

    void start_runtime() {
        // Get our final decision for scheduling parameters.
        //fiber_internal::InitializeSchedulingParametersFromFlags();
        // If CPU migration is explicit disallowed, we need to make sure there are
        // enough CPUs for us.
        //DisallowProcessorMigrationPreconditionCheck();
        DLOG_INFO("enable_numa_affinity: {}", fiber_config::get_global_fiber_config().enable_numa_aware);
        if (fiber_config::get_global_fiber_config().enable_numa_aware) {
            StartWorkersNuma();
        } else {
            StartWorkersUma();
        }


        // Fill `flatten_scheduling_groups`.
        for (auto &&e : scheduling_groups) {
            for (auto &&ee : e) {
                flatten_scheduling_groups.push_back(ee.get());
            }
        }

        // Start the workers.
        for (auto &&e : scheduling_groups) {
            for (auto &&ee : e) {
                ee->start(fiber_config::get_global_fiber_config().fiber_worker_disallow_cpu_migration);
            }
        }

    }

    void terminate_runtime() {
        for (auto &&e : scheduling_groups) {
            for (auto &&ee : e) {
                ee->stop();
            }
        }
        for (auto &&e : scheduling_groups) {
            for (auto &&ee : e) {
                ee->join();
            }
        }
        for (auto &&e : scheduling_groups) {
            e.clear();
        }
        flatten_scheduling_groups.clear();
    }

    std::size_t get_scheduling_group_count() {
        return flatten_scheduling_groups.size();
    }

    std::size_t get_scheduling_group_size() {
        return fiber_config::get_global_fiber_config().workers_per_group;
    }

    int get_scheduling_group_assigned_node(std::size_t sg_index) {
        DCHECK_LT(sg_index, flatten_scheduling_groups.size());
        return flatten_scheduling_groups[sg_index]->node_id;
    }

    namespace fiber_internal {

        scheduling_group *routine_get_scheduling_group(std::size_t index) {
            DCHECK_LT(index, flatten_scheduling_groups.size());
            return flatten_scheduling_groups[index]->scheduling_group.get();
        }

        scheduling_group *nearest_scheduling_group_slow(scheduling_group **cache) {
            if (auto rc = scheduling_group::current()) {
                // Only if we indeed belong to the scheduling group (in which case the
                // "nearest" scheduling group never changes) we fill the cache.
                *cache = rc;
                return rc;
            }

            // We don't pay for overhead of initialize `next` unless we're not in running
            // fiber worker.
            ABEL_INTERNAL_TLS_MODEL thread_local std::size_t next = Random();

            auto &&current_node =
                    scheduling_groups[fiber_config::get_global_fiber_config().enable_numa_aware
                                      ? numa::get_current_node()
                                      : 0];
            if (!current_node.empty()) {
                return current_node[next++ % current_node.size()]->scheduling_group.get();
            }

            if (!flatten_scheduling_groups.empty()) {
                return flatten_scheduling_groups[next++ % flatten_scheduling_groups.size()]
                        ->scheduling_group.get();
            }

            return nullptr;
        }

        std::ptrdiff_t nearest_scheduling_group_index() {
            ABEL_INTERNAL_TLS_MODEL thread_local auto cached = []() -> std::ptrdiff_t {
                auto sg = nearest_scheduling_group();
                if (sg) {
                    auto iter = std::find_if(
                            flatten_scheduling_groups.begin(), flatten_scheduling_groups.end(),
                            [&](auto &&e) { return e->scheduling_group.get() == sg; });
                    DCHECK(iter != flatten_scheduling_groups.end());
                    return iter - flatten_scheduling_groups.begin();
                }
                return -1;
            }();
            return cached;
        }

    }  // namespace fiber_internal

}  // namespace abel
