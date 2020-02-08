//
// -----------------------------------------------------------------------------
// File: failure_signal_handler.h
// -----------------------------------------------------------------------------
//
// This file configures the abel *failure signal handler* to capture and dump
// useful debugging information (such as a stacktrace) upon program failure.
//
// To use the failure signal handler, call `abel::InstallFailureSignalHandler()`
// very early in your program, usually in the first few lines of main():
//
// int main(int argc, char** argv) {
//   // Initialize the symbolizer to get a human-readable stack trace
//   abel::InitializeSymbolizer(argv[0]);
//
//   abel::FailureSignalHandlerOptions options;
//   abel::InstallFailureSignalHandler(options);
//   DoSomethingInteresting();
//   return 0;
// }
//
// Any program that raises a fatal signal (such as `SIGSEGV`, `SIGILL`,
// `SIGFPE`, `SIGABRT`, `SIGTERM`, `SIGBUG`, and `SIGTRAP`) will call the
// installed failure signal handler and provide debugging information to stderr.
//
// Note that you should *not* install the abel failure signal handler more
// than once. You may, of course, have another (non-abel) failure signal
// handler installed (which would be triggered if abel's failure signal
// handler sets `call_previous_handler` to `true`).

#ifndef ABEL_DEBUGGING_FAILURE_SIGNAL_HANDLER_H_
#define ABEL_DEBUGGING_FAILURE_SIGNAL_HANDLER_H_

#include <abel/base/profile.h>

namespace abel {


// FailureSignalHandlerOptions
//
// Struct for holding `abel::InstallFailureSignalHandler()` configuration
// options.
struct FailureSignalHandlerOptions {
  // If true, try to symbolize the stacktrace emitted on failure, provided that
  // you have initialized a symbolizer for that purpose. (See symbolize.h for
  // more information.)
  bool symbolize_stacktrace = true;

  // If true, try to run signal handlers on an alternate stack (if supported on
  // the given platform). An alternate stack is useful for program crashes due
  // to a stack overflow; by running on a alternate stack, the signal handler
  // may run even when normal stack space has been exausted. The downside of
  // using an alternate stack is that extra memory for the alternate stack needs
  // to be pre-allocated.
  bool use_alternate_stack = true;

  // If positive, indicates the number of seconds after which the failure signal
  // handler is invoked to abort the program. Setting such an alarm is useful in
  // cases where the failure signal handler itself may become hung or
  // deadlocked.
  int alarm_on_failure_secs = 3;

  // If true, call the previously registered signal handler for the signal that
  // was received (if one was registered) after the existing signal handler
  // runs. This mechanism can be used to chain signal handlers together.
  //
  // If false, the signal is raised to the default handler for that signal
  // (which normally terminates the program).
  //
  // IMPORTANT: If true, the chained fatal signal handlers must not try to
  // recover from the fatal signal. Instead, they should terminate the program
  // via some mechanism, like raising the default handler for the signal, or by
  // calling `_exit()`. Note that the failure signal handler may put parts of
  // the abel library into a state from which they cannot recover.
  bool call_previous_handler = false;

  // If non-null, indicates a pointer to a callback function that will be called
  // upon failure, with a std::string argument containing failure data. This function
  // may be used as a hook to write failure data to a secondary location, such
  // as a log file. This function may also be called with null data, as a hint
  // to flush any buffered data before the program may be terminated. Consider
  // flushing any buffered data in all calls to this function.
  //
  // Since this function runs within a signal handler, it should be
  // async-signal-safe if possible.
  // See http://man7.org/linux/man-pages/man7/signal-safety.7.html
  void (*writerfn)(const char*) = nullptr;
};

// InstallFailureSignalHandler()
//
// Installs a signal handler for the common failure signals `SIGSEGV`, `SIGILL`,
// `SIGFPE`, `SIGABRT`, `SIGTERM`, `SIGBUG`, and `SIGTRAP` (provided they exist
// on the given platform). The failure signal handler dumps program failure data
// useful for debugging in an unspecified format to stderr. This data may
// include the program counter, a stacktrace, and register information on some
// systems; do not rely on an exact format for the output, as it is subject to
// change.
void InstallFailureSignalHandler(const FailureSignalHandlerOptions& options);

namespace debugging_internal {
const char* FailureSignalToString(int signo);
}  // namespace debugging_internal


}  // namespace abel

#endif  // ABEL_DEBUGGING_FAILURE_SIGNAL_HANDLER_H_
