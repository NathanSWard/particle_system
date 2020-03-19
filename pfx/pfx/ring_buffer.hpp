#ifndef PFX_RING_BUFFER_HPP
#define PFX_RING_BUFFER_HPP
#pragma once

#include <array>
#include <new>
#include <type_traits>

#include "debug.hpp"

namespace pfx {

/*! @brief A queue-like class where insertions only take place once, then when all spots are filled,
 *  you can iterator through the elements from beginning to end.
 *  @tparam T The type of elements.
 *  @Tparam Cap The maximum size of the ring buffer.
 * */
template<class T, std::size_t Cap>
class ring_buffer {
public:
    static_assert(Cap > 0, "ring_buffer capacity must be greater than 0");

    /*! @brief Default constructor.*/
    constexpr ring_buffer() noexcept = default;

    /*! @brief Check if the ring_buffer has hit capacity.
     *  @return A bool denoting if the ring buffer is full.
     * */
    constexpr bool full() const noexcept { return full_; }

    /*! @brief Add an element into the ring_buffer
     *  @param u The element to insert.
     *  @warning A runtime assertion is used to check if the ring_buffer is not full.
     * */
    template<class U>
    void push_back(U&& u) noexcept(std::is_nothrow_assignable_v<T, U>) {
        PFX_ASSERT(!full_);
        data_[curr_] = std::forward<U>(u);
        if (++curr_ == Cap) {
            full_ = true;
            curr_ = 0;
        }
    }

    /*! @brief Get the next element in the queue, in a circular loop fashion.
     *  @return The next element in the queue.
     *  @warning A runtime assertion is used to ensure the ring_buffer is full.
     * */
    T const& next() const noexcept {
        PFX_ASSERT(full_);
        auto const& val = data_[curr_];
        if (++curr_ == Cap)
            curr_ = 0;
        return val;
    }

    /*! @brief Query the number of empty spots in the ring_buffer.
     *  @return The number of emtpy spots.
     */
    constexpr std::size_t remaining() const noexcept {
        return full_ ? 0 : Cap - curr_;
    }

    constexpr auto begin() const noexcept { return data_.begin(); }
    constexpr auto end() const noexcept { return full_ ? data_.end() : data_.begin() + curr_; }

private:
    std::array<T, Cap>  data_{};
    std::size_t mutable curr_ = 0;
    bool                full_ = false;
};

} // namespace pfx

#endif // PFX_RING_BUFFER_HPP
