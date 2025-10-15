#pragma once

/// @file utils/generate_salt.hpp
/// @brief @copybrief utils

#include <string>

namespace utils {
/// @brief  Генерирует соль для хеширования паролей
/// @return Рандомная соль в виде строки
std::string GenerateSalt();
}  // namespace utils
