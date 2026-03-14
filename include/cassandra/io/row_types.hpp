#pragma once

namespace cassandra::io {

struct RowTag {};
struct FieldTag {};

constexpr RowTag kRowTag{};
constexpr FieldTag kFieldTag{};
}  // namespace cassandra::io
