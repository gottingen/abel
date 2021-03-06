// Copyright (c) 2021, gottingen group.
// All rights reserved.
// Created by liyinbin lijippy@163.com


#ifndef TEST_TESTING_LOG_INCLUDES_H_
#define TEST_TESTING_LOG_INCLUDES_H_

#include <cstdio>
#include <exception>
#include <fstream>
#include <ostream>
#include <string>
#include "testing/log_utils.h"

#define ABEL_LOG_TRACE_ON
#define ABEL_LOG_DEBUG_ON

#include "abel/log/async.h"
#include "abel/log/sinks/basic_file_sink.h"
#include "abel/log/sinks/daily_file_sink.h"
#include "abel/log/sinks/null_sink.h"
#include "abel/log/sinks/ostream_sink.h"
#include "abel/log/sinks/rotating_file_sink.h"
#include "abel/log/sinks/stdout_color_sinks.h"
#include "abel/log/log.h"
#include "gtest/gtest.h"

#endif//TEST_TESTING_LOG_INCLUDES_H_
