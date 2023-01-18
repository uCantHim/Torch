#include <gtest/gtest.h>

#include <componentlib/ComponentStorage.h>
#include <componentlib/ComponentID.h>
using namespace componentlib;

struct _ObjectTypeTag {};
using Object = ComponentID<_ObjectTypeTag>;

struct TestSceneType : public ComponentStorage<TestSceneType, Object> {};

struct TestCreateInfo
{
    static constexpr int value{ 42 };
};

struct TestComponent
{
    TestComponent() = default;
    explicit TestComponent(int value) : i(value) {}
    explicit TestComponent(TestCreateInfo info) : i(info.value) {}
    TestComponent(int value, std::string) : TestComponent(value) {}

    int i{ 0 };
};

template<>
struct componentlib::ComponentTraits<TestComponent>
{
    static void onCreate(TestSceneType& /*scene*/, Object /*obj*/, TestComponent& /*c*/)
    {
        created = true;
    }

    static void onDelete(TestSceneType& /*scene*/, Object /*obj*/, TestComponent /*c*/)
    {
        destroyed = true;
    }

    static inline bool created{ false };
    static inline bool destroyed{ false };
};

template<>
struct componentlib::ComponentCreateInfoTraits<TestCreateInfo> {
    using ConstructedComponentType = TestComponent;
};



class ComponentStorageTest : public ::testing::Test
{
protected:
    TestSceneType scene;
};

TEST_F(ComponentStorageTest, DefaultConstructor)
{
    Object obj = scene.createObject(TestComponent{});
    ASSERT_TRUE(scene.has<TestComponent>(obj));
    ASSERT_EQ(scene.get<TestComponent>(obj).i, 0);

    scene.deleteObject(obj);
    ASSERT_FALSE(scene.has<TestComponent>(obj));
}

TEST_F(ComponentStorageTest, CreateInfo)
{
    Object obj = scene.createObject(TestCreateInfo{});
    ASSERT_TRUE(scene.has<TestComponent>(obj));
    ASSERT_EQ(scene.get<TestComponent>(obj).i, TestCreateInfo::value);
}

TEST_F(ComponentStorageTest, ComponentTraits)
{
    scene.deleteObject(scene.createObject(TestComponent{}));

    ASSERT_TRUE(ComponentTraits<TestComponent>::created);
    ASSERT_TRUE(ComponentTraits<TestComponent>::destroyed);
}

TEST_F(ComponentStorageTest, AutomaticDestructor)
{
    struct Foo
    {
        Foo(int* ptr) : i(ptr) {}
        ~Foo() { *i = -1337; }
        int* i{ nullptr };
    };

    int value{ 0 };
    scene.deleteObject(scene.createObject(Foo{ &value }));
    ASSERT_EQ(value, -1337);
}

TEST_F(ComponentStorageTest, AddSingleComponents)
{
    struct Foo
    {
        Foo(double, std::string) {}
    };
    struct Bar {};

    auto obj = scene.createObject();
    scene.add<TestComponent>(obj);
    scene.add<Foo>(obj, 42, "hi");

    ASSERT_TRUE(scene.has<TestComponent>(obj));
    ASSERT_TRUE(scene.has<Foo>(obj));
    ASSERT_FALSE(scene.has<Bar>(obj));

    // Query and alter a value
    TestComponent& comp = scene.get<TestComponent>(obj);
    comp.i = 12345;
    ASSERT_EQ(scene.get<TestComponent>(obj).i, 12345);
    ASSERT_EQ(&scene.get<TestComponent>(obj), &comp);

    // Erase one component
    scene.remove<TestComponent>(obj);
    ASSERT_FALSE(scene.has<TestComponent>(obj));
    ASSERT_TRUE(scene.has<Foo>(obj));

    // Recreate component
    scene.add<TestComponent>(obj, 9876);
    ASSERT_TRUE(scene.has<TestComponent>(obj));
    ASSERT_EQ(scene.get<TestComponent>(obj).i, 9876);
    scene.remove<TestComponent>(obj);

    // get throws when component doesn't exist
    ASSERT_THROW(scene.get<TestComponent>(obj), std::out_of_range);

    // Erase the last remaining component
    scene.remove<Foo>(obj);
    ASSERT_FALSE(scene.has<Foo>(obj));

    // Able to delete object that has no components
    scene.deleteObject(obj);

    ASSERT_FALSE(scene.has<TestComponent>(obj));
    ASSERT_FALSE(scene.has<Foo>(obj));
}
