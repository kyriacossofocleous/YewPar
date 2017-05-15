#ifndef SKELETONS_BNB_SEQ_HPP
#define SKELETONS_BNB_SEQ_HPP

#include <vector>

#include <hpx/util/tuple.hpp>

namespace skeletons { namespace BnB { namespace Seq {

template <typename Space,
          typename Sol,
          typename Bnd,
          typename Cand,
          typename Gen,
          typename Bound>
void
expand(const Space & space,
      hpx::util::tuple<Sol, Bnd, Cand> & incumbent,
      const hpx::util::tuple<Sol, Bnd, Cand> & n,
      const Gen && gen,
      const Bound && ubound) {
  std::vector<hpx::util::tuple<Sol, Bnd, Cand> > newCands = gen(space, n);
  for (auto const & c : newCands) {

    /* Prune if required */
    if (ubound(space, c) < hpx::util::get<1>(incumbent)) {
      continue;
    }

    /* Update incumbent if required */
    if (hpx::util::get<1>(c) > hpx::util::get<1>(incumbent)) {
      incumbent = c;
    }

    /* Search the child nodes */
    expand(space, incumbent, c, gen, ubound);
  }
}

template <typename Space,
          typename Sol,
          typename Bnd,
          typename Cand,
          typename Gen,
          typename Bound>
hpx::util::tuple<Sol, Bnd, Cand>
search(const Space & space,
       const hpx::util::tuple<Sol, Bnd, Cand> & root,
       const Gen && gen,
       const Bound && ubound) {
  auto incumbent = root;
  expand(space, incumbent, root, gen, ubound);
  return incumbent;
}

}}}

#endif
