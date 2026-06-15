#pragma once

#include <concepts>
#include <utility>
namespace cassandra::io::concepts {

template <typename T>
concept SequenceContainerConcept = requires(T container) {
    typename T::value_type;

    typename T::iterator;
    typename T::const_iterator;
    typename T::size_type;

    { container.begin() } -> std::same_as<typename T::iterator>;
    { container.end() } -> std::same_as<typename T::iterator>;
    { container.cbegin() } -> std::same_as<typename T::const_iterator>;
    { container.cend() } -> std::same_as<typename T::const_iterator>;
    { container.size() } -> std::convertible_to<typename T::size_type>;
    { container.empty() } -> std::convertible_to<bool>;

    { container.front() } -> std::same_as<typename T::value_type&>;
    { container.back() } -> std::same_as<typename T::value_type&>;
};

template <typename T>
concept MapConcept =
    requires(T container) {
        typename T::value_type;
        typename T::key_type;
        typename T::mapped_type;

        typename T::iterator;
        typename T::const_iterator;
        typename T::size_type;

        { container.begin() } -> std::same_as<typename T::iterator>;
        { container.end() } -> std::same_as<typename T::iterator>;
        { container.cbegin() } -> std::same_as<typename T::const_iterator>;
        { container.cend() } -> std::same_as<typename T::const_iterator>;
        { container.size() } -> std::convertible_to<typename T::size_type>;
        { container.empty() } -> std::convertible_to<bool>;
        {
            typename T::value_type{}
        } -> std::convertible_to<
              std::pair<const typename T::key_type, typename T::mapped_type>>;
    } &&
    (
        // --- Emplace Logic: OR Condition ---
        // Case 1: Associative Maps (returns iterator)
        requires(T container) {
            {
                container.emplace(
                    std::declval<typename T::key_type>(),
                    std::declval<typename T::mapped_type>()
                )
            } -> std::same_as<typename T::iterator>;
        } ||
        // Case 2: Unordered Associative Maps (returns pair<iterator,
        // bool>)
        requires(T container) {
            {
                container.emplace(
                    std::declval<typename T::key_type>(),
                    std::declval<typename T::mapped_type>()
                )
            } -> std::same_as<std::pair<typename T::iterator, bool>>;
        }
    );
}  // namespace cassandra::io::concepts
