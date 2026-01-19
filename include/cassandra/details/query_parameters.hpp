#pragma once

#include <cassandra/cassandra_fwd.hpp>

namespace cassandra::details {
class QueryParameters {
 public:
  QueryParameters() = default;

  template <class ParamsHolder>
  explicit QueryParameters(ParamsHolder& ph)
      : size_(ph.Size()),
        values_(ph.ParamBuffers()),
        lengths_(ph.ParamLengthsBuffer()),
        formats_(ph.ParamFormatsBuffer()) {}

  bool Empty() const { return size_ == 0; }
  std::size_t Size() const { return size_; }
  const char* const* ParamBuffers() const { return values_; }
  const int* ParamLengthsBuffer() const { return lengths_; }
  const int* ParamFormatsBuffer() const { return formats_; }

  std::size_t TypeHash() const;

 private:
  std::size_t size_ = 0;
  const char* const* values_ = nullptr;
  const int* lengths_ = nullptr;
  const int* formats_ = nullptr;
};
}  // namespace cassandra::details
