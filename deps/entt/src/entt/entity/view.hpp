#ifndef ENTT_ENTITY_VIEW_HPP
#define ENTT_ENTITY_VIEW_HPP


#include <iterator>
#include <array>
#include <tuple>
#include <utility>
#include <algorithm>
#include <type_traits>
#include "../config/config.h"
#include "../core/type_traits.hpp"
#include "sparse_set.hpp"
#include "storage.hpp"
#include "entity.hpp"
#include "fwd.hpp"


namespace entt {


/**
 * @brief View.
 *
 * Primary template isn't defined on purpose. All the specializations give a
 * compile-time error, but for a few reasonable cases.
 */
template<typename...>
class basic_view;


/**
 * @brief Multi component view.
 *
 * Multi component views iterate over those entities that have at least all the
 * given components in their bags. During initialization, a multi component view
 * looks at the number of entities available for each component and uses the
 * smallest set in order to get a performance boost when iterate.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given components are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, if one of the
 *   given components is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pools iterated by the view in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share references to the underlying data structures of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Exclude Types of components used to filter the view.
 * @tparam Component Types of components iterated by the view.
 */
template<typename Entity, typename... Exclude, typename... Component>
class basic_view<Entity, exclude_t<Exclude...>, Component...> {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    template<typename Comp>
    using pool_type = std::conditional_t<std::is_const_v<Comp>, const storage<Entity, std::remove_const_t<Comp>>, storage<Entity, Comp>>;

    template<typename Comp>
    using component_iterator_type = decltype(std::declval<pool_type<Comp>>().begin());

    using underlying_iterator_type = typename sparse_set<Entity>::iterator_type;
    using unchecked_type = std::array<const sparse_set<Entity> *, (sizeof...(Component) - 1)>;
    using filter_type = std::array<const sparse_set<Entity> *, sizeof...(Exclude)>;
    using traits_type = entt_traits<std::underlying_type_t<Entity>>;

    class iterator {
        friend class basic_view<Entity, exclude_t<Exclude...>, Component...>;

        iterator(const sparse_set<Entity> *candidate, unchecked_type other, filter_type ignore, underlying_iterator_type curr) ENTT_NOEXCEPT
            : view{candidate},
              unchecked{other},
              filter{ignore},
              it{curr}
        {
            if(it != view->end() && !valid()) {
                ++(*this);
            }
        }

        bool valid() const {
            return std::all_of(unchecked.cbegin(), unchecked.cend(), [entt = *it](const sparse_set<Entity> *curr) { return curr->has(entt); })
                    && std::none_of(filter.cbegin(), filter.cend(), [entt = *it](const sparse_set<Entity> *curr) { return curr->has(entt); });
        }

    public:
        using difference_type = typename underlying_iterator_type::difference_type;
        using value_type = typename underlying_iterator_type::value_type;
        using pointer = typename underlying_iterator_type::pointer;
        using reference = typename underlying_iterator_type::reference;
        using iterator_category = std::bidirectional_iterator_tag;

        iterator() ENTT_NOEXCEPT = default;

        iterator & operator++() {
            while(++it != view->end() && !valid());
            return *this;
        }

        iterator operator++(int) {
            iterator orig = *this;
            return operator++(), orig;
        }

        iterator & operator--() ENTT_NOEXCEPT {
            while(--it != view->begin() && !valid());
            return *this;
        }

        iterator operator--(int) ENTT_NOEXCEPT {
            iterator orig = *this;
            return operator--(), orig;
        }

        bool operator==(const iterator &other) const ENTT_NOEXCEPT {
            return other.it == it;
        }

        bool operator!=(const iterator &other) const ENTT_NOEXCEPT {
            return !(*this == other);
        }

        pointer operator->() const {
            return it.operator->();
        }

        reference operator*() const {
            return *operator->();
        }

    private:
        const sparse_set<Entity> *view;
        unchecked_type unchecked;
        filter_type filter;
        underlying_iterator_type it;
    };

    // we could use pool_type<Component> &..., but vs complains about it and refuses to compile for unknown reasons (likely a bug)
    basic_view(storage<Entity, std::remove_const_t<Component>> &... component, storage<Entity, std::remove_const_t<Exclude>> &... epool) ENTT_NOEXCEPT
        : pools{&component..., &epool...}
    {}

    const sparse_set<Entity> * candidate() const ENTT_NOEXCEPT {
        return std::min({ static_cast<const sparse_set<Entity> *>(std::get<pool_type<Component> *>(pools))... }, [](const auto *lhs, const auto *rhs) {
            return lhs->size() < rhs->size();
        });
    }

    unchecked_type unchecked(const sparse_set<Entity> *view) const {
        std::size_t pos{};
        unchecked_type other{};
        ((std::get<pool_type<Component> *>(pools) == view ? nullptr : (other[pos++] = std::get<pool_type<Component> *>(pools))), ...);
        return other;
    }

    template<typename Comp, typename Other>
    decltype(auto) get([[maybe_unused]] component_iterator_type<Comp> it, [[maybe_unused]] pool_type<Other> *cpool, [[maybe_unused]] const Entity entt) const {
        if constexpr(std::is_same_v<Comp, Other>) {
            return *it;
        } else {
            return cpool->get(entt);
        }
    }

    template<typename Comp, typename Func, typename... Type>
    void traverse(Func func, type_list<Type...>) const {
        if constexpr(std::disjunction_v<std::is_same<Comp, Type>...>) {
            auto it = std::get<pool_type<Comp> *>(pools)->begin();

            for(const auto entt: static_cast<const sparse_set<entity_type> &>(*std::get<pool_type<Comp> *>(pools))) {
                auto curr = it++;

                if(((std::is_same_v<Comp, Component> || std::get<pool_type<Component> *>(pools)->has(entt)) && ...) && (!std::get<pool_type<Exclude> *>(pools)->has(entt) && ...)) {
                    if constexpr(std::is_invocable_v<Func, decltype(get<Type>({}))...>) {
                        func(get<Comp, Type>(curr, std::get<pool_type<Type> *>(pools), entt)...);
                    } else {
                        func(entt, get<Comp, Type>(curr, std::get<pool_type<Type> *>(pools), entt)...);
                    }
                }
            }
        } else {
            for(const auto entt: static_cast<const sparse_set<entity_type> &>(*std::get<pool_type<Comp> *>(pools))) {
                if(((std::is_same_v<Comp, Component> || std::get<pool_type<Component> *>(pools)->has(entt)) && ...) && (!std::get<pool_type<Exclude> *>(pools)->has(entt) && ...)) {
                    if constexpr(std::is_invocable_v<Func, decltype(get<Type>({}))...>) {
                        func(std::get<pool_type<Type> *>(pools)->get(entt)...);
                    } else {
                        func(entt, std::get<pool_type<Type> *>(pools)->get(entt)...);
                    }
                }
            }
        }
    }

public:
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator_type = iterator;

    /**
     * @brief Returns the number of existing components of the given type.
     *
     * This isn't the number of entities iterated by the view.
     *
     * @tparam Comp Type of component of which to return the size.
     * @return Number of existing components of the given type.
     */
    template<typename Comp>
    size_type size() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->size();
    }

    /**
     * @brief Estimates the number of entities iterated by the view.
     * @return Estimated number of entities iterated by the view.
     */
    size_type size() const ENTT_NOEXCEPT {
        return std::min({ std::get<pool_type<Component> *>(pools)->size()... });
    }

    /**
     * @brief Checks whether a view or some pools are empty.
     *
     * The view is definitely empty if one of the pools it uses is empty. In all
     * other cases, the view may be empty and not return entities even if this
     * function returns false.
     *
     * @tparam Comp Types of components in which one is interested.
     * @return True if the view or the pools are empty, false otherwise.
     */
    template<typename... Comp>
    bool empty() const ENTT_NOEXCEPT {
        if constexpr(sizeof...(Comp) == 0) {
            return (std::get<pool_type<Component> *>(pools)->empty() || ...);
        } else {
            return (std::get<pool_type<Comp> *>(pools)->empty() && ...);
        }
    }

    /**
     * @brief Direct access to the list of components of a given pool.
     *
     * The returned pointer is such that range
     * `[raw<Comp>(), raw<Comp>() + size<Comp>()]` is always a valid range, even
     * if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
     *
     * @tparam Comp Type of component in which one is interested.
     * @return A pointer to the array of components.
     */
    template<typename Comp>
    Comp * raw() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->raw();
    }

    /**
     * @brief Direct access to the list of entities of a given pool.
     *
     * The returned pointer is such that range
     * `[data<Comp>(), data<Comp>() + size<Comp>()]` is always a valid range,
     * even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
     *
     * @tparam Comp Type of component in which one is interested.
     * @return A pointer to the array of entities.
     */
    template<typename Comp>
    const entity_type * data() const ENTT_NOEXCEPT {
        return std::get<pool_type<Comp> *>(pools)->data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * components.
     *
     * The returned iterator points to the first entity that has the given
     * components. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given components.
     */
    iterator_type begin() const {
        const auto *view = candidate();
        const filter_type ignore{std::get<pool_type<Exclude> *>(pools)...};
        return iterator_type{view, unchecked(view), ignore, view->begin()};
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given components.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given components. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given components.
     */
    iterator_type end() const {
        const auto *view = candidate();
        const filter_type ignore{std::get<pool_type<Exclude> *>(pools)...};
        return iterator_type{view, unchecked(view), ignore, view->end()};
    }

    /**
     * @brief Returns the first entity that has the given components, if any.
     * @return The first entity that has the given components if one exists, the
     * null entity otherwise.
     */
    entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity that has the given components, if any.
     * @return The last entity that has the given components if one exists, the
     * null entity otherwise.
     */
    entity_type back() const {
        const auto it = std::make_reverse_iterator(end());
        return it != std::make_reverse_iterator(begin()) ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const {
        const auto *view = candidate();
        const filter_type ignore{std::get<pool_type<Exclude> *>(pools)...};
        iterator_type it{view, unchecked(view), ignore, view->find(entt)};
        return (it != end() && *it == entt) ? it : end();
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const {
        return (std::get<pool_type<Component> *>(pools)->has(entt) && ...)
                && (!std::get<pool_type<Exclude> *>(pools)->has(entt) && ...);
    }

    /**
     * @brief Returns the components assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an invalid component type results in a compilation
     * error. Attempting to use an entity that doesn't belong to the view
     * results in undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @tparam Comp Types of components to get.
     * @param entt A valid entity identifier.
     * @return The components assigned to the entity.
     */
    template<typename... Comp>
    decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        ENTT_ASSERT(contains(entt));

        if constexpr(sizeof...(Comp) == 0) {
            static_assert(sizeof...(Component) == 1);
            return (std::get<pool_type<Component> *>(pools)->get(entt), ...);
        } else if constexpr(sizeof...(Comp) == 1) {
            return (std::get<pool_type<Comp> *>(pools)->get(entt), ...);
        } else {
            return std::tuple<decltype(get<Comp>({}))...>{get<Comp>(entt)...};
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all its components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Component &...);
     * void(Component &...);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        const auto *view = candidate();
        ((std::get<pool_type<Component> *>(pools) == view ? each<Component>(std::move(func)) : void()), ...);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to all its components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Component &...);
     * void(Component &...);
     * @endcode
     *
     * The pool of the suggested component is used to lead the iterations. The
     * returned entities will therefore respect the order of the pool associated
     * with that type.<br/>
     * It is no longer guaranteed that the performance is the best possible, but
     * there will be greater control over the order of iteration.
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Comp Type of component to use to enforce the iteration order.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Comp, typename Func>
    void each(Func func) const {
        traverse<Comp>(std::move(func), type_list<Component...>{});
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * @sa each
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void less(Func func) const {
        const auto *view = candidate();
        ((std::get<pool_type<Component> *>(pools) == view ? less<Component>(std::move(func)) : void()), ...);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a set of references to non-empty components. The
     * _constness_ of the components is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Type &...);
     * void(Type &...);
     * @endcode
     *
     * The pool of the suggested component is used to lead the iterations. The
     * returned entities will therefore respect the order of the pool associated
     * with that type.<br/>
     * It is no longer guaranteed that the performance is the best possible, but
     * there will be greater control over the order of iteration.
     *
     * @sa each
     *
     * @tparam Comp Type of component to use to enforce the iteration order.
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Comp, typename Func>
    void less(Func func) const {
        using non_empty_type = type_list_cat_t<std::conditional_t<ENTT_ENABLE_ETO(Component), type_list<>, type_list<Component>>...>;
        traverse<Comp>(std::move(func), non_empty_type{});
    }

private:
    const std::tuple<pool_type<Component> *..., pool_type<Exclude> *...> pools;
};


/**
 * @brief Single component view specialization.
 *
 * Single component views are specialized in order to get a boost in terms of
 * performance. This kind of views can access the underlying data structure
 * directly and avoid superfluous checks.
 *
 * @b Important
 *
 * Iterators aren't invalidated if:
 *
 * * New instances of the given component are created and assigned to entities.
 * * The entity currently pointed is modified (as an example, the given
 *   component is removed from the entity to which the iterator points).
 * * The entity currently pointed is destroyed.
 *
 * In all other cases, modifying the pool iterated by the view in any way
 * invalidates all the iterators and using them results in undefined behavior.
 *
 * @note
 * Views share a reference to the underlying data structure of the registry that
 * generated them. Therefore any change to the entities and to the components
 * made by means of the registry are immediately reflected by views.
 *
 * @warning
 * Lifetime of a view must overcome the one of the registry that generated it.
 * In any other case, attempting to use a view results in undefined behavior.
 *
 * @tparam Entity A valid entity type (see entt_traits for more details).
 * @tparam Component Type of component iterated by the view.
 */
template<typename Entity, typename Component>
class basic_view<Entity, exclude_t<>, Component> {
    /*! @brief A registry is allowed to create views. */
    friend class basic_registry<Entity>;

    using pool_type = std::conditional_t<std::is_const_v<Component>, const storage<Entity, std::remove_const_t<Component>>, storage<Entity, Component>>;

    basic_view(pool_type &ref) ENTT_NOEXCEPT
        : pool{&ref}
    {}

public:
    /*! @brief Type of component iterated by the view. */
    using raw_type = Component;
    /*! @brief Underlying entity identifier. */
    using entity_type = Entity;
    /*! @brief Unsigned integer type. */
    using size_type = std::size_t;
    /*! @brief Input iterator type. */
    using iterator_type = typename sparse_set<Entity>::iterator_type;

    /**
     * @brief Returns the number of entities that have the given component.
     * @return Number of entities that have the given component.
     */
    size_type size() const ENTT_NOEXCEPT {
        return pool->size();
    }

    /**
     * @brief Checks whether a view is empty.
     * @return True if the view is empty, false otherwise.
     */
    bool empty() const ENTT_NOEXCEPT {
        return pool->empty();
    }

    /**
     * @brief Direct access to the list of components.
     *
     * The returned pointer is such that range `[raw(), raw() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the components. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
     *
     * @return A pointer to the array of components.
     */
    raw_type * raw() const ENTT_NOEXCEPT {
        return pool->raw();
    }

    /**
     * @brief Direct access to the list of entities.
     *
     * The returned pointer is such that range `[data(), data() + size()]` is
     * always a valid range, even if the container is empty.
     *
     * @note
     * There are no guarantees on the order of the entities. Use `begin` and
     * `end` if you want to iterate the view in the expected order.
     *
     * @return A pointer to the array of entities.
     */
    const entity_type * data() const ENTT_NOEXCEPT {
        return pool->data();
    }

    /**
     * @brief Returns an iterator to the first entity that has the given
     * component.
     *
     * The returned iterator points to the first entity that has the given
     * component. If the view is empty, the returned iterator will be equal to
     * `end()`.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the first entity that has the given component.
     */
    iterator_type begin() const ENTT_NOEXCEPT {
        return pool->sparse_set<Entity>::begin();
    }

    /**
     * @brief Returns an iterator that is past the last entity that has the
     * given component.
     *
     * The returned iterator points to the entity following the last entity that
     * has the given component. Attempting to dereference the returned iterator
     * results in undefined behavior.
     *
     * @note
     * Input iterators stay true to the order imposed to the underlying data
     * structures.
     *
     * @return An iterator to the entity following the last entity that has the
     * given component.
     */
    iterator_type end() const ENTT_NOEXCEPT {
        return pool->sparse_set<Entity>::end();
    }

    /**
     * @brief Returns the first entity that has the given component, if any.
     * @return The first entity that has the given component if one exists, the
     * null entity otherwise.
     */
    entity_type front() const {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    /**
     * @brief Returns the last entity that has the given component, if any.
     * @return The last entity that has the given component if one exists, the
     * null entity otherwise.
     */
    entity_type back() const {
        const auto it = std::make_reverse_iterator(end());
        return it != std::make_reverse_iterator(begin()) ? *it : null;
    }

    /**
     * @brief Finds an entity.
     * @param entt A valid entity identifier.
     * @return An iterator to the given entity if it's found, past the end
     * iterator otherwise.
     */
    iterator_type find(const entity_type entt) const {
        const auto it = pool->find(entt);
        return it != end() && *it == entt ? it : end();
    }

    /**
     * @brief Returns the identifier that occupies the given position.
     * @param pos Position of the element to return.
     * @return The identifier that occupies the given position.
     */
    entity_type operator[](const size_type pos) const {
        return begin()[pos];
    }

    /**
     * @brief Checks if a view contains an entity.
     * @param entt A valid entity identifier.
     * @return True if the view contains the given entity, false otherwise.
     */
    bool contains(const entity_type entt) const {
        return pool->has(entt);
    }

    /**
     * @brief Returns the component assigned to the given entity.
     *
     * Prefer this function instead of `registry::get` during iterations. It has
     * far better performance than its counterpart.
     *
     * @warning
     * Attempting to use an entity that doesn't belong to the view results in
     * undefined behavior.<br/>
     * An assertion will abort the execution at runtime in debug mode if the
     * view doesn't contain the given entity.
     *
     * @param entt A valid entity identifier.
     * @return The component assigned to the entity.
     */
    template<typename Comp = Component>
    decltype(auto) get(const entity_type entt) const {
        static_assert(std::is_same_v<Comp, Component>);
        ENTT_ASSERT(contains(entt));
        return pool->get(entt);
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a reference to its component. The _constness_ of the
     * component is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms:
     *
     * @code{.cpp}
     * void(const entity_type, Component &);
     * void(Component &);
     * @endcode
     *
     * @note
     * Empty types aren't explicitly instantiated. Therefore, temporary objects
     * are returned during iterations. They can be caught only by copy or with
     * const references.
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void each(Func func) const {
        if constexpr(std::is_invocable_v<Func, decltype(get({}))>) {
            for(auto &&component: *pool) {
                func(component);
            }
        } else {
            auto raw = pool->begin();

            for(const auto entt: *this) {
                func(entt, *(raw++));
            }
        }
    }

    /**
     * @brief Iterates entities and components and applies the given function
     * object to them.
     *
     * The function object is invoked for each entity. It is provided with the
     * entity itself and a reference to its component if it's a non-empty one.
     * The _constness_ of the component is as requested.<br/>
     * The signature of the function must be equivalent to one of the following
     * forms in case the component isn't an empty one:
     *
     * @code{.cpp}
     * void(const entity_type, Component &);
     * void(Component &);
     * @endcode
     *
     * In case the component is an empty one instead, the following forms are
     * accepted:
     *
     * @code{.cpp}
     * void(const entity_type);
     * void();
     * @endcode
     *
     * @sa each
     *
     * @tparam Func Type of the function object to invoke.
     * @param func A valid function object.
     */
    template<typename Func>
    void less(Func func) const {
        if constexpr(ENTT_ENABLE_ETO(Component)) {
            if constexpr(std::is_invocable_v<Func>) {
                for(auto pos = pool->size(); pos; --pos) {
                    func();
                }
            } else {
                for(const auto entt: *this) {
                    func(entt);
                }
            }
        } else {
            each(std::move(func));
        }
    }

private:
    pool_type *pool;
};


}


#endif
