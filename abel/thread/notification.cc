//

#include <abel/thread/notification.h>

#include <atomic>

#include <abel/base/profile.h>
#include <abel/log/abel_logging.h>
#include <abel/thread/mutex.h>
#include <abel/chrono/time.h>

namespace abel {


    void notification::Notify() {
        mutex_lock l(&this->mutex_);

#ifndef NDEBUG
        if (ABEL_UNLIKELY(notified_yet_.load(std::memory_order_relaxed))) {
            ABEL_RAW_CRITICAL("Notify() method called more than once for notification object {:p}",
                    static_cast<void *>(this));
        }
#endif

        notified_yet_.store(true, std::memory_order_release);
    }

    notification::~notification() {
        // Make sure that the thread running Notify() exits before the object is
        // destructed.
        mutex_lock l(&this->mutex_);
    }

    void notification::wait_for_notification() const {
        if (!has_been_notified_internal(&this->notified_yet_)) {
            this->mutex_.LockWhen(condition(&has_been_notified_internal,
                                            &this->notified_yet_));
            this->mutex_.unlock();
        }
    }

    bool notification::wait_for_notification_with_timeout(
            abel::duration timeout) const {
        bool notified = has_been_notified_internal(&this->notified_yet_);
        if (!notified) {
            notified = this->mutex_.LockWhenWithTimeout(
                    condition(&has_been_notified_internal, &this->notified_yet_), timeout);
            this->mutex_.unlock();
        }
        return notified;
    }

    bool notification::wait_for_notification_with_deadline(abel::abel_time deadline) const {
        bool notified = has_been_notified_internal(&this->notified_yet_);
        if (!notified) {
            notified = this->mutex_.LockWhenWithDeadline(
                    condition(&has_been_notified_internal, &this->notified_yet_), deadline);
            this->mutex_.unlock();
        }
        return notified;
    }


}  // namespace abel
