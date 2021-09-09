#include <gtest/gtest.h>

#include <trc/util/data/SelfManagedObject.h>
#include <trc/util/data/IndexMap.h>
#include <trc/util/data/TypesafeId.h>
using namespace trc::data;

struct IdType {};

TEST(TypesafeIdTest, CorrectCasting)
{
    using ID = TypesafeID<IdType, uint32_t>;

    ID a{ 0 };
    ID b{ a + 1 };

    IndexMap<int, uint32_t> map;

    SelfManagedObject<IdType> obj;
}
