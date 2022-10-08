#pragma once

#include <componentlib/Table.h>

#include "trc/Node.h"

namespace trc::drawcomp
{
    struct NodeComponent
    {
        Node node;
    };
} // namespace trc::drawcomp



template<>
struct componentlib::TableTraits<trc::drawcomp::NodeComponent>
{
    using UniqueStorage = std::true_type;
};
