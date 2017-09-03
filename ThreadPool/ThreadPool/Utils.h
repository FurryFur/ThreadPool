#pragma once

#include <array>

template <typename T, size_t DimFirst, size_t... Dims>
class Tensor : public std::array<Tensor<T, Dims...>, DimFirst> {};

template <typename T, size_t DimLast>
class Tensor<T, DimLast> : public std::array<T, DimLast> {};