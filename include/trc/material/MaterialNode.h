#pragma once

#include <string>

#include "BasicType.h"
#include "MaterialFunction.h"

namespace trc
{
    class MaterialNode
    {
    public:
        void setInput(ui32 input, MaterialNode* node);

        auto getInputs() const -> const std::vector<MaterialNode*>&;
        auto getOutputChannelCount() const -> ui32;

        auto getFunction() -> MaterialFunction&;

    protected:
        friend class MaterialGraph;
        friend class MaterialCompiler;

        explicit MaterialNode(u_ptr<MaterialFunction> func);

    private:
        std::vector<MaterialNode*> inputs;
        u_ptr<MaterialFunction> function;
    };
} // namespace trc
