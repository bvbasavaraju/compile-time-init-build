#pragma once
#include <lookup/detail/select.hpp>
#include <lookup/strategy_failed.hpp>

#include <stdx/compiler.hpp>

#include <algorithm>
#include <array>
#include <bit>

namespace lookup {
template <int MinLoadFactor> struct direct_array_lookup {
  private:
    template <typename InputValues> struct details {
        constexpr static auto key_lt = [](auto lhs, auto rhs) {
            return lhs.key_ < rhs.key_;
        };
        constexpr static auto min_key =
            std::min_element(InputValues::entries.begin(),
                             InputValues::entries.end(), key_lt) -> key_;
        constexpr static auto max_key =
            std::max_element(InputValues::entries.begin(),
                             InputValues::entries.end(), key_lt) -> key_;
        constexpr static auto storage_size = max_key - min_key + 2;
        constexpr static auto num_keys = InputValues::entries.size();
        constexpr static auto load_factor = (num_keys * 100) / storage_size;
    };

    template <typename InputValues> struct impl {
      private:
        using key_type = typename InputValues::key_type;
        using value_type = typename InputValues::value_type;

        constexpr static auto default_value = InputValues::default_value;
        constexpr static auto min_key = details<InputValues>::min_key;

        std::array<value_type, details<InputValues>::storage_size> storage{};

      public:
        constexpr impl() {
            storage.fill(default_value);

            for (auto [k, v] : InputValues::entries) {
                auto const index = k - min_key;
                storage[index] = v;
            }
        }

        [[nodiscard]] constexpr auto operator[](key_type key) const
            -> value_type {
            auto const index = key - min_key;
            auto const idx = detail::select_lt(
                index, static_cast<std::uint32_t>(storage.size()), index,
                static_cast<std::uint32_t>(storage.size()) - 1);
            return storage[idx];
        }
    };

  public:
    template <typename InputValues> [[nodiscard]] CONSTEVAL static auto make() {
        constexpr auto load_factor = details<InputValues>::load_factor;

        if constexpr (load_factor >= MinLoadFactor) {
            return impl<InputValues>();
        } else {
            return strategy_failed_t{};
        }
    }
};
} // namespace lookup
