#include <fstream>

#include <gtest/gtest.h>

#include <trc/shader_edit/Parser.h>
#include <trc/shader_edit/LayoutQualifier.h>
#include <trc/shader_edit/Document.h>

using namespace shader_edit;

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
    Document document(file);

    document.set("vertex_location", layout::Location{ 0 });

    auto results = document.compile();

    ASSERT_EQ(results.size(), 1);

    auto targetResult = stringFromFile(DATADIR"/test_single_variable_result.vert");
    ASSERT_STREQ(results[0].c_str(), targetResult.c_str());
}

TEST(ShaderEdit, UnsetVariableThrows)
{
    std::ifstream file(DATADIR"/test_single_variable.vert");
    Document document(file);

    ASSERT_THROW(document.compile(), shader_edit::CompileError);
}

TEST(ShaderEdit, NestedPermutation)
{
    std::ifstream file(DATADIR"/test_permutations.vert");
    Document document(file);

    document.set("var", "Hello World!");
    document.permutate("permutation", { "Permutation #1", "Permutation #2" });
    document.permutate("nested", { "Nested #1", "Nested #2", "Nested #3" });
    document.permutate("inner", { "Foo", "Bar" });

    auto results = document.compile();

    ASSERT_EQ(results.size(), 2 * 3 * 2);

    // Merge results
    std::string mergedPermutations;
    for (auto& doc : results) {
        mergedPermutations += doc;
    }

    auto targetResult = stringFromFile(DATADIR"/test_permutations_result.vert");
    ASSERT_STREQ(mergedPermutations.c_str(), targetResult.c_str());
}
