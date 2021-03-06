//

// HERMETIC NOTE: The randen_hwaes target must not introduce duplicate
// symbols from arbitrary system and other headers, since it may be built
// with different flags from other targets, using different levels of
// optimization, potentially introducing ODR violations.

#include "abel/random/engine/randen_hwaes.h"
#include <cstdint>
#include <cstring>
#include "abel/base/profile.h"

// ABEL_RANDEN_HWAES_IMPL indicates whether this file will contain
// a hardware accelerated implementation of randen, or whether it
// will contain stubs that exit the process.
#if defined(ABEL_ARCH_X86_64) || defined(ABEL_ARCH_X86_32)
// The platform.h directives are sufficient to indicate whether
// we should build accelerated implementations for x86.
#if (ABEL_HAVE_ACCELERATED_AES || ABEL_AES_DISPATCH)
#define ABEL_RANDEN_HWAES_IMPL 1
#endif
#elif defined(ABEL_ARCH_PPC)
// The platform.h directives are sufficient to indicate whether
// we should build accelerated implementations for PPC.
//
// NOTE: This has mostly been tested on 64-bit Power variants,
// and not embedded cpus such as powerpc32-8540
#if ABEL_HAVE_ACCELERATED_AES
#define ABEL_RANDEN_HWAES_IMPL 1
#endif
#elif defined(ABEL_ARCH_ARM) || defined(ABEL_ARCH_AARCH64)
// ARM is somewhat more complicated. We might support crypto natively...
#if ABEL_HAVE_ACCELERATED_AES || \
    (defined(__ARM_NEON) && defined(__ARM_FEATURE_CRYPTO))
#define ABEL_RANDEN_HWAES_IMPL 1

#elif ABEL_AES_DISPATCH && !defined(__APPLE__) && \
    (defined(__GNUC__) && __GNUC__ > 4 || __GNUC__ == 4 && __GNUC_MINOR__ > 9)
// ...or, on GCC, we can use an ASM directive to
// instruct the assember to allow crypto instructions.
#define ABEL_RANDEN_HWAES_IMPL 1
#define ABEL_RANDEN_HWAES_IMPL_CRYPTO_DIRECTIVE 1
#endif
#else
// HWAES is unsupported by these architectures / platforms:
//   __myriad2__
//   __mips__
//
// Other architectures / platforms are unknown.
//
#endif

#if !defined(ABEL_RANDEN_HWAES_IMPL)
// No accelerated implementation is supported.
// The randen_hw_aes functions are stubs that print an error and exit.

#include <cstdio>
#include <cstdlib>

namespace abel {

namespace random_internal {

// No accelerated implementation.
bool has_randen_hw_aes_implementation() { return false; }

// NOLINTNEXTLINE
const void* randen_hw_aes::GetKeys() {
  // Attempted to dispatch to an unsupported dispatch target.
  const int d = ABEL_AES_DISPATCH;
  fprintf(stderr, "AES Hardware detection failed (%d).\n", d);
  exit(1);
  return nullptr;
}

// NOLINTNEXTLINE
void randen_hw_aes::Absorb(const void*, void*) {
  // Attempted to dispatch to an unsupported dispatch target.
  const int d = ABEL_AES_DISPATCH;
  fprintf(stderr, "AES Hardware detection failed (%d).\n", d);
  exit(1);
}

// NOLINTNEXTLINE
void randen_hw_aes::Generate(const void*, void*) {
  // Attempted to dispatch to an unsupported dispatch target.
  const int d = ABEL_AES_DISPATCH;
  fprintf(stderr, "AES Hardware detection failed (%d).\n", d);
  exit(1);
}

}  // namespace random_internal

}  // namespace abel

#else  // defined(ABEL_RANDEN_HWAES_IMPL)
//
// Accelerated implementations are supported.
// We need the per-architecture includes and defines.
//

#include "abel/random/engine/randen_traits.h"

// TARGET_CRYPTO defines a crypto attribute for each architecture.
//
// NOTE: Evaluate whether we should eliminate ABEL_TARGET_CRYPTO.
#if (defined(__clang__) || defined(__GNUC__))
#if defined(ABEL_ARCH_X86_64) || defined(ABEL_ARCH_X86_32)
#define ABEL_TARGET_CRYPTO __attribute__((target("aes")))
#elif defined(ABEL_ARCH_PPC)
#define ABEL_TARGET_CRYPTO __attribute__((target("crypto")))
#else
#define ABEL_TARGET_CRYPTO
#endif
#else
#define ABEL_TARGET_CRYPTO
#endif

#if defined(ABEL_ARCH_PPC)
// NOTE: Keep in mind that PPC can operate in little-endian or big-endian mode,
// however the PPC altivec vector registers (and thus the AES instructions)
// always operate in big-endian mode.

#include <altivec.h>
// <altivec.h> #defines vector __vector; in C++, this is bad form.
#undef vector

// Rely on the PowerPC AltiVec vector operations for accelerated AES
// instructions. GCC support of the PPC vector types is described in:
// https://gcc.gnu.org/onlinedocs/gcc-4.9.0/gcc/PowerPC-AltiVec_002fVSX-Built-in-Functions.html
//
// Already provides operator^=.
using Vector128 = __vector unsigned long long;  // NOLINT(runtime/int)

namespace {

ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO Vector128 ReverseBytes(const Vector128& v) {
  // Reverses the bytes of the vector.
  const __vector unsigned char perm = {15, 14, 13, 12, 11, 10, 9, 8,
                                       7,  6,  5,  4,  3,  2,  1, 0};
  return vec_perm(v, v, perm);
}

// WARNING: these load/store in native byte order. It is OK to load and then
// store an unchanged vector, but interpreting the bits as a number or input
// to AES will have undefined results.
ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO Vector128 Vector128Load(const void* from) {
  return vec_vsx_ld(0, reinterpret_cast<const Vector128*>(from));
}

ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO void Vector128Store(const Vector128& v, void* to) {
  vec_vsx_st(v, 0, reinterpret_cast<Vector128*>(to));
}

// One round of AES. "round_key" is a public constant for breaking the
// symmetry of AES (ensures previously equal columns differ afterwards).
ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO Vector128 AesRound(const Vector128& state,
                                             const Vector128& round_key) {
  return Vector128(__builtin_crypto_vcipher(state, round_key));
}

// Enables native loads in the round loop by pre-swapping.
ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO void SwapEndian(uint64_t* state) {
  using abel::random_internal::randen_traits;
  constexpr size_t kLanes = 2;
  constexpr size_t kFeistelBlocks = randen_traits::kFeistelBlocks;

  for (uint32_t branch = 0; branch < kFeistelBlocks; ++branch) {
    const Vector128 v = ReverseBytes(Vector128Load(state + kLanes * branch));
    Vector128Store(v, state + kLanes * branch);
  }
}

}  // namespace

#elif defined(ABEL_ARCH_ARM) || defined(ABEL_ARCH_AARCH64)

// This asm directive will cause the file to be compiled with crypto extensions
// whether or not the cpu-architecture supports it.
#if ABEL_RANDEN_HWAES_IMPL_CRYPTO_DIRECTIVE
asm(".arch_extension  crypto\n");

// Override missing defines.
#if !defined(__ARM_NEON)
#define __ARM_NEON 1
#endif

#if !defined(__ARM_FEATURE_CRYPTO)
#define __ARM_FEATURE_CRYPTO 1
#endif

#endif

// Rely on the ARM NEON+Crypto advanced simd types, defined in <arm_neon.h>.
// uint8x16_t is the user alias for underlying __simd128_uint8_t type.
// http://infocenter.arm.com/help/topic/com.arm.doc.ihi0073a/IHI0073A_arm_neon_intrinsics_ref.pdf
//
// <arm_neon> defines the following
//
// typedef __attribute__((neon_vector_type(16))) uint8_t uint8x16_t;
// typedef __attribute__((neon_vector_type(16))) int8_t int8x16_t;
// typedef __attribute__((neon_polyvector_type(16))) int8_t poly8x16_t;
//
// vld1q_v
// vst1q_v
// vaeseq_v
// vaesmcq_v
#include <arm_neon.h>

// Already provides operator^=.
using Vector128 = uint8x16_t;

namespace {

ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO Vector128 Vector128Load(const void* from) {
  return vld1q_u8(reinterpret_cast<const uint8_t*>(from));
}

ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO void Vector128Store(const Vector128& v, void* to) {
  vst1q_u8(reinterpret_cast<uint8_t*>(to), v);
}

// One round of AES. "round_key" is a public constant for breaking the
// symmetry of AES (ensures previously equal columns differ afterwards).
ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO Vector128 AesRound(const Vector128& state,
                                             const Vector128& round_key) {
  // It is important to always use the full round function - omitting the
  // final MixColumns reduces security [https://eprint.iacr.org/2010/041.pdf]
  // and does not help because we never decrypt.
  //
  // Note that ARM divides AES instructions differently than x86 / PPC,
  // And we need to skip the first AddRoundKey step and add an extra
  // AddRoundKey step to the end. Lucky for us this is just XOR.
  return vaesmcq_u8(vaeseq_u8(state, uint8x16_t{})) ^ round_key;
}

ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO void SwapEndian(uint64_t*) {}

}  // namespace

#elif defined(ABEL_ARCH_X86_64) || defined(ABEL_ARCH_X86_32)
// On x86 we rely on the aesni instructions
#include <wmmintrin.h>

namespace {

// Vector128 class is only wrapper for __m128i, benchmark indicates that it's
// faster than using __m128i directly.
class Vector128 {
  public:
    // Convert from/to intrinsics.
    ABEL_FORCE_INLINE explicit Vector128(const __m128i &Vector128) : data_(Vector128) {}

    ABEL_FORCE_INLINE __m128i data() const { return data_; }

    ABEL_FORCE_INLINE Vector128 &operator^=(const Vector128 &other) {
        data_ = _mm_xor_si128(data_, other.data());
        return *this;
    }

  private:
    __m128i data_;
};

ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO Vector128 Vector128Load(const void *from) {
    return Vector128(_mm_load_si128(reinterpret_cast<const __m128i *>(from)));
}

ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO void Vector128Store(const Vector128 &v, void *to) {
    _mm_store_si128(reinterpret_cast<__m128i *>(to), v.data());
}

// One round of AES. "round_key" is a public constant for breaking the
// symmetry of AES (ensures previously equal columns differ afterwards).
ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO Vector128 AesRound(const Vector128 &state,
                                                        const Vector128 &round_key) {
    // It is important to always use the full round function - omitting the
    // final MixColumns reduces security [https://eprint.iacr.org/2010/041.pdf]
    // and does not help because we never decrypt.
    return Vector128(_mm_aesenc_si128(state.data(), round_key.data()));
}

ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO void SwapEndian(uint64_t *) {}

}  // namespace

#endif

namespace {

// u64x2 is a 128-bit, (2 x uint64_t lanes) struct used to store
// the randen_keys.
struct alignas(16) u64x2 {
    constexpr u64x2(uint64_t hi, uint64_t lo)
#if defined(ABEL_ARCH_PPC)
    // This has been tested with PPC running in little-endian mode;
    // We byte-swap the u64x2 structure from little-endian to big-endian
    // because altivec always runs in big-endian mode.
    : v{__builtin_bswap64(hi), __builtin_bswap64(lo)} {
#else
            : v{lo, hi} {
#endif
    }

    constexpr bool operator==(const u64x2 &other) const {
        return v[0] == other.v[0] && v[1] == other.v[1];
    }

    constexpr bool operator!=(const u64x2 &other) const {
        return !(*this == other);
    }

    uint64_t v[2];
};  // namespace

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-pragmas"
#endif

// At this point, all of the platform-specific features have been defined /
// implemented.
//
// REQUIRES: using u64x2 = ...
// REQUIRES: using Vector128 = ...
// REQUIRES: Vector128 Vector128Load(void*) {...}
// REQUIRES: void Vector128Store(Vector128, void*) {...}
// REQUIRES: Vector128 AesRound(Vector128, Vector128) {...}
// REQUIRES: void SwapEndian(uint64_t*) {...}
//
// PROVIDES: abel::random_internal::randen_hw_aes::Absorb
// PROVIDES: abel::random_internal::randen_hw_aes::Generate

// RANDen = RANDom generator or beetroots in Swiss German.
// 'Strong' (well-distributed, unpredictable, backtracking-resistant) random
// generator, faster in some benchmarks than std::mt19937_64 and pcg64_c32.
//
// High-level summary:
// 1) Reverie (see "A Robust and Sponge-Like PRNG with Improved Efficiency") is
//    a sponge-like random generator that requires a cryptographic permutation.
//    It improves upon "Provably Robust Sponge-Based PRNGs and KDFs" by
//    achieving backtracking resistance with only one Permute() per buffer.
//
// 2) "Simpira v2: A Family of Efficient Permutations Using the AES Round
//    Function" constructs up to 1024-bit permutations using an improved
//    Generalized Feistel network with 2-round AES-128 functions. This Feistel
//    block shuffle achieves diffusion faster and is less vulnerable to
//    sliced-biclique attacks than the Type-2 cyclic shuffle.
//
// 3) "Improving the Generalized Feistel" and "New criterion for diffusion
//    property" extends the same kind of improved Feistel block shuffle to 16
//    branches, which enables a 2048-bit permutation.
//
// We combine these three ideas and also change Simpira's subround keys from
// structured/low-entropy counters to digits of Pi.

// randen constants.
using abel::random_internal::randen_traits;
constexpr size_t kStateBytes = randen_traits::kStateBytes;
constexpr size_t kCapacityBytes = randen_traits::kCapacityBytes;
constexpr size_t kFeistelBlocks = randen_traits::kFeistelBlocks;
constexpr size_t kFeistelRounds = randen_traits::kFeistelRounds;
constexpr size_t kFeistelFunctions = randen_traits::kFeistelFunctions;

// Independent keys (272 = 2.1 KiB) for the first AES subround of each function.
constexpr size_t kKeys = kFeistelRounds * kFeistelFunctions;

// INCLUDE keys.

    #include "abel/random/engine/randen-keys.inc"

static_assert(kKeys == kRoundKeys, "kKeys and kRoundKeys must be equal");
static_assert(round_keys[kKeys - 1] != u64x2(0, 0),
              "Too few round_keys initializers");

// Number of uint64_t lanes per 128-bit vector;
constexpr size_t kLanes = 2;

// block shuffles applies a shuffle to the entire state between AES rounds.
// Improved odd-even shuffle from "New criterion for diffusion property".
ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO void BlockShuffle(uint64_t *state) {
    static_assert(kFeistelBlocks == 16, "Expecting 16 FeistelBlocks.");

    constexpr size_t shuffle[kFeistelBlocks] = {7, 2, 13, 4, 11, 8, 3, 6,
                                                15, 0, 9, 10, 1, 14, 5, 12};

    // The fully unrolled loop without the memcpy improves the speed by about
    // 30% over the equivalent loop.
    const Vector128 v0 = Vector128Load(state + kLanes * shuffle[0]);
    const Vector128 v1 = Vector128Load(state + kLanes * shuffle[1]);
    const Vector128 v2 = Vector128Load(state + kLanes * shuffle[2]);
    const Vector128 v3 = Vector128Load(state + kLanes * shuffle[3]);
    const Vector128 v4 = Vector128Load(state + kLanes * shuffle[4]);
    const Vector128 v5 = Vector128Load(state + kLanes * shuffle[5]);
    const Vector128 v6 = Vector128Load(state + kLanes * shuffle[6]);
    const Vector128 v7 = Vector128Load(state + kLanes * shuffle[7]);
    const Vector128 w0 = Vector128Load(state + kLanes * shuffle[8]);
    const Vector128 w1 = Vector128Load(state + kLanes * shuffle[9]);
    const Vector128 w2 = Vector128Load(state + kLanes * shuffle[10]);
    const Vector128 w3 = Vector128Load(state + kLanes * shuffle[11]);
    const Vector128 w4 = Vector128Load(state + kLanes * shuffle[12]);
    const Vector128 w5 = Vector128Load(state + kLanes * shuffle[13]);
    const Vector128 w6 = Vector128Load(state + kLanes * shuffle[14]);
    const Vector128 w7 = Vector128Load(state + kLanes * shuffle[15]);

    Vector128Store(v0, state + kLanes * 0);
    Vector128Store(v1, state + kLanes * 1);
    Vector128Store(v2, state + kLanes * 2);
    Vector128Store(v3, state + kLanes * 3);
    Vector128Store(v4, state + kLanes * 4);
    Vector128Store(v5, state + kLanes * 5);
    Vector128Store(v6, state + kLanes * 6);
    Vector128Store(v7, state + kLanes * 7);
    Vector128Store(w0, state + kLanes * 8);
    Vector128Store(w1, state + kLanes * 9);
    Vector128Store(w2, state + kLanes * 10);
    Vector128Store(w3, state + kLanes * 11);
    Vector128Store(w4, state + kLanes * 12);
    Vector128Store(w5, state + kLanes * 13);
    Vector128Store(w6, state + kLanes * 14);
    Vector128Store(w7, state + kLanes * 15);
}

// Feistel round function using two AES subrounds. Very similar to F()
// from Simpira v2, but with independent subround keys. Uses 17 AES rounds
// per 16 bytes (vs. 10 for AES-CTR). Computing eight round functions in
// parallel hides the 7-cycle AESNI latency on HSW. Note that the Feistel
// XORs are 'free' (included in the second AES instruction).
ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO const u64x2 *FeistelRound(
        uint64_t *state, const u64x2 *ABEL_RESTRICT keys) {
    static_assert(kFeistelBlocks == 16, "Expecting 16 FeistelBlocks.");

    // MSVC does a horrible job at unrolling loops.
    // So we unroll the loop by hand to improve the performance.
    const Vector128 s0 = Vector128Load(state + kLanes * 0);
    const Vector128 s1 = Vector128Load(state + kLanes * 1);
    const Vector128 s2 = Vector128Load(state + kLanes * 2);
    const Vector128 s3 = Vector128Load(state + kLanes * 3);
    const Vector128 s4 = Vector128Load(state + kLanes * 4);
    const Vector128 s5 = Vector128Load(state + kLanes * 5);
    const Vector128 s6 = Vector128Load(state + kLanes * 6);
    const Vector128 s7 = Vector128Load(state + kLanes * 7);
    const Vector128 s8 = Vector128Load(state + kLanes * 8);
    const Vector128 s9 = Vector128Load(state + kLanes * 9);
    const Vector128 s10 = Vector128Load(state + kLanes * 10);
    const Vector128 s11 = Vector128Load(state + kLanes * 11);
    const Vector128 s12 = Vector128Load(state + kLanes * 12);
    const Vector128 s13 = Vector128Load(state + kLanes * 13);
    const Vector128 s14 = Vector128Load(state + kLanes * 14);
    const Vector128 s15 = Vector128Load(state + kLanes * 15);

    // Encode even blocks with keys.
    const Vector128 e0 = AesRound(s0, Vector128Load(keys + 0));
    const Vector128 e2 = AesRound(s2, Vector128Load(keys + 1));
    const Vector128 e4 = AesRound(s4, Vector128Load(keys + 2));
    const Vector128 e6 = AesRound(s6, Vector128Load(keys + 3));
    const Vector128 e8 = AesRound(s8, Vector128Load(keys + 4));
    const Vector128 e10 = AesRound(s10, Vector128Load(keys + 5));
    const Vector128 e12 = AesRound(s12, Vector128Load(keys + 6));
    const Vector128 e14 = AesRound(s14, Vector128Load(keys + 7));

    // Encode odd blocks with even output from above.
    const Vector128 o1 = AesRound(e0, s1);
    const Vector128 o3 = AesRound(e2, s3);
    const Vector128 o5 = AesRound(e4, s5);
    const Vector128 o7 = AesRound(e6, s7);
    const Vector128 o9 = AesRound(e8, s9);
    const Vector128 o11 = AesRound(e10, s11);
    const Vector128 o13 = AesRound(e12, s13);
    const Vector128 o15 = AesRound(e14, s15);

    // Store odd blocks. (These will be shuffled later).
    Vector128Store(o1, state + kLanes * 1);
    Vector128Store(o3, state + kLanes * 3);
    Vector128Store(o5, state + kLanes * 5);
    Vector128Store(o7, state + kLanes * 7);
    Vector128Store(o9, state + kLanes * 9);
    Vector128Store(o11, state + kLanes * 11);
    Vector128Store(o13, state + kLanes * 13);
    Vector128Store(o15, state + kLanes * 15);

    return keys + 8;
}

// Cryptographic permutation based via type-2 Generalized Feistel Network.
// Indistinguishable from ideal by chosen-ciphertext adversaries using less than
// 2^64 queries if the round function is a PRF. This is similar to the b=8 case
// of Simpira v2, but more efficient than its generic construction for b=16.
ABEL_FORCE_INLINE ABEL_TARGET_CRYPTO void Permute(
        const void *ABEL_RESTRICT keys, uint64_t *state) {
    const u64x2 *ABEL_RESTRICT keys128 =
            static_cast<const u64x2 *>(keys);

    // (Successfully unrolled; the first iteration jumps into the second half)
#ifdef __clang__
#pragma clang loop unroll_count(2)
#endif
    for (size_t round = 0; round < kFeistelRounds; ++round) {
        keys128 = FeistelRound(state, keys128);
        BlockShuffle(state);
    }
}

}  // namespace

namespace abel {

namespace random_internal {

bool has_randen_hw_aes_implementation() { return true; }

const void *ABEL_TARGET_CRYPTO randen_hw_aes::get_keys() {
    // Round keys for one AES per Feistel round and branch.
    // The canonical implementation uses first digits of Pi.
    return round_keys;
}

// NOLINTNEXTLINE
void ABEL_TARGET_CRYPTO randen_hw_aes::absorb(const void *seed_void,
                                              void *state_void) {
    auto *state = static_cast<uint64_t *>(state_void);
    const auto *seed = static_cast<const uint64_t *>(seed_void);

    constexpr size_t kCapacityBlocks = kCapacityBytes / sizeof(Vector128);
    constexpr size_t kStateBlocks = kStateBytes / sizeof(Vector128);

    static_assert(kCapacityBlocks * sizeof(Vector128) == kCapacityBytes,
                  "Not i*V");
    static_assert(kCapacityBlocks == 1, "Unexpected randen kCapacityBlocks");
    static_assert(kStateBlocks == 16, "Unexpected randen kStateBlocks");

    Vector128 b1 = Vector128Load(state + kLanes * 1);
    b1 ^= Vector128Load(seed + kLanes * 0);
    Vector128Store(b1, state + kLanes * 1);

    Vector128 b2 = Vector128Load(state + kLanes * 2);
    b2 ^= Vector128Load(seed + kLanes * 1);
    Vector128Store(b2, state + kLanes * 2);

    Vector128 b3 = Vector128Load(state + kLanes * 3);
    b3 ^= Vector128Load(seed + kLanes * 2);
    Vector128Store(b3, state + kLanes * 3);

    Vector128 b4 = Vector128Load(state + kLanes * 4);
    b4 ^= Vector128Load(seed + kLanes * 3);
    Vector128Store(b4, state + kLanes * 4);

    Vector128 b5 = Vector128Load(state + kLanes * 5);
    b5 ^= Vector128Load(seed + kLanes * 4);
    Vector128Store(b5, state + kLanes * 5);

    Vector128 b6 = Vector128Load(state + kLanes * 6);
    b6 ^= Vector128Load(seed + kLanes * 5);
    Vector128Store(b6, state + kLanes * 6);

    Vector128 b7 = Vector128Load(state + kLanes * 7);
    b7 ^= Vector128Load(seed + kLanes * 6);
    Vector128Store(b7, state + kLanes * 7);

    Vector128 b8 = Vector128Load(state + kLanes * 8);
    b8 ^= Vector128Load(seed + kLanes * 7);
    Vector128Store(b8, state + kLanes * 8);

    Vector128 b9 = Vector128Load(state + kLanes * 9);
    b9 ^= Vector128Load(seed + kLanes * 8);
    Vector128Store(b9, state + kLanes * 9);

    Vector128 b10 = Vector128Load(state + kLanes * 10);
    b10 ^= Vector128Load(seed + kLanes * 9);
    Vector128Store(b10, state + kLanes * 10);

    Vector128 b11 = Vector128Load(state + kLanes * 11);
    b11 ^= Vector128Load(seed + kLanes * 10);
    Vector128Store(b11, state + kLanes * 11);

    Vector128 b12 = Vector128Load(state + kLanes * 12);
    b12 ^= Vector128Load(seed + kLanes * 11);
    Vector128Store(b12, state + kLanes * 12);

    Vector128 b13 = Vector128Load(state + kLanes * 13);
    b13 ^= Vector128Load(seed + kLanes * 12);
    Vector128Store(b13, state + kLanes * 13);

    Vector128 b14 = Vector128Load(state + kLanes * 14);
    b14 ^= Vector128Load(seed + kLanes * 13);
    Vector128Store(b14, state + kLanes * 14);

    Vector128 b15 = Vector128Load(state + kLanes * 15);
    b15 ^= Vector128Load(seed + kLanes * 14);
    Vector128Store(b15, state + kLanes * 15);
}

// NOLINTNEXTLINE
void ABEL_TARGET_CRYPTO randen_hw_aes::generate(const void *keys,
                                                void *state_void) {
    static_assert(kCapacityBytes == sizeof(Vector128), "Capacity mismatch");

    auto *state = static_cast<uint64_t *>(state_void);

    const Vector128 prev_inner = Vector128Load(state);

    SwapEndian(state);

    Permute(keys, state);

    SwapEndian(state);

    // Ensure backtracking resistance.
    Vector128 inner = Vector128Load(state);
    inner ^= prev_inner;
    Vector128Store(inner, state);
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif

}  // namespace random_internal

}  // namespace abel

#endif  // (ABEL_RANDEN_HWAES_IMPL)
