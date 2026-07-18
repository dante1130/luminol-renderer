#pragma once

#include <cstdint>
#include <optional>
#include <set>

namespace Luminol::Graphics {

template <typename Id = uint32_t>
class IdPool {
public:
    explicit IdPool(std::optional<Id> capacity = std::nullopt)
        : capacity{capacity} {}

    [[nodiscard]] auto allocate() -> std::optional<Id> {
        if (!this->free_ids.empty()) {
            const auto free_id = *this->free_ids.begin();
            this->free_ids.erase(this->free_ids.begin());
            return free_id;
        }

        if (this->capacity.has_value() && this->next_id >= *this->capacity) {
            return std::nullopt;
        }

        return this->next_id++;
    }

    auto free(Id freed_id) -> void {
        if (freed_id < this->next_id) {
            this->free_ids.insert(freed_id);
        }
    }

private:
    std::optional<Id> capacity;
    std::set<Id> free_ids;
    Id next_id = 0;
};

}  // namespace Luminol::Graphics
