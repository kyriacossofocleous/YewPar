#ifndef SKELETONS_SEQ_HPP
#define SKELETONS_SEQ_HPP

#include <vector>
#include <cstdint>

#include "API.hpp"
#include "util/NodeGenerator.hpp"

namespace YewPar { namespace Skeletons {

template <typename Generator, typename ...Args>
struct Seq {
  typedef typename Generator::Nodetype Node;
  typedef typename Generator::Spacetype Space;

  typedef typename API::skeleton_signature::bind<Args...>::type args;

  static constexpr bool isCountNodes = parameter::value_type<args, API::tag::CountNodes_, std::integral_constant<bool, false> >::type::value;

  // TODO: Have a single function that always "counts" but returns an empty vector if we toggle it off at compile time (then we return the right thing in the "search" dispatcher)
  static void expandCounts (const unsigned maxDepth,
                            const Space & space,
                            const Node & n,
                            const unsigned childDepth,
                            std::vector<std::uint64_t> & counts) {
    Generator newCands;
    if constexpr(std::is_same<Space, Empty>::value) {
      newCands = Generator(n);
    } else {
      newCands = Generator(space, n);
    }

    counts[childDepth] += newCands.numChildren;

    if (childDepth == maxDepth) {
      return;
    }

    for (auto i = 0; i < newCands.numChildren; ++i) {
      auto c = newCands.next();
      expandCounts(maxDepth, space, c, childDepth + 1, counts);
    }
  }

  static std::vector<std::uint64_t> countNodes(const unsigned maxDepth,
                                               const Space & space,
                                               const Node & root) {
    // TODO: How to specify maxDepth in an easy way (ideally not at compile time? Or maybe this is okay?)
    std::vector<std::uint64_t> counts(maxDepth + 1);

    counts[0] = 1;

    expandCounts(maxDepth, space, root, 1, counts);

    return counts;
  }

  static auto search(const unsigned maxDepth,
                     const Space & space,
                     const Node & root) {
    if constexpr(isCountNodes) {
      return countNodes(maxDepth, space, root);
    } else {
      static_assert(isCountNodes, "Please provide a supported search type: CountNodes");
    }
  }

};

}}

#endif
