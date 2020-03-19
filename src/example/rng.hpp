#ifndef RNG_HPP
#define RNG_HPP
#pragma once

#include <random>

namespace {
inline std::random_device rd_;
inline std::mt19937_64 gen_(rd_());
} // namespace

/*! @brief float random number generator.
 *  @tparam Min minimum random value.
 *  @tparam Max maximum random value.
 * */
template<auto Min, auto Max>
struct frng;

/*! @brief frng specialization using function pointers.
 *  @tparam Min function pointer returning the minimum float value.
 *  @tparma Max function pointer returning the maximum float value.
 * */
template<float(*Min)(), float(*Max)()>
struct frng<Min, Max> {
    /*! @brief Returns a random number between Max and Min
     *  @return The random number.
     * */
    inline static float rand() {
        return dist_(gen_);
    }
private:
    inline static std::uniform_real_distribution<float> dist_{Min(), Max()};
};

/*! @brief frng specialization using ints.
 *  @tparam Min minimum random value.
 *  @tparma Max maximum random value.
 *  @note Min/Max value are explicitly cast to floats.
 * */
template<int Min, int Max>
struct frng<Min, Max> {
    /*! @brief Returns a random number between Max and Min
     *  @return The random number.
     * */
    inline static float rand() {
        return dist_(gen_);
    }
private:
    inline static std::uniform_real_distribution<float> dist_{float(Min), float(Max)};
};

#endif //RNG_HPP
