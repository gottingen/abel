//

#include <abel/random/seed_sequences.h>

#include <abel/random/internal/pool_urbg.h>

namespace abel {


    SeedSeq MakeSeedSeq() {
        SeedSeq::result_type seed_material[8];
        random_internal::RandenPool<uint32_t>::Fill(abel::MakeSpan(seed_material));
        return SeedSeq(std::begin(seed_material), std::end(seed_material));
    }


}  // namespace abel
