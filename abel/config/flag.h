//
//
// -----------------------------------------------------------------------------
// File: flag.h
// -----------------------------------------------------------------------------
//
// This header file defines the `abel::Flag<T>` type for holding command-line
// flag data, and abstractions to create, get and set such flag data.
//
// It is important to note that this type is **unspecified** (an implementation
// detail) and you do not construct or manipulate actual `abel::Flag<T>`
// instances. Instead, you define and declare flags using the
// `ABEL_FLAG()` and `ABEL_DECLARE_FLAG()` macros, and get and set flag values
// using the `abel::get_flag()` and `abel::set_flag()` functions.

#ifndef ABEL_FLAGS_FLAG_H_
#define ABEL_FLAGS_FLAG_H_

#include <abel/base/profile.h>
#include <abel/math/bit_cast.h>
#include <abel/config/config.h>
#include <abel/config/declare.h>
#include <abel/config/internal/command_line_flag.h>
#include <abel/config/internal/flag.h>
#include <abel/config/marshalling.h>

namespace abel {


// Flag
//
// An `abel::Flag` holds a command-line flag value, providing a runtime
// parameter to a binary. Such flags should be defined in the global namespace
// and (preferably) in the module containing the binary's `main()` function.
//
// You should not construct and cannot use the `abel::Flag` type directly;
// instead, you should declare flags using the `ABEL_DECLARE_FLAG()` macro
// within a header file, and define your flag using `ABEL_FLAG()` within your
// header's associated `.cc` file. Such flags will be named `FLAGS_name`.
//
// Example:
//
//    .h file
//
//      // Declares usage of a flag named "FLAGS_count"
//      ABEL_DECLARE_FLAG(int, count);
//
//    .cc file
//
//      // Defines a flag named "FLAGS_count" with a default `int` value of 0.
//      ABEL_FLAG(int, count, 0, "Count of items to process");
//
// No public methods of `abel::Flag<T>` are part of the abel Flags API.
#if !defined(_MSC_VER) || defined(__clang__)
    template<typename T>
    using Flag = flags_internal::Flag<T>;
#else
    // MSVC debug builds do not implement initialization with constexpr constructors
    // correctly. To work around this we add a level of indirection, so that the
    // class `abel::Flag` contains an `internal::Flag*` (instead of being an alias
    // to that class) and dynamically allocates an instance when necessary. We also
    // forward all calls to internal::Flag methods via trampoline methods. In this
    // setup the `abel::Flag` class does not have constructor and virtual methods,
    // all the data members are public and thus MSVC is able to initialize it at
    // link time. To deal with multiple threads accessing the flag for the first
    // time concurrently we use an atomic boolean indicating if flag object is
    // initialized. We also employ the double-checked locking pattern where the
    // second level of protection is a global mutex, so if two threads attempt to
    // construct the flag concurrently only one wins.
    // This solution is based on a recomendation here:
    // https://developercommunity.visualstudio.com/content/problem/336946/class-with-constexpr-constructor-not-using-static.html?childToView=648454#comment-648454

    namespace flags_internal {
    abel::mutex* GetGlobalConstructionGuard();
    }  // namespace flags_internal

    template <typename T>
    class Flag {
     public:
      // No constructor and destructor to ensure this is an aggregate type.
      // Visual Studio 2015 still requires the constructor for class to be
      // constexpr initializable.
#if _MSC_VER <= 1900
      constexpr Flag(const char* name, const char* filename,
                     const flags_internal::flag_marshalling_op_fn marshalling_op,
                     const flags_internal::HelpGenFunc help_gen,
                     const flags_internal::FlagDfltGenFunc default_value_gen)
          : name_(name),
            filename_(filename),
            marshalling_op_(marshalling_op),
            help_gen_(help_gen),
            default_value_gen_(default_value_gen),
            inited_(false),
            impl_(nullptr) {}
#endif

      flags_internal::Flag<T>* GetImpl() const {
        if (!inited_.load(std::memory_order_acquire)) {
          abel::mutex_lock l(flags_internal::GetGlobalConstructionGuard());

          if (inited_.load(std::memory_order_acquire)) {
            return impl_;
          }

          impl_ = new flags_internal::Flag<T>(
              name_, filename_, marshalling_op_,
              {flags_internal::FlagHelpSrc(help_gen_),
               flags_internal::FlagHelpSrcKind::kGenFunc},
              default_value_gen_);
          inited_.store(true, std::memory_order_release);
        }

        return impl_;
      }

      // Public methods of `abel::Flag<T>` are NOT part of the abel Flags API.
      bool is_retired() const { return GetImpl()->is_retired(); }
      bool IsAbelFlag() const { return GetImpl()->IsAbelFlag(); }
      abel::string_view Name() const { return GetImpl()->Name(); }
      std::string Help() const { return GetImpl()->Help(); }
      bool is_modified() const { return GetImpl()->is_modified(); }
      bool is_specified_on_command_line() const {
        return GetImpl()->is_specified_on_command_line();
      }
      abel::string_view Typename() const { return GetImpl()->Typename(); }
      std::string Filename() const { return GetImpl()->Filename(); }
      std::string DefaultValue() const { return GetImpl()->DefaultValue(); }
      std::string CurrentValue() const { return GetImpl()->CurrentValue(); }
      template <typename U>
      ABEL_FORCE_INLINE bool is_of_type() const {
        return GetImpl()->template is_of_type<U>();
      }
      T Get() const { return GetImpl()->Get(); }
      bool AtomicGet(T* v) const { return GetImpl()->AtomicGet(v); }
      void Set(const T& v) { GetImpl()->Set(v); }
      void SetCallback(const flags_internal::FlagCallback mutation_callback) {
        GetImpl()->SetCallback(mutation_callback);
      }
      void InvokeCallback() { GetImpl()->InvokeCallback(); }

      // The data members are logically private, but they need to be public for
      // this to be an aggregate type.
      const char* name_;
      const char* filename_;
      const flags_internal::flag_marshalling_op_fn marshalling_op_;
      const flags_internal::HelpGenFunc help_gen_;
      const flags_internal::FlagDfltGenFunc default_value_gen_;

      mutable std::atomic<bool> inited_;
      mutable flags_internal::Flag<T>* impl_;
    };
#endif

// get_flag()
//
// Returns the value (of type `T`) of an `abel::Flag<T>` instance, by value. Do
// not construct an `abel::Flag<T>` directly and call `abel::get_flag()`;
// instead, refer to flag's constructed variable name (e.g. `FLAGS_name`).
// Because this function returns by value and not by reference, it is
// thread-safe, but note that the operation may be expensive; as a result, avoid
// `abel::get_flag()` within any tight loops.
//
// Example:
//
//   // FLAGS_count is a Flag of type `int`
//   int my_count = abel::get_flag(FLAGS_count);
//
//   // FLAGS_firstname is a Flag of type `std::string`
//   std::string first_name = abel::get_flag(FLAGS_firstname);
    template<typename T>
    ABEL_MUST_USE_RESULT T get_flag(const abel::Flag<T> &flag) {
#define ABEL_FLAGS_INTERNAL_LOCK_FREE_VALIDATE(BIT) \
  static_assert(                                    \
      !std::is_same<T, BIT>::value,                 \
      "Do not specify explicit template parameters to abel::get_flag");
        ABEL_FLAGS_INTERNAL_FOR_EACH_LOCK_FREE(ABEL_FLAGS_INTERNAL_LOCK_FREE_VALIDATE)
#undef ABEL_FLAGS_INTERNAL_LOCK_FREE_VALIDATE

        return flag.Get();
    }

// Overload for `get_flag()` for types that support lock-free reads.
#define ABEL_FLAGS_INTERNAL_LOCK_FREE_EXPORT(T) \
  ABEL_MUST_USE_RESULT T get_flag(const abel::Flag<T>& flag);

    ABEL_FLAGS_INTERNAL_FOR_EACH_LOCK_FREE(ABEL_FLAGS_INTERNAL_LOCK_FREE_EXPORT)

#undef ABEL_FLAGS_INTERNAL_LOCK_FREE_EXPORT

// set_flag()
//
// Sets the value of an `abel::Flag` to the value `v`. Do not construct an
// `abel::Flag<T>` directly and call `abel::set_flag()`; instead, use the
// flag's variable name (e.g. `FLAGS_name`). This function is
// thread-safe, but is potentially expensive. Avoid setting flags in general,
// but especially within performance-critical code.
    template<typename T>
    void set_flag(abel::Flag<T> *flag, const T &v) {
        flag->Set(v);
    }

// Overload of `set_flag()` to allow callers to pass in a value that is
// convertible to `T`. E.g., use this overload to pass a "const char*" when `T`
// is `std::string`.
    template<typename T, typename V>
    void set_flag(abel::Flag<T> *flag, const V &v) {
        T value(v);
        flag->Set(value);
    }


}  // namespace abel


// ABEL_FLAG()
//
// This macro defines an `abel::Flag<T>` instance of a specified type `T`:
//
//   ABEL_FLAG(T, name, default_value, help);
//
// where:
//
//   * `T` is a supported flag type (see the list of types in `marshalling.h`),
//   * `name` designates the name of the flag (as a global variable
//     `FLAGS_name`),
//   * `default_value` is an expression holding the default value for this flag
//     (which must be implicitly convertible to `T`),
//   * `help` is the help text, which can also be an expression.
//
// This macro expands to a flag named 'FLAGS_name' of type 'T':
//
//   abel::Flag<T> FLAGS_name = ...;
//
// Note that all such instances are created as global variables.
//
// For `ABEL_FLAG()` values that you wish to expose to other translation units,
// it is recommended to define those flags within the `.cc` file associated with
// the header where the flag is declared.
//
// Note: do not construct objects of type `abel::Flag<T>` directly. Only use the
// `ABEL_FLAG()` macro for such construction.
#define ABEL_FLAG(Type, name, default_value, help) \
  ABEL_FLAG_IMPL(Type, name, default_value, help)

// ABEL_FLAG().OnUpdate()
//
// Defines a flag of type `T` with a callback attached:
//
//   ABEL_FLAG(T, name, default_value, help).OnUpdate(callback);
//
// After any setting of the flag value, the callback will be called at least
// once. A rapid sequence of changes may be merged together into the same
// callback. No concurrent calls to the callback will be made for the same
// flag. Callbacks are allowed to read the current value of the flag but must
// not mutate that flag.
//
// The update mechanism guarantees "eventual consistency"; if the callback
// derives an auxiliary data structure from the flag value, it is guaranteed
// that eventually the flag value and the derived data structure will be
// consistent.
//
// Note: ABEL_FLAG.OnUpdate() does not have a public definition. Hence, this
// comment serves as its API documentation.


// -----------------------------------------------------------------------------
// Implementation details below this section
// -----------------------------------------------------------------------------

// ABEL_FLAG_IMPL macro definition conditional on ABEL_FLAGS_STRIP_NAMES

#if ABEL_FLAGS_STRIP_NAMES
#define ABEL_FLAG_IMPL_FLAGNAME(txt) ""
#define ABEL_FLAG_IMPL_FILENAME() ""
#if !defined(_MSC_VER) || defined(__clang__)
#define ABEL_FLAG_IMPL_REGISTRAR(T, flag) \
  abel::flags_internal::FlagRegistrar<T, false>(&flag)
#else
#define ABEL_FLAG_IMPL_REGISTRAR(T, flag) \
  abel::flags_internal::FlagRegistrar<T, false>(flag.GetImpl())
#endif
#else
#define ABEL_FLAG_IMPL_FLAGNAME(txt) txt
#define ABEL_FLAG_IMPL_FILENAME() __FILE__
#if !defined(_MSC_VER) || defined(__clang__)
#define ABEL_FLAG_IMPL_REGISTRAR(T, flag) \
  abel::flags_internal::FlagRegistrar<T, true>(&flag)
#else
#define ABEL_FLAG_IMPL_REGISTRAR(T, flag) \
  abel::flags_internal::FlagRegistrar<T, true>(flag.GetImpl())
#endif
#endif

// ABEL_FLAG_IMPL macro definition conditional on ABEL_FLAGS_STRIP_HELP

#if ABEL_FLAGS_STRIP_HELP
#define ABEL_FLAG_IMPL_FLAGHELP(txt) abel::flags_internal::kStrippedFlagHelp
#else
#define ABEL_FLAG_IMPL_FLAGHELP(txt) txt
#endif

// AbelFlagHelpGenFor##name is used to encapsulate both immediate (method Const)
// and lazy (method NonConst) evaluation of help message expression. We choose
// between the two via the call to HelpArg in abel::Flag instantiation below.
// If help message expression is constexpr evaluable compiler will optimize
// away this whole struct.
#define ABEL_FLAG_IMPL_DECLARE_HELP_WRAPPER(name, txt)                     \
  struct AbelFlagHelpGenFor##name {                                        \
    template <typename T = void>                                           \
    static constexpr const char* Const() {                                 \
      return abel::flags_internal::HelpConstexprWrap(                      \
          ABEL_FLAG_IMPL_FLAGHELP(txt));                                   \
    }                                                                      \
    static std::string NonConst() { return ABEL_FLAG_IMPL_FLAGHELP(txt); } \
  }

#define ABEL_FLAG_IMPL_DECLARE_DEF_VAL_WRAPPER(name, Type, default_value)   \
  static void* AbelFlagsInitFlag##name() {                                  \
    return abel::flags_internal::MakeFromDefaultValue<Type>(default_value); \
  }

// ABEL_FLAG_IMPL
//
// Note: Name of registrar object is not arbitrary. It is used to "grab"
// global name for FLAGS_no<flag_name> symbol, thus preventing the possibility
// of defining two flags with names foo and nofoo.
#if !defined(_MSC_VER) || defined(__clang__)
#define ABEL_FLAG_IMPL(Type, name, default_value, help)             \
  namespace abel /* block flags in namespaces */ {}                 \
  ABEL_FLAG_IMPL_DECLARE_DEF_VAL_WRAPPER(name, Type, default_value) \
  ABEL_FLAG_IMPL_DECLARE_HELP_WRAPPER(name, help);                  \
  ABEL_CONST_INIT abel::Flag<Type> FLAGS_##name{                    \
      ABEL_FLAG_IMPL_FLAGNAME(#name), ABEL_FLAG_IMPL_FILENAME(),    \
      &abel::flags_internal::flag_marshalling_ops<Type>,              \
      abel::flags_internal::HelpArg<AbelFlagHelpGenFor##name>(0),   \
      &AbelFlagsInitFlag##name};                                    \
  extern bool FLAGS_no##name;                                       \
  bool FLAGS_no##name = ABEL_FLAG_IMPL_REGISTRAR(Type, FLAGS_##name)
#else
// MSVC version uses aggregate initialization. We also do not try to
// optimize away help wrapper.
#define ABEL_FLAG_IMPL(Type, name, default_value, help)               \
  namespace abel /* block flags in namespaces */ {}                   \
  ABEL_FLAG_IMPL_DECLARE_DEF_VAL_WRAPPER(name, Type, default_value)   \
  ABEL_FLAG_IMPL_DECLARE_HELP_WRAPPER(name, help);                    \
  ABEL_CONST_INIT abel::Flag<Type> FLAGS_##name{                      \
      ABEL_FLAG_IMPL_FLAGNAME(#name), ABEL_FLAG_IMPL_FILENAME(),      \
      &abel::flags_internal::flag_marshalling_ops<Type>,                \
      &AbelFlagHelpGenFor##name::NonConst, &AbelFlagsInitFlag##name}; \
  extern bool FLAGS_no##name;                                         \
  bool FLAGS_no##name = ABEL_FLAG_IMPL_REGISTRAR(Type, FLAGS_##name)
#endif

// ABEL_RETIRED_FLAG
//
// Designates the flag (which is usually pre-existing) as "retired." A retired
// flag is a flag that is now unused by the program, but may still be passed on
// the command line, usually by production scripts. A retired flag is ignored
// and code can't access it at runtime.
//
// This macro registers a retired flag with given name and type, with a name
// identical to the name of the original flag you are retiring. The retired
// flag's type can change over time, so that you can retire code to support a
// custom flag type.
//
// This macro has the same signature as `ABEL_FLAG`. To retire a flag, simply
// replace an `ABEL_FLAG` definition with `ABEL_RETIRED_FLAG`, leaving the
// arguments unchanged (unless of course you actually want to retire the flag
// type at this time as well).
//
// `default_value` is only used as a double check on the type. `explanation` is
// unused.
// TODO(rogeeff): Return an anonymous struct instead of bool, and place it into
// the unnamed namespace.
#define ABEL_RETIRED_FLAG(type, flagname, default_value, explanation) \
  ABEL_ATTRIBUTE_UNUSED static const bool ignored_##flagname =        \
      ([] { return type(default_value); },                            \
       abel::flags_internal::retired_flag<type>(#flagname))

#endif  // ABEL_FLAGS_FLAG_H_