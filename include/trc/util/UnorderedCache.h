#pragma once

#include <generator>
#include <memory>
#include <vector>

namespace trc
{
    /**
     * @brief An iterable container of weak_ptrs.
     */
    template<typename T>
    class UnorderedCache
    {
    public:
        using value_type = T;
        using reference = value_type&;
        using const_reference = const value_type&;

        using weak_ref = std::weak_ptr<value_type>;
        using shared_ref = std::shared_ptr<value_type>;

        template<typename ...Args>
        auto emplace(Args&&... args) -> shared_ref
        {
            auto elem = std::make_shared<value_type>(std::forward<Args>(args)...);
            elements.emplace_back(elem);
            return elem;
        }

        void emplace(const weak_ref& elem) {
            elements.emplace_back(elem);
        }

        auto iterValidElements() const -> std::generator<const_reference>
        {
            for (const auto& elem : elements)
            {
                if (auto ptr = elem.lock()) {
                    co_yield *ptr;
                }
            }
        }

        auto iterValidElementsWithCleanup() -> std::generator<reference>
        {
            for (auto it = std::begin(elements); it != std::end(elements); /*nothing*/)
            {
                if (shared_ref ptr = it->lock())
                {
                    co_yield *ptr;
                    ++it;
                }
                else {
                    std::swap(*it, elements.back());
                    elements.pop_back();
                }
            }
        }

    private:
        std::vector<weak_ref> elements;
    };
} // namespace trc
