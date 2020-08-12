#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <new>

/// A dense table-based deterministic finite automaton (DFA).
///
/// A dense DFA represents the core matching primitive in this crate. That is,
/// logically, all DFAs have a single start state, one or more match states
/// and a transition table that maps the current state and the current byte of
/// input to the next state. A DFA can use this information to implement fast
/// searching. In particular, the use of a dense DFA generally makes the trade
/// off that match speed is the most valuable characteristic, even if building
/// the regex may take significant time *and* space. As such, the processing
/// of every byte of input is done with a small constant number of operations
/// that does not vary with the pattern, its size or the size of the alphabet.
/// If your needs don't line up with this trade off, then a dense DFA may not
/// be an adequate solution to your problem.
///
/// In contrast, a [sparse DFA](enum.SparseDFA.html) makes the opposite
/// trade off: it uses less space but will execute a variable number of
/// instructions per byte at match time, which makes it slower for matching.
///
/// A DFA can be built using the default configuration via the
/// [`DenseDFA::new`](enum.DenseDFA.html#method.new) constructor. Otherwise,
/// one can configure various aspects via the
/// [`dense::Builder`](dense/struct.Builder.html).
///
/// A single DFA fundamentally supports the following operations:
///
/// 1. Detection of a match.
/// 2. Location of the end of the first possible match.
/// 3. Location of the end of the leftmost-first match.
///
/// A notable absence from the above list of capabilities is the location of
/// the *start* of a match. In order to provide both the start and end of a
/// match, *two* DFAs are required. This functionality is provided by a
/// [`Regex`](struct.Regex.html), which can be built with its basic
/// constructor, [`Regex::new`](struct.Regex.html#method.new), or with
/// a [`RegexBuilder`](struct.RegexBuilder.html).
///
/// # State size
///
/// A `DenseDFA` has two type parameters, `T` and `S`. `T` corresponds to
/// the type of the DFA's transition table while `S` corresponds to the
/// representation used for the DFA's state identifiers as described by the
/// [`StateID`](trait.StateID.html) trait. This type parameter is typically
/// `usize`, but other valid choices provided by this crate include `u8`,
/// `u16`, `u32` and `u64`. The primary reason for choosing a different state
/// identifier representation than the default is to reduce the amount of
/// memory used by a DFA. Note though, that if the chosen representation cannot
/// accommodate the size of your DFA, then building the DFA will fail and
/// return an error.
///
/// While the reduction in heap memory used by a DFA is one reason for choosing
/// a smaller state identifier representation, another possible reason is for
/// decreasing the serialization size of a DFA, as returned by
/// [`to_bytes_little_endian`](enum.DenseDFA.html#method.to_bytes_little_endian),
/// [`to_bytes_big_endian`](enum.DenseDFA.html#method.to_bytes_big_endian)
/// or
/// [`to_bytes_native_endian`](enum.DenseDFA.html#method.to_bytes_native_endian).
///
/// The type of the transition table is typically either `Vec<S>` or `&[S]`,
/// depending on where the transition table is stored.
///
/// # Variants
///
/// This DFA is defined as a non-exhaustive enumeration of different types of
/// dense DFAs. All of these dense DFAs use the same internal representation
/// for the transition table, but they vary in how the transition table is
/// read. A DFA's specific variant depends on the configuration options set via
/// [`dense::Builder`](dense/struct.Builder.html). The default variant is
/// `PremultipliedByteClass`.
///
/// # The `DFA` trait
///
/// This type implements the [`DFA`](trait.DFA.html) trait, which means it
/// can be used for searching. For example:
///
/// ```
/// use regex_automata::{DFA, DenseDFA};
///
/// # fn example() -> Result<(), regex_automata::Error> {
/// let dfa = DenseDFA::new("foo[0-9]+")?;
/// assert_eq!(Some(8), dfa.find(b"foo12345"));
/// # Ok(()) }; example().unwrap()
/// ```
///
/// The `DFA` trait also provides an assortment of other lower level methods
/// for DFAs, such as `start_state` and `next_state`. While these are correctly
/// implemented, it is an anti-pattern to use them in performance sensitive
/// code on the `DenseDFA` type directly. Namely, each implementation requires
/// a branch to determine which type of dense DFA is being used. Instead,
/// this branch should be pushed up a layer in the code since walking the
/// transitions of a DFA is usually a hot path. If you do need to use these
/// lower level methods in performance critical code, then you should match on
/// the variants of this DFA and use each variant's implementation of the `DFA`
/// trait directly.
template<typename T, typename S>
struct DenseDFA;

/// A regular expression that uses deterministic finite automata for fast
/// searching.
///
/// A regular expression is comprised of two DFAs, a "forward" DFA and a
/// "reverse" DFA. The forward DFA is responsible for detecting the end of a
/// match while the reverse DFA is responsible for detecting the start of a
/// match. Thus, in order to find the bounds of any given match, a forward
/// search must first be run followed by a reverse search. A match found by
/// the forward DFA guarantees that the reverse DFA will also find a match.
///
/// The type of the DFA used by a `Regex` corresponds to the `D` type
/// parameter, which must satisfy the [`DFA`](trait.DFA.html) trait. Typically,
/// `D` is either a [`DenseDFA`](enum.DenseDFA.html) or a
/// [`SparseDFA`](enum.SparseDFA.html), where dense DFAs use more memory but
/// search faster, while sparse DFAs use less memory but search more slowly.
///
/// By default, a regex's DFA type parameter is set to
/// `DenseDFA<Vec<usize>, usize>`. For most in-memory work loads, this is the
/// most convenient type that gives the best search performance.
///
/// # Sparse DFAs
///
/// Since a `Regex` is generic over the `DFA` trait, it can be used with any
/// kind of DFA. While this crate constructs dense DFAs by default, it is easy
/// enough to build corresponding sparse DFAs, and then build a regex from
/// them:
///
/// ```
/// use regex_automata::Regex;
///
/// # fn example() -> Result<(), regex_automata::Error> {
/// // First, build a regex that uses dense DFAs.
/// let dense_re = Regex::new("foo[0-9]+")?;
///
/// // Second, build sparse DFAs from the forward and reverse dense DFAs.
/// let fwd = dense_re.forward().to_sparse()?;
/// let rev = dense_re.reverse().to_sparse()?;
///
/// // Third, build a new regex from the constituent sparse DFAs.
/// let sparse_re = Regex::from_dfas(fwd, rev);
///
/// // A regex that uses sparse DFAs can be used just like with dense DFAs.
/// assert_eq!(true, sparse_re.is_match(b"foo123"));
/// # Ok(()) }; example().unwrap()
/// ```
template<typename D>
struct Regex;

/// A regular expression that uses deterministic finite automata for fast
/// searching.
///
/// A regular expression is comprised of two DFAs, a "forward" DFA and a
/// "reverse" DFA. The forward DFA is responsible for detecting the end of a
/// match while the reverse DFA is responsible for detecting the start of a
/// match. Thus, in order to find the bounds of any given match, a forward
/// search must first be run followed by a reverse search. A match found by
/// the forward DFA guarantees that the reverse DFA will also find a match.
///
/// The type of the DFA used by a `Regex` corresponds to the `D` type
/// parameter, which must satisfy the [`DFA`](trait.DFA.html) trait. Typically,
/// `D` is either a [`DenseDFA`](enum.DenseDFA.html) or a
/// [`SparseDFA`](enum.SparseDFA.html), where dense DFAs use more memory but
/// search faster, while sparse DFAs use less memory but search more slowly.
///
/// When using this crate without the standard library, the `Regex` type has
/// no default type parameter.
///
/// # Sparse DFAs
///
/// Since a `Regex` is generic over the `DFA` trait, it can be used with any
/// kind of DFA. While this crate constructs dense DFAs by default, it is easy
/// enough to build corresponding sparse DFAs, and then build a regex from
/// them:
///
/// ```
/// use regex_automata::Regex;
///
/// # fn example() -> Result<(), regex_automata::Error> {
/// // First, build a regex that uses dense DFAs.
/// let dense_re = Regex::new("foo[0-9]+")?;
///
/// // Second, build sparse DFAs from the forward and reverse dense DFAs.
/// let fwd = dense_re.forward().to_sparse()?;
/// let rev = dense_re.reverse().to_sparse()?;
///
/// // Third, build a new regex from the constituent sparse DFAs.
/// let sparse_re = Regex::from_dfas(fwd, rev);
///
/// // A regex that uses sparse DFAs can be used just like with dense DFAs.
/// assert_eq!(true, sparse_re.is_match(b"foo123"));
/// # Ok(()) }; example().unwrap()
/// ```
template<typename D>
struct Regex;

template<typename T>
struct Vec;

extern "C" {

Regex<DenseDFA<Vec<uintptr_t>, uintptr_t>> *regex_create(const char *pattern);

uintptr_t regex_match(Regex<DenseDFA<Vec<uintptr_t>, uintptr_t>> *re, const char *text);

} // extern "C"
