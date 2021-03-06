// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#include "abel/metrics/counter.h"
#include "abel/metrics/metrics_type.h"

namespace abel {
namespace metrics {

void counter::inc() {
    _gauge.inc();
}

void counter::inc(const double val) {
    _gauge.inc(val);
}

double counter::value() const {
    return _gauge.value();
}

cache_metrics counter::collect() const noexcept {
    cache_metrics cm;
    cm.counter.value = value();
    cm.type = metrics_type::mt_counter;
    return cm;
}

}  // namespace metrics
}  // namespace abel
