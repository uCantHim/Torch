#include <iostream>
#include <fstream>

#include <trc/shader_edit/Parser.h>
#include <trc/shader_edit/LayoutQualifier.h>
#include <trc/shader_edit/Document.h>

namespace layout = shader_edit::layout;

int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        std::cout << "No filename given. Exiting.\n";
        return 0;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open())
    {
        std::cout << "File " << argv[1] << " is not a valid file\n";
        return 1;
    }

    auto parseResult = shader_edit::parse(file);
    for (auto [line, var] : parseResult.variablesByLine)
    {
        std::cout << "Variable \"" << var.name << "\" in line " << line << "\n";
    }

    shader_edit::Document document(std::move(parseResult));
    document.set("vert_pos_loc", layout::Location{ 3 });
    document.set("vert_pos_loc", 42);
    document.permutate("camera_buf_binding",
        {
            layout::Set{ 0, 2, { "std430" } },
            layout::Set{ 1, 2, {} },
            layout::Set{ 2, 4, { "std140" } },
        }
    );
    shader_edit::render("Hi");
    document.permutate("some_cool_name",
        {
            "const bool VAR = true;",
            "const bool VAR = false;",
        }
    );

    auto permutations = document.compile();
    for (const auto& doc : permutations)
    {
        std::cout << "\n--- Permutation\n" << doc << "\n";
    }

    return 0;
}
