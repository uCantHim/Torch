#include <gtest/gtest.h>
#include <trc_util/data/ObjectPool.h>

struct Dummy : public trc::data::PooledObject<Dummy> {};

class ObjectPoolPlainObjectTest : public testing::Test
{
protected:
    void SetUp() override
    {
        a.i = 3;
        b.i = 5;
        c.i = 12;
    }

    struct Foo : public trc::data::PooledObject<Foo>
    {
        int i;
    };

    trc::data::ObjectPool<Foo> pool{ 100 };
    Foo& a = pool.createObject();
    Foo& b = pool.createObject();
    Foo& c = pool.createObject();
};

TEST(ObjectPoolTest, EmptyPoolThrows)
{
    trc::data::ObjectPool<Dummy> pool{ 1 };

    ASSERT_NO_THROW(pool.createObject());
    ASSERT_THROW(pool.createObject(), trc::data::ObjectPoolOverflowError);
}

TEST(ObjectPoolTest, ObjectsAreReusedInOrder)
{
    struct Foo : public trc::data::PooledObject<Foo> {
        int i{ 0 };
    };

    trc::data::ObjectPool<Foo> pool{ 20 };

    Foo& a = pool.createObject();
    a.i = 22;
    a.release();

    Foo& b = pool.createObject();
    ASSERT_EQ(b.i, 22);
}

TEST_F(ObjectPoolPlainObjectTest, Foreach)
{
    int sum{ 0 };
    pool.foreachActive([&sum](Foo& foo) {
        sum += foo.i;
    });

    ASSERT_EQ(sum, 3 + 5 + 12);
}

TEST_F(ObjectPoolPlainObjectTest, ForeachWithReleasedObject)
{
    int sum{ 0 };

    // Release b
    b.release();
    pool.foreachActive([&sum](Foo& foo) {
        sum += foo.i;
    });
    ASSERT_EQ(sum, 3 + 12);

    // Release a
    sum = 0;
    a.release();
    pool.foreachActive([&sum](Foo& foo) {
        sum += foo.i;
    });
    ASSERT_EQ(sum, 12);

    // Release c (none remain)
    sum = 0;
    c.release();
    pool.foreachActive([&sum](Foo& foo) {
        sum += foo.i;
    });
    ASSERT_EQ(sum, 0);
}
