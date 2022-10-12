#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "shader_tools/ShaderDocument.h"

using namespace shader_edit;

struct CustomLocation
{
    uint32_t location;

    explicit operator std::string() const {
        return "layout (location = " + std::to_string(location) + ")";
    }
};

auto stringFromFile(const std::string& path) -> std::string
{
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();

    return ss.str();
}

TEST(ShaderEdit, ReplaceVariable)
{
    std::ifstream file(DATADIR"/test_single_variable.vert");
    ShaderDocument document(file);

    document.set("vertex_location", "layout (location = 0)");

    std::string result = document.compile();
    std::string targetResult = stringFromFile(DATADIR"/test_single_variable_result.vert");
    ASSERT_STREQ(result.c_str(), targetResult.c_str());
}

TEST(ShaderEdit, CustomRenderable)
{
    std::ifstream file(DATADIR"/test_single_variable.vert");
    ShaderDocument document(file);

    document.set("vertex_location", CustomLocation{ 0 });

    std::string result = document.compile();
    std::string targetResult = stringFromFile(DATADIR"/test_single_variable_result.vert");
    ASSERT_STREQ(result.c_str(), targetResult.c_str());
}

TEST(ShaderEdit, UnsetVariableThrows)
{
    std::ifstream file(DATADIR"/test_single_variable.vert");
    ShaderDocument document(file);

    ASSERT_THROW(document.compile(), shader_edit::CompileError);
}

TEST(ShaderEdit, NestedPermutation)
{
    std::ifstream file(DATADIR"/test_permutations.vert");

    ShaderDocument document(file);
    document.set("var", "Hello World!");

    std::vector<ShaderDocument> docs;
    docs = permutate(document, "permutation", { "Permutation #1", "Permutation #2" });
    docs = permutate(docs, "nested", { "Nested #1", "Nested #2", "Nested #3" });
    docs = permutate(docs, "inner", { "Foo", "Bar" });

    auto results = compile(docs);

    ASSERT_EQ(results.size(), 2 * 3 * 2);

    // Merge results
    std::string mergedPermutations;
    for (auto& doc : results) {
        mergedPermutations += doc;
    }

    auto targetResult = stringFromFile(DATADIR"/test_permutations_result.vert");
    ASSERT_STREQ(mergedPermutations.c_str(), targetResult.c_str());
}
