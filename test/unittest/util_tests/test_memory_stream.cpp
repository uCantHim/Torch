#include <cstring>
#include <sstream>
#include <string>

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
using namespace nlohmann;

#include <trc_util/MemoryStream.h>

auto parse(std::istream& is) -> json
{
    return json::parse(is);
}

TEST(MemoryStreamTest, BasicIstream)
{
    char str[] = R"({"hello":"world"})";
    trc::util::MemoryStream istream(str, sizeof(str));

    json j1;
    ASSERT_NO_THROW(j1 = parse(istream));

    json truth{ { "hello", "world" } };
    ASSERT_EQ(j1, truth);
}

TEST(MemoryStreamTest, BasicOstream)
{
    std::array<char, 19> data;
    trc::util::MemoryStream ostream(data.data(), data.size());

    ostream << json{ "foo", "bar", "baz" };

    json j1;
    ASSERT_NO_THROW(j1 = parse(ostream));
    json truth{ "foo", "bar", "baz" };
    ASSERT_EQ(j1, truth);
}

TEST(MemoryStreamTest, SeekPosition)
{
    std::array<char, 1000> data{};
    trc::util::MemoryStream stream(data.data(), data.size());

    stream << "Hello World!\n"
           << "This is a new line.";

    std::string line;

    // Read a line from the stream
    std::getline(stream, line);
    ASSERT_EQ(line, "Hello World!");
    ASSERT_TRUE(stream.good());

    // Reset read pointer and read the line again
    stream.seekg(0);
    std::getline(stream, line);
    ASSERT_EQ(line, "Hello World!");
    ASSERT_TRUE(stream.good());

    // Read the second line
    std::getline(stream, line, char{0});
    ASSERT_EQ(line, "This is a new line.");
    ASSERT_TRUE(stream.good());

    // Reset write pointer and write some new data
    stream.seekp(0);
    stream << "New data";

    // Reset read pointer and read newly written data
    stream.seekg(0);
    line.resize(8);
    stream.read(line.data(), 8);
    ASSERT_EQ(line, "New data");

    // Read the remainder of the partially overwritten first line
    std::getline(stream, line);
    ASSERT_EQ(line, "rld!");

    // Read the entire stream (until EOF)
    line.clear();
    std::getline(stream, line);
    ASSERT_EQ(line.size(), data.size() - strlen("Hello World!\n"));
    line.clear();
    std::getline(stream, line);
    ASSERT_TRUE(line.empty());
}
