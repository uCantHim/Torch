#include <iostream>
#include <fstream>

#include <trc/shader_edit/Parser.h>
#include <trc/shader_edit/LayoutQualifier.h>
#include <trc/shader_edit/Document.h>

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
        std::cout << "Variable \"" << var.name << "\""
            << " of type " << uint(var.type)
            << " in line " << line
            << "\n";
    }

    shader_edit::Document document(std::move(parseResult));
    document.set("vert_pos_loc", shader_edit::layout::Location{ 3 });
    document.set("camera_buf_binding", 42);

    std::cout << "\n\nFinal document:\n\n" << document.compile()[0];

    return 0;
}
