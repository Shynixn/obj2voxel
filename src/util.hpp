#ifndef OBJ2VOXEL_UTIL_HPP
#define OBJ2VOXEL_UTIL_HPP

#include "3rd_party/tinyobj.hpp"

#include "voxelio/color.hpp"
#include "voxelio/ileave.hpp"
#include "voxelio/types.hpp"
#include "voxelio/vec.hpp"

#define OBJ2VOXEL_USE_UNORDERED_MAP

#ifdef OBJ2VOXEL_USE_UNORDERED_MAP
#include <unordered_map>
#else
#include <map>
#endif

namespace obj2voxel {

using namespace voxelio;
using real_type = tinyobj::real_t;

/// Returns true if two floating point numbers are exactly equal without warnings (-Wfloat-equal).
template <typename Float>
constexpr bool eqExactly(Float a, Float b)
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
    return a == b;
#pragma clang diagnostic pop
}

// VEC UTILITY FUNCTIONS ===============================================================================================

namespace detail {

/**
 * @brief Applies a binary function to each component of two vectors and returns the result.
 */
template <typename T, size_t N, typename BinaryFunction>
constexpr Vec<T, N> applyBinary(Vec<T, N> a, Vec<T, N> b, BinaryFunction function)
{
    Vec<T, N> result{};
    for (usize i = 0; i < N; ++i) {
        result[i] = function(a[i], b[i]);
    }
    return result;
}

/**
 * @brief Applies a unary function to each component of two vectors and returns the result.
 */
template <typename T, size_t N, typename UnaryFunction>
constexpr Vec<T, N> applyUnary(Vec<T, N> a, UnaryFunction function)
{
    Vec<T, N> result{};
    for (usize i = 0; i < N; ++i) {
        result[i] = function(a[i]);
    }
    return result;
}

}  // namespace detail

/// Returns the component-wise min.
template <typename T, size_t N>
constexpr Vec<T, N> min(Vec<T, N> a, Vec<T, N> b)
{
    using function_type = const T &(*) (const T &, const T &);
    return detail::applyBinary<T, N, function_type>(a, b, std::min<T>);
}

/// Returns the component-wise min.
template <typename T, size_t N>
constexpr Vec<T, N> max(Vec<T, N> a, Vec<T, N> b)
{
    using function_type = const T &(*) (const T &, const T &);
    return detail::applyBinary<T, N, function_type>(a, b, std::max<T>);
}

/// Three-parameter min. For Vec types, returns the component-wise minimum.
template <typename T>
constexpr T min(const T &a, const T &b, const T &c)
{
    if constexpr (voxelio::isVec<T>) {
        return obj2voxel::min(a, obj2voxel::min(b, c));
    }
    else {
        return std::min(a, std::min(b, c));
    }
}

/// Three-parameter min. For Vec types, returns the component-wise maximum.
template <typename T>
constexpr T max(const T &a, const T &b, const T &c)
{
    if constexpr (voxelio::isVec<T>) {
        return obj2voxel::max(a, obj2voxel::max(b, c));
    }
    else {
        return std::max(a, std::max(b, c));
    }
}

/// Computes the component-wise floor of the vector.
template <typename T, size_t N>
constexpr Vec<T, N> floor(Vec<T, N> v)
{
    T (*function)(T) = std::floor;
    return detail::applyUnary<T, N>(v, function);
}

/// Computes the component-wise ceil of the vector.
template <typename T, size_t N>
constexpr Vec<T, N> ceil(Vec<T, N> v)
{
    T (*function)(T) = std::ceil;
    return detail::applyUnary<T, N>(v, function);
}

/// Computes the component-wise abs of the vector.
template <typename T, size_t N>
constexpr Vec<T, N> abs(Vec<T, N> v)
{
    T (*function)(T) = std::abs;
    return detail::applyUnary<T, N>(v, function);
}

/// Computes the length or magnitude of the vector.
template <typename T, size_t N>
T length(Vec<T, N> v)
{
    T result = std::sqrt(dot<T, T, N>(v, v));
    return result;
}

/// Divides a vector by its length.
template <typename T, size_t N>
constexpr Vec<T, N> normalize(Vec<T, N> v)
{
    return v / length(v);
}

/// Mixes or linearly interpolates two vectors.
template <typename T, size_t N>
constexpr Vec<T, N> mix(Vec<T, N> a, Vec<T, N> b, T t)
{
    return (1 - t) * a + t * b;
}

// WEIGHTED ============================================================================================================

template <typename T>
struct Weighted {
    float weight;
    T value;
};

using WeightedColor = Weighted<Vec<real_type, 3>>;
using WeightedUv = Weighted<Vec<real_type, 2>>;

/// Mixes two colors based on their weights.
template <typename T>
constexpr Weighted<T> mix(const Weighted<T> &lhs, const Weighted<T> &rhs)
{
    float weightSum = lhs.weight + rhs.weight;
    return {weightSum, (lhs.weight * lhs.value + rhs.weight * rhs.value) / weightSum};
}

/// Chooses the color with the greater weight.
template <typename T>
constexpr Weighted<T> max(const Weighted<T> &lhs, const Weighted<T> &rhs)
{
    return lhs.weight > rhs.weight ? lhs : rhs;
}

template <typename T>
using WeightedCombineFunction = Weighted<T> (*)(const Weighted<T> &, const Weighted<T> &);

// VOXEL MAP ===========================================================================================================

template <typename K, typename V>
#ifdef OBJ2VOXEL_USE_UNORDERED_MAP
using voxel_map_base_type = std::unordered_map<K, V>;
#else
using voxel_map_base_type = std::map<K, V>;
#endif

template <typename T>
struct VoxelMap : public voxel_map_base_type<u64, T> {
    static u64 indexOf(Vec3u32 pos)
    {
        return voxelio::ileave3(pos.x(), pos.y(), pos.z());
    }

    static Vec3u32 posOf(u64 index)
    {
        Vec3u32 result;
        voxelio::dileave3(index, result.data());
        return result;
    }

    using base_type = voxel_map_base_type<u64, T>;
    using iterator = typename base_type::iterator;

    template <typename V>
    std::pair<iterator, bool> emplace(Vec3u pos, V &&value)
    {
        return static_cast<base_type *>(this)->emplace(indexOf(pos), std::forward<V>(value));
    }

    template <typename V>
    std::pair<iterator, bool> emplace(u64 index, V &&value)
    {
        return static_cast<base_type *>(this)->emplace(index, std::forward<V>(value));
    }
};

// AFFINE TRANSFORM ====================================================================================================

struct AffineTransform {
    using vec_type = Vec<real_type, 3>;

    static constexpr AffineTransform fromUnitTransform(int matrix[9], vec_type translation = vec_type::zero())
    {
        VXIO_DEBUG_ASSERT_NOTNULL(matrix);
        AffineTransform result{};
        for (usize i = 0; i < 9; ++i) {
            result.matrix[i / 3][i % 3] = static_cast<real_type>(matrix[i]);
        }
        result.translation = translation;
        return result;
    }

    vec_type matrix[3];
    vec_type translation;

    constexpr AffineTransform(float scale = 1, vec_type translation = vec_type::zero())
        : matrix{{scale, 0, 0}, {0, scale, 0}, {0, 0, scale}}, translation{translation}
    {
    }

    constexpr bool isScale() const
    {
        for (usize i = 0; i < 3; ++i) {
            for (usize j = 0; j < 3; ++j) {
                if (not eqExactly(matrix[i][j], 0.f) != eqExactly(i, j)) {
                    return false;
                }
            }
        }
        return true;
    }

    constexpr bool isUniformScale() const
    {
        return isScale() && eqExactly(matrix[0][0], matrix[1][1]) && eqExactly(matrix[0][0], matrix[2][2]);
    }

    constexpr vec_type row(usize i) const
    {
        return matrix[i];
    }

    constexpr vec_type col(usize j) const
    {
        return {matrix[0][j], matrix[1][j], matrix[2][j]};
    }
};

constexpr AffineTransform::vec_type operator*(const AffineTransform &a, const AffineTransform::vec_type &v)
{
    real_type x = dot(a.row(0), v);
    real_type y = dot(a.row(1), v);
    real_type z = dot(a.row(2), v);
    return AffineTransform::vec_type{x, y, z} + a.translation;
}

constexpr AffineTransform operator*(const AffineTransform &lhs, const AffineTransform &rhs)
{
    AffineTransform result{};
    for (usize i = 0; i < 3; ++i) {
        for (usize j = 0; j < 3; ++j) {
            result.matrix[i][j] = dot(lhs.row(i), rhs.col(j));
        }
        result.translation[i] = dot(lhs.row(i), rhs.translation);
    }
    result.translation += lhs.translation;
    return result;
}

}  // namespace obj2voxel

#endif  // OBJ2VOXEL_UTIL_HPP
