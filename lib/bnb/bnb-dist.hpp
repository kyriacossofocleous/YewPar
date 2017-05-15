#ifndef SKELETONS_BNB_DIST_HPP
#define SKELETONS_BNB_DIST_HPP

#include <vector>
#include <hpx/util/tuple.hpp>
#include <hpx/lcos/broadcast.hpp>

#include "registry.hpp"
#include "incumbent.hpp"

#include "workstealing/scheduler.hpp"
#include "workstealing/workqueue.hpp"

namespace skeletons { namespace BnB { namespace Dist {

template <typename Space,
          typename Sol,
          typename Bnd,
          typename Cand,
          typename Gen,
          typename Bound,
          typename ChildTask>
void
expand(unsigned spawnDepth,
       const hpx::util::tuple<Sol, Bnd, Cand> & n) {

  auto reg = skeletons::BnB::Components::Registry<Space,Bnd>::gReg;

  Gen gen;
  std::vector< hpx::util::tuple<Sol, Bnd, Cand> > newCands = gen(hpx::find_here(), reg->space_, n);

  std::vector<hpx::future<void> > childFuts;
  if (spawnDepth > 0) {
    childFuts.reserve(newCands.size());
  }

  for (auto const & c : newCands) {
    auto lbnd = reg->localBound_.load();

    /* Prune if required */
    Bound ubound;
    if (ubound(hpx::find_here(), reg->space_, c) < lbnd) {
      continue;
    }

    /* Update incumbent if required */
    if (hpx::util::get<1>(c) > lbnd) {
      typedef typename bounds::Incumbent<Sol, Bnd, Cand>::updateIncumbent_action act;
      hpx::async<act>(reg->globalIncumbent_, c).get();
      reg->localBound_.store(hpx::util::get<1>(c));
      hpx::lcos::broadcast<updateRegistryBound_act>(hpx::find_all_localities(), hpx::util::get<1>(c));
    }

    /* Search the child nodes */
    if (spawnDepth > 0) {
      hpx::lcos::promise<void> prom;
      auto pfut = prom.get_future();
      auto pid  = prom.get_id();

      ChildTask t;
      hpx::util::function<void(hpx::naming::id_type)> task;
      task = hpx::util::bind(t, hpx::util::placeholders::_1, spawnDepth - 1, c, pid);
      hpx::apply<workstealing::workqueue::addWork_action>(workstealing::local_workqueue, task);

      childFuts.push_back(std::move(pfut));
    } else {
      expand<Space, Sol, Bnd, Cand, Gen, Bound, ChildTask>(spawnDepth - 1, c);
    }
  }

  if (spawnDepth > 0) {
    workstealing::tasks_required_sem.signal(); // Going to sleep, get more tasks
    hpx::wait_all(childFuts);
  }
}

template <typename Space,
        typename Sol,
        typename Bnd,
        typename Cand,
        typename Gen,
        typename Bound,
        typename ChildTask>
hpx::util::tuple<Sol, Bnd, Cand>
search(unsigned spawnDepth,
       const Space & space,
       const hpx::util::tuple<Sol, Bnd, Cand> & root) {

  // Initialise the registries on all localities
  auto bnd = hpx::util::get<1>(root);
  auto inc = hpx::new_<bounds::Incumbent<Sol, Bnd, Cand> >(hpx::find_here()).get();
  hpx::wait_all(hpx::lcos::broadcast<initRegistry_act>(hpx::find_all_localities(), space, bnd, inc));

  // Initialise the global incumbent
  typedef typename bounds::Incumbent<Sol, Bnd, Cand>::updateIncumbent_action updateInc;
  hpx::async<updateInc>(inc, root).get();

  // Start work stealing schedulers on all localities
  std::vector<hpx::naming::id_type> workqueues;
  for (auto const& loc : hpx::find_all_localities()) {
    workqueues.push_back(hpx::new_<workstealing::workqueue>(loc).get());
  }
  hpx::wait_all(hpx::lcos::broadcast<startScheduler_action>(hpx::find_all_localities(), workqueues));

  expand<Space, Sol, Bnd, Cand, Gen, Bound, ChildTask>(spawnDepth, root);

  // Stop all work stealing schedulers
  hpx::wait_all(hpx::lcos::broadcast<stopScheduler_action>(hpx::find_all_localities()));

  // Read the result form the global incumbent
  typedef typename bounds::Incumbent<Sol, Bnd, Cand>::getIncumbent_action getInc;
  return hpx::async<getInc>::getIncumbent_action>(inc).get();
}

template <typename Space,
          typename Sol,
          typename Bnd,
          typename Cand,
          typename Gen,
          typename Bound,
          typename ChildTask>
void
searchChildTask(unsigned spawnDepth,
                hpx::util::tuple<Sol, Bnd, Cand> c,
                hpx::naming::id_type p) {
  expand<Space, Sol, Bnd, Cand, Gen, Bound, ChildTask>(spawnDepth, c);
  workstealing::tasks_required_sem.signal();
  hpx::async<hpx::lcos::base_lco_with_value<void>::set_value_action>(p, true).get();
}

}}}

#endif
