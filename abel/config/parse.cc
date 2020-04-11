//
//

#include <abel/config/parse.h>

#include <stdlib.h>

#include <fstream>
#include <iostream>
#include <tuple>

#ifdef _WIN32
#include <windows.h>
#endif

#include <abel/config/flag.h>
#include <abel/config/internal/program_name.h>
#include <abel/config/internal/registry.h>
#include <abel/config/internal/usage.h>
#include <abel/config/usage.h>
#include <abel/config/usage_config.h>
#include <abel/strings/str_cat.h>
#include <abel/strings/strip.h>
#include <abel/strings/trim.h>
#include <abel/thread/mutex.h>


// --------------------------------------------------------------------

namespace abel {

    namespace flags_internal {
        namespace {

            ABEL_CONST_INIT abel::mutex processing_checks_guard(abel::kConstInit);

            ABEL_CONST_INIT bool flagfile_needs_processing
                    ABEL_GUARDED_BY(processing_checks_guard) = false;
            ABEL_CONST_INIT bool fromenv_needs_processing
                    ABEL_GUARDED_BY(processing_checks_guard) = false;
            ABEL_CONST_INIT bool tryfromenv_needs_processing
                    ABEL_GUARDED_BY(processing_checks_guard) = false;

        }  // namespace
    }  // namespace flags_internal

}  // namespace abel

ABEL_FLAG(std::vector<std::string>, flagfile, {},
          "comma-separated list of files to load flags from")
.OnUpdate([]() {
    if (abel::get_flag(FLAGS_flagfile).empty()) return;

    abel::mutex_lock l(&abel::flags_internal::processing_checks_guard);

    // Setting this flag twice before it is handled most likely an internal
    // error and should be reviewed by developers.
    if (abel::flags_internal::flagfile_needs_processing) {
        ABEL_INTERNAL_LOG(WARNING, "flagfile set twice before it is handled");
    }

    abel::flags_internal::flagfile_needs_processing = true;
});
ABEL_FLAG(std::vector<std::string>, fromenv, {},
          "comma-separated list of flags to set from the environment"
          " [use 'export FLAGS_flag1=value']")
.OnUpdate([]() {
    if (abel::get_flag(FLAGS_fromenv).empty()) return;

    abel::mutex_lock l(&abel::flags_internal::processing_checks_guard);

    // Setting this flag twice before it is handled most likely an internal
    // error and should be reviewed by developers.
    if (abel::flags_internal::fromenv_needs_processing) {
        ABEL_INTERNAL_LOG(WARNING, "fromenv set twice before it is handled.");
    }

    abel::flags_internal::fromenv_needs_processing = true;
});
ABEL_FLAG(std::vector<std::string>, tryfromenv, {},
          "comma-separated list of flags to try to set from the environment if "
          "present")
.OnUpdate([]() {
    if (abel::get_flag(FLAGS_tryfromenv).empty()) return;

    abel::mutex_lock l(&abel::flags_internal::processing_checks_guard);

    // Setting this flag twice before it is handled most likely an internal
    // error and should be reviewed by developers.
    if (abel::flags_internal::tryfromenv_needs_processing) {
        ABEL_INTERNAL_LOG(WARNING,
                          "tryfromenv set twice before it is handled.");
    }

    abel::flags_internal::tryfromenv_needs_processing = true;
});

ABEL_FLAG(std::vector<std::string>, undefok, {},
          "comma-separated list of flag names that it is okay to specify "
          "on the command line even if the program does not define a flag "
          "with that name");

namespace abel {

    namespace flags_internal {

        namespace {

            class ArgsList {
            public:
                ArgsList() : next_arg_(0) {}

                ArgsList(int argc, char *argv[]) : args_(argv, argv + argc), next_arg_(0) {}

                explicit ArgsList(const std::vector<std::string> &args)
                        : args_(args), next_arg_(0) {}

                // Returns success status: true if parsing successful, false otherwise.
                bool ReadFromFlagfile(const std::string &flag_file_name);

                int Size() const { return args_.size() - next_arg_; }

                int FrontIndex() const { return next_arg_; }

                abel::string_view Front() const { return args_[next_arg_]; }

                void PopFront() { next_arg_++; }

            private:
                std::vector<std::string> args_;
                int next_arg_;
            };

            bool ArgsList::ReadFromFlagfile(const std::string &flag_file_name) {
                std::ifstream flag_file(flag_file_name);

                if (!flag_file) {
                    flags_internal::report_usage_error(
                            abel::string_cat("Can't open flagfile ", flag_file_name), true);

                    return false;
                }

                // This argument represents fake argv[0], which should be present in all arg
                // lists.
                args_.push_back("");

                std::string line;
                bool success = true;

                while (std::getline(flag_file, line)) {
                    abel::string_view stripped = abel::trim_left(line);

                    if (stripped.empty() || stripped[0] == '#') {
                        // Comment or empty line; just ignore.
                        continue;
                    }

                    if (stripped[0] == '-') {
                        if (stripped == "--") {
                            flags_internal::report_usage_error(
                                    "Flagfile can't contain position arguments or --", true);

                            success = false;
                            break;
                        }

                        args_.push_back(std::string(stripped));
                        continue;
                    }

                    flags_internal::report_usage_error(
                            abel::string_cat("Unexpected line in the flagfile ", flag_file_name, ": ",
                                             line),
                            true);

                    success = false;
                }

                return success;
            }

// --------------------------------------------------------------------

// Reads the environment variable with name `name` and stores results in
// `value`. If variable is not present in environment returns false, otherwise
// returns true.
            bool GetEnvVar(const char *var_name, std::string *var_value) {
#ifdef _WIN32
                char buf[1024];
                auto get_res = GetEnvironmentVariableA(var_name, buf, sizeof(buf));
                if (get_res >= sizeof(buf)) {
                  return false;
                }

                if (get_res == 0) {
                  return false;
                }

                *var_value = std::string(buf, get_res);
#else
                const char *val = ::getenv(var_name);
                if (val == nullptr) {
                    return false;
                }

                *var_value = val;
#endif

                return true;
            }

// --------------------------------------------------------------------

// Returns:
//  Flag name or empty if arg= --
//  Flag value after = in --flag=value (empty if --foo)
//  "Is empty value" status. True if arg= --foo=, false otherwise. This is
//  required to separate --foo from --foo=.
// For example:
//      arg           return values
//   "--foo=bar" -> {"foo", "bar", false}.
//   "--foo"     -> {"foo", "", false}.
//   "--foo="    -> {"foo", "", true}.
            std::tuple<abel::string_view, abel::string_view, bool> SplitNameAndValue(
                    abel::string_view arg) {
                // Allow -foo and --foo
                abel::consume_prefix(&arg, "-");

                if (arg.empty()) {
                    return std::make_tuple("", "", false);
                }

                auto equal_sign_pos = arg.find("=");

                abel::string_view flag_name = arg.substr(0, equal_sign_pos);

                abel::string_view value;
                bool is_empty_value = false;

                if (equal_sign_pos != abel::string_view::npos) {
                    value = arg.substr(equal_sign_pos + 1);
                    is_empty_value = value.empty();
                }

                return std::make_tuple(flag_name, value, is_empty_value);
            }

// --------------------------------------------------------------------

// Returns:
//  found flag or nullptr
//  is negative in case of --nofoo
            std::tuple<command_line_flag *, bool> LocateFlag(abel::string_view flag_name) {
                command_line_flag *flag = flags_internal::find_command_line_flag(flag_name);
                bool is_negative = false;

                if (!flag && abel::consume_prefix(&flag_name, "no")) {
                    flag = flags_internal::find_command_line_flag(flag_name);
                    is_negative = true;
                }

                return std::make_tuple(flag, is_negative);
            }

// --------------------------------------------------------------------

// Verify that default values of typed flags must be convertible to string and
// back.
            void CheckDefaultValuesParsingRoundtrip() {
#ifndef NDEBUG
                flags_internal::for_each_flag([&](command_line_flag *flag) {
                    if (flag->IsRetired()) return;

#define IGNORE_TYPE(T) \
  if (flag->IsOfType<T>()) return;

                    ABEL_FLAGS_INTERNAL_FOR_EACH_LOCK_FREE(IGNORE_TYPE)
                    IGNORE_TYPE(std::string)
                    IGNORE_TYPE(std::vector<std::string>)
#undef IGNORE_TYPE

                    flag->CheckDefaultValueParsingRoundtrip();
                });
#endif
            }

// --------------------------------------------------------------------

// Returns success status, which is true if we successfully read all flag files,
// in which case new ArgLists are appended to the input_args in a reverse order
// of file names in the input flagfiles list. This order ensures that flags from
// the first flagfile in the input list are processed before the second flagfile
// etc.
            bool ReadFlagfiles(const std::vector<std::string> &flagfiles,
                               std::vector<ArgsList> *input_args) {
                bool success = true;
                for (auto it = flagfiles.rbegin(); it != flagfiles.rend(); ++it) {
                    ArgsList al;

                    if (al.ReadFromFlagfile(*it)) {
                        input_args->push_back(al);
                    } else {
                        success = false;
                    }
                }

                return success;
            }

// Returns success status, which is true if were able to locate all environment
// variables correctly or if fail_on_absent_in_env is false. The environment
// variable names are expected to be of the form `FLAGS_<flag_name>`, where
// `flag_name` is a string from the input flag_names list. If successful we
// append a single ArgList at the end of the input_args.
            bool ReadFlagsFromEnv(const std::vector<std::string> &flag_names,
                                  std::vector<ArgsList> *input_args,
                                  bool fail_on_absent_in_env) {
                bool success = true;
                std::vector<std::string> args;

                // This argument represents fake argv[0], which should be present in all arg
                // lists.
                args.push_back("");

                for (const auto &flag_name : flag_names) {
                    // Avoid infinite recursion.
                    if (flag_name == "fromenv" || flag_name == "tryfromenv") {
                        flags_internal::report_usage_error(
                                abel::string_cat("Infinite recursion on flag ", flag_name), true);

                        success = false;
                        continue;
                    }

                    const std::string envname = abel::string_cat("FLAGS_", flag_name);
                    std::string envval;
                    if (!GetEnvVar(envname.c_str(), &envval)) {
                        if (fail_on_absent_in_env) {
                            flags_internal::report_usage_error(
                                    abel::string_cat(envname, " not found in environment"), true);

                            success = false;
                        }

                        continue;
                    }

                    args.push_back(abel::string_cat("--", flag_name, "=", envval));
                }

                if (success) {
                    input_args->emplace_back(args);
                }

                return success;
            }

// --------------------------------------------------------------------

// Returns success status, which is true if were able to handle all generator
// flags (flagfile, fromenv, tryfromemv) successfully.
            bool HandleGeneratorFlags(std::vector<ArgsList> *input_args,
                                      std::vector<std::string> *flagfile_value) {
                bool success = true;

                abel::mutex_lock l(&flags_internal::processing_checks_guard);

                // flagfile could have been set either on a command line or
                // programmatically before invoking parse_command_line. Note that we do not
                // actually process arguments specified in the flagfile, but instead
                // create a secondary arguments list to be processed along with the rest
                // of the comamnd line arguments. Since we always the process most recently
                // created list of arguments first, this will result in flagfile argument
                // being processed before any other argument in the command line. If
                // FLAGS_flagfile contains more than one file name we create multiple new
                // levels of arguments in a reverse order of file names. Thus we always
                // process arguments from first file before arguments containing in a
                // second file, etc. If flagfile contains another
                // --flagfile inside of it, it will produce new level of arguments and
                // processed before the rest of the flagfile. We are also collecting all
                // flagfiles set on original command line. Unlike the rest of the flags,
                // this flag can be set multiple times and is expected to be handled
                // multiple times. We are collecting them all into a single list and set
                // the value of FLAGS_flagfile to that value at the end of the parsing.
                if (flags_internal::flagfile_needs_processing) {
                    auto flagfiles = abel::get_flag(FLAGS_flagfile);

                    if (input_args->size() == 1) {
                        flagfile_value->insert(flagfile_value->end(), flagfiles.begin(),
                                               flagfiles.end());
                    }

                    success &= ReadFlagfiles(flagfiles, input_args);

                    flags_internal::flagfile_needs_processing = false;
                }

                // Similar to flagfile fromenv/tryfromemv can be set both
                // programmatically and at runtime on a command line. Unlike flagfile these
                // can't be recursive.
                if (flags_internal::fromenv_needs_processing) {
                    auto flags_list = abel::get_flag(FLAGS_fromenv);

                    success &= ReadFlagsFromEnv(flags_list, input_args, true);

                    flags_internal::fromenv_needs_processing = false;
                }

                if (flags_internal::tryfromenv_needs_processing) {
                    auto flags_list = abel::get_flag(FLAGS_tryfromenv);

                    success &= ReadFlagsFromEnv(flags_list, input_args, false);

                    flags_internal::tryfromenv_needs_processing = false;
                }

                return success;
            }

// --------------------------------------------------------------------

            void reset_generator_flags(const std::vector<std::string> &flagfile_value) {
                // Setting flagfile to the value which collates all the values set on a
                // command line and programmatically. So if command line looked like
                // --flagfile=f1 --flagfile=f2 the final value of the FLAGS_flagfile flag is
                // going to be {"f1", "f2"}
                if (!flagfile_value.empty()) {
                    abel::set_flag(&FLAGS_flagfile, flagfile_value);
                    abel::mutex_lock l(&flags_internal::processing_checks_guard);
                    flags_internal::flagfile_needs_processing = false;
                }

                // fromenv/tryfromenv are set to <undefined> value.
                if (!abel::get_flag(FLAGS_fromenv).empty()) {
                    abel::set_flag(&FLAGS_fromenv, {});
                }
                if (!abel::get_flag(FLAGS_tryfromenv).empty()) {
                    abel::set_flag(&FLAGS_tryfromenv, {});
                }

                abel::mutex_lock l(&flags_internal::processing_checks_guard);
                flags_internal::fromenv_needs_processing = false;
                flags_internal::tryfromenv_needs_processing = false;
            }

// --------------------------------------------------------------------

// Returns:
//  success status
//  deduced value
// We are also mutating curr_list in case if we need to get a hold of next
// argument in the input.
            std::tuple<bool, abel::string_view> DeduceFlagValue(const command_line_flag &flag,
                                                                abel::string_view value,
                                                                bool is_negative,
                                                                bool is_empty_value,
                                                                ArgsList *curr_list) {
                // Value is either an argument suffix after `=` in "--foo=<value>"
                // or separate argument in case of "--foo" "<value>".

                // boolean flags have these forms:
                //   --foo
                //   --nofoo
                //   --foo=true
                //   --foo=false
                //   --nofoo=<value> is not supported
                //   --foo <value> is not supported

                // non boolean flags have these forms:
                // --foo=<value>
                // --foo <value>
                // --nofoo is not supported

                if (flag.IsOfType<bool>()) {
                    if (value.empty()) {
                        if (is_empty_value) {
                            // "--bool_flag=" case
                            flags_internal::report_usage_error(
                                    abel::string_cat(
                                            "Missing the value after assignment for the boolean flag '",
                                            flag.Name(), "'"),
                                    true);
                            return std::make_tuple(false, "");
                        }

                        // "--bool_flag" case
                        value = is_negative ? "0" : "1";
                    } else if (is_negative) {
                        // "--nobool_flag=Y" case
                        flags_internal::report_usage_error(
                                abel::string_cat("Negative form with assignment is not valid for the "
                                                 "boolean flag '",
                                                 flag.Name(), "'"),
                                true);
                        return std::make_tuple(false, "");
                    }
                } else if (is_negative) {
                    // "--noint_flag=1" case
                    flags_internal::report_usage_error(
                            abel::string_cat("Negative form is not valid for the flag '", flag.Name(),
                                             "'"),
                            true);
                    return std::make_tuple(false, "");
                } else if (value.empty() && (!is_empty_value)) {
                    if (curr_list->Size() == 1) {
                        // "--int_flag" case
                        flags_internal::report_usage_error(
                                abel::string_cat("Missing the value for the flag '", flag.Name(), "'"),
                                true);
                        return std::make_tuple(false, "");
                    }

                    // "--int_flag" "10" case
                    curr_list->PopFront();
                    value = curr_list->Front();

                    // Heuristic to detect the case where someone treats a std::string arg
                    // like a bool or just forgets to pass a value:
                    // --my_string_var --foo=bar
                    // We look for a flag of std::string type, whose value begins with a
                    // dash and corresponds to known flag or standalone --.
                    if (!value.empty() && value[0] == '-' && flag.IsOfType<std::string>()) {
                        auto maybe_flag_name = std::get<0>(SplitNameAndValue(value.substr(1)));

                        if (maybe_flag_name.empty() ||
                            std::get<0>(LocateFlag(maybe_flag_name)) != nullptr) {
                            // "--string_flag" "--known_flag" case
                            ABEL_INTERNAL_LOG(
                                    WARNING,
                                    abel::string_cat("Did you really mean to set flag '", flag.Name(),
                                                     "' to the value '", value, "'?"));
                        }
                    }
                }

                return std::make_tuple(true, value);
            }

// --------------------------------------------------------------------

            bool CanIgnoreUndefinedFlag(abel::string_view flag_name) {
                auto undefok = abel::get_flag(FLAGS_undefok);
                if (std::find(undefok.begin(), undefok.end(), flag_name) != undefok.end()) {
                    return true;
                }

                if (abel::consume_prefix(&flag_name, "no") &&
                    std::find(undefok.begin(), undefok.end(), flag_name) != undefok.end()) {
                    return true;
                }

                return false;
            }

        }  // namespace

// --------------------------------------------------------------------

        std::vector<char *> ParseCommandLineImpl(int argc, char *argv[],
                                                 ArgvListAction arg_list_act,
                                                 UsageFlagsAction usage_flag_act,
                                                 OnUndefinedFlag on_undef_flag) {
            ABEL_INTERNAL_CHECK(argc > 0, "Missing argv[0]");

            // This routine does not return anything since we abort on failure.
            CheckDefaultValuesParsingRoundtrip();

            std::vector<std::string> flagfile_value;

            std::vector<ArgsList> input_args;
            input_args.push_back(ArgsList(argc, argv));

            std::vector<char *> output_args;
            std::vector<char *> positional_args;
            output_args.reserve(argc);

            // This is the list of undefined flags. The element of the list is the pair
            // consisting of boolean indicating if flag came from command line (vs from
            // some flag file we've read) and flag name.
            // TODO(rogeeff): Eliminate the first element in the pair after cleanup.
            std::vector<std::pair<bool, std::string>> undefined_flag_names;

            // Set program invocation name if it is not set before.
            if (ProgramInvocationName() == "UNKNOWN") {
                flags_internal::SetProgramInvocationName(argv[0]);
            }
            output_args.push_back(argv[0]);

            // Iterate through the list of the input arguments. First level are arguments
            // originated from argc/argv. Following levels are arguments originated from
            // recursive parsing of flagfile(s).
            bool success = true;
            while (!input_args.empty()) {
                // 10. First we process the built-in generator flags.
                success &= HandleGeneratorFlags(&input_args, &flagfile_value);

                // 30. Select top-most (most recent) arguments list. If it is empty drop it
                // and re-try.
                ArgsList &curr_list = input_args.back();

                curr_list.PopFront();

                if (curr_list.Size() == 0) {
                    input_args.pop_back();
                    continue;
                }

                // 40. Pick up the front remaining argument in the current list. If current
                // stack of argument lists contains only one element - we are processing an
                // argument from the original argv.
                abel::string_view arg(curr_list.Front());
                bool arg_from_argv = input_args.size() == 1;

                // 50. If argument does not start with - or is just "-" - this is
                // positional argument.
                if (!abel::consume_prefix(&arg, "-") || arg.empty()) {
                    ABEL_INTERNAL_CHECK(arg_from_argv,
                                        "Flagfile cannot contain positional argument");

                    positional_args.push_back(argv[curr_list.FrontIndex()]);
                    continue;
                }

                if (arg_from_argv && (arg_list_act == ArgvListAction::kKeepParsedArgs)) {
                    output_args.push_back(argv[curr_list.FrontIndex()]);
                }

                // 60. Split the current argument on '=' to figure out the argument
                // name and value. If flag name is empty it means we've got "--". value
                // can be empty either if there were no '=' in argument std::string at all or
                // an argument looked like "--foo=". In a latter case is_empty_value is
                // true.
                abel::string_view flag_name;
                abel::string_view value;
                bool is_empty_value = false;

                std::tie(flag_name, value, is_empty_value) = SplitNameAndValue(arg);

                // 70. "--" alone means what it does for GNU: stop flags parsing. We do
                // not support positional arguments in flagfiles, so we just drop them.
                if (flag_name.empty()) {
                    ABEL_INTERNAL_CHECK(arg_from_argv,
                                        "Flagfile cannot contain positional argument");

                    curr_list.PopFront();
                    break;
                }

                // 80. Locate the flag based on flag name. Handle both --foo and --nofoo
                command_line_flag *flag = nullptr;
                bool is_negative = false;
                std::tie(flag, is_negative) = LocateFlag(flag_name);

                if (flag == nullptr) {
                    if (on_undef_flag != OnUndefinedFlag::kIgnoreUndefined) {
                        undefined_flag_names.emplace_back(arg_from_argv,
                                                          std::string(flag_name));
                    }
                    continue;
                }

                // 90. Deduce flag's value (from this or next argument)
                auto curr_index = curr_list.FrontIndex();
                bool value_success = true;
                std::tie(value_success, value) =
                        DeduceFlagValue(*flag, value, is_negative, is_empty_value, &curr_list);
                success &= value_success;

                // If above call consumed an argument, it was a standalone value
                if (arg_from_argv && (arg_list_act == ArgvListAction::kKeepParsedArgs) &&
                    (curr_index != curr_list.FrontIndex())) {
                    output_args.push_back(argv[curr_list.FrontIndex()]);
                }

                // 100. Set the located flag to a new new value, unless it is retired.
                // Setting retired flag fails, but we ignoring it here.
                if (flag->IsRetired()) continue;

                std::string error;
                if (!flag->SetFromString(value, SET_FLAGS_VALUE, kCommandLine, &error)) {
                    flags_internal::report_usage_error(error, true);
                    success = false;
                }
            }

            for (const auto &flag_name : undefined_flag_names) {
                if (CanIgnoreUndefinedFlag(flag_name.second)) continue;

                flags_internal::report_usage_error(
                        abel::string_cat("Unknown command line flag '", flag_name.second, "'"),
                        true);

                success = false;
            }

#if ABEL_FLAGS_STRIP_NAMES
            if (!success) {
              flags_internal::report_usage_error(
                  "NOTE: command line flags are disabled in this build", true);
            }
#endif

            if (!success) {
                flags_internal::handle_usage_flags(std::cout,
                                                 program_usage_message());
                std::exit(1);
            }

            if (usage_flag_act == UsageFlagsAction::kHandleUsage) {
                int exit_code = flags_internal::handle_usage_flags(
                        std::cout, program_usage_message());

                if (exit_code != -1) {
                    std::exit(exit_code);
                }
            }

            reset_generator_flags(flagfile_value);

            // Reinstate positional args which were intermixed with flags in the arguments
            // list.
            for (auto arg : positional_args) {
                output_args.push_back(arg);
            }

            // All the remaining arguments are positional.
            if (!input_args.empty()) {
                for (int arg_index = input_args.back().FrontIndex(); arg_index < argc;
                     ++arg_index) {
                    output_args.push_back(argv[arg_index]);
                }
            }

            return output_args;
        }

    }  // namespace flags_internal

// --------------------------------------------------------------------

    std::vector<char *> parse_command_line(int argc, char *argv[]) {
        return flags_internal::ParseCommandLineImpl(
                argc, argv, flags_internal::ArgvListAction::kRemoveParsedArgs,
                flags_internal::UsageFlagsAction::kHandleUsage,
                flags_internal::OnUndefinedFlag::kAbortIfUndefined);
    }


}  // namespace abel
