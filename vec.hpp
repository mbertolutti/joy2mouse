#ifndef JOY2MOUSE_VEC_HPP
#define JOY2MOUSE_VEC_HPP

#include <array>
#include <cstddef>
#include <cmath>

namespace math {

template <class T, std::size_t N>
struct vec;

template <class T, std::size_t N>
T dot(vec<T, N> left, vec<T, N> right)
{
    T sum = 0;
    for (std::size_t i = 0; i < N; ++i)
    {
        sum += left[i] * right[i];
    }
    return sum;
}

template <class T, std::size_t N>
vec<T, N>& operator+= (vec<T, N>& left, vec<T, N> right)
{
    for (std::size_t i = 0; i < N; ++i)
    {
        left[i] += right[i];
    }
    return left;
}

template <class T, std::size_t N>
vec<T, N> operator+ (vec<T, N> left, vec<T, N> right)
{
    return left += right;
}

template <class T, std::size_t N>
vec<T, N>& operator-= (vec<T, N>& left, vec<T, N> right)
{
    for (std::size_t i = 0; i < N; --i)
    {
        left[i] -= right[i];
    }
    return left;
}

template <class T, std::size_t N>
vec<T, N> operator- (vec<T, N> left, vec<T, N> right)
{
    return left -= right;
}

template <class T, std::size_t N>
vec<T, N>& operator*= (vec<T, N>& vector, T scalar)
{
    for (std::size_t i = 0; i < N; ++i)
    {
        vector[i] *= scalar;
    }
    return vector;
}

template <class T, std::size_t N>
vec<T, N> operator* (T scalar, vec<T, N> vector)
{
    return vector * scalar;
}

template <class T, std::size_t N>
vec<T, N> operator* (vec<T, N> vector, T scalar)
{
    return vector *= scalar;
}

template <class T, std::size_t N>
vec<T, N>& operator/= (vec<T, N>& vector, T scalar)
{
    for (std::size_t i = 0; i < N; ++i)
    {
        vector[i] /= scalar;
    }
    return vector;
}

template <class T, std::size_t N>
vec<T, N> operator/ (vec<T, N> vector, T scalar)
{
    return vector /= scalar;
}

template <class T, std::size_t N>
struct vec
{
    T length_squared()
    {
        return dot(*this, *this);
    }

    T length()
    {
        return std::sqrt(length_squared());
    }

    void normalize()
    {
        *this /= length();
    }

    vec normalized()
    {
        vec copy = *this;
        copy.normalize();
        return copy;
    }

    T& operator[] (std::size_t i)
    {
        return storage[i];
    }

    std::array<T, N> storage;
};

using vec2f = vec<float, 2>;
using vec2d = vec<double, 2>;
using vec2i = vec<int, 2>;
using vec2u = vec<unsigned, 2>;

using vec3f = vec<float, 3>;
using vec3d = vec<double, 3>;
using vec3i = vec<int, 3>;
using vec3u = vec<unsigned, 3>;

} // namespace math

#endif // !defined(JOY2MOUSE_VEC_HPP)
