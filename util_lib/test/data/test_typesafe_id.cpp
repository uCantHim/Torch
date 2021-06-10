#include <gtest/gtest.h>

#include <nc/data/SelfManagedObject.h>
#include <nc/data/IndexMap.h>
#include <nc/data/TypesafeId.h>
using namespace nc::data;

struct IdType {};

TEST(TypesafeIdTest, CorrectCasting)
{
    using ID = TypesafeID<IdType, uint32_t>;

    ID a{ 0 };
    ID b{ a + 1 };

    IndexMap<int, uint32_t> map;

    SelfManagedObject<IdType> obj;
}
