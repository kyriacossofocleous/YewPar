#ifndef BNB_REGISTRY_HPP
#define BNB_REGISTRY_HPP

namespace skeletons { namespace BnB { namespace Components {

      template <typename Space, typename Bnd>
      struct Registry {
        static Registry<Space, Bnd>* gReg;

        Space space_;
        std::atomic<Bnd> localBound_;
        hpx::naming::id_type globalIncumbent_;

        // For decision based problems
        std::atomic<bool> stopSearch_ {false};
      };

      template<typename Space, typename Bnd>
      Registry<Space, Bnd>* Registry<Space,Bnd>::gReg = new Registry<Space, Bnd>;

      template <typename Space, typename Bnd>
      void initialiseRegistry(Space space, Bnd bnd, hpx::naming::id_type inc) {
        auto reg = Registry<Space,Bnd>::gReg;
        reg->space_ = space;
        reg->localBound_ = bnd;
        reg->globalIncumbent_ = inc;
      }

      template <typename Space, typename Bnd>
      void updateRegistryBound(Bnd bnd) {
        auto reg = Registry<Space,Bnd>::gReg;
        while (true) {
          auto curBound = reg->localBound_.load();
          if (bnd < curBound) {
            break;
          }

          if (reg->localBound_.compare_exchange_weak(curBound, bnd)) {
            break;
          }
        }
      }

      // FIXME: Fake arg needed for the action to find the correct function. Not sure why.
      template <typename Space, typename Bnd>
      void setStopSearchFlag(Bnd fake) {
        auto reg = Registry<Space,Bnd>::gReg;
        reg->stopSearch_.store(true);
      }
}}}

HPX_DECLARE_PLAIN_ACTION(skeletons::BnB::Components::initialiseRegistry, initRegistry_act);
HPX_DECLARE_PLAIN_ACTION(skeletons::BnB::Components::updateRegistryBound, updateRegistryBound_act);
HPX_DECLARE_PLAIN_ACTION(skeletons::BnB::Components::setStopSearchFlag, setStopSearchFlag_act);

#define COMMA ,
#define REGISTER_REGISTRY(space,bnd)                                                 \
  struct initRegistry_act : hpx::actions::make_action<                               \
    decltype(&skeletons::BnB::Components::initialiseRegistry<space COMMA bnd >),     \
    &skeletons::BnB::Components::initialiseRegistry<space COMMA bnd >,               \
    initRegistry_act>::type {};                                                      \
                                                                                     \
  HPX_REGISTER_ACTION_DECLARATION(initRegistry_act, initRegistry_act);               \
  HPX_REGISTER_ACTION(initRegistry_act, initRegistry_act);                           \
                                                                                     \
  struct updateRegistryBound_act : hpx::actions::make_action<                        \
    decltype(&skeletons::BnB::Components::updateRegistryBound<space COMMA bnd >),    \
    &skeletons::BnB::Components::updateRegistryBound<space COMMA bnd >,              \
    updateRegistryBound_act>::type {};                                               \
                                                                                     \
  HPX_REGISTER_ACTION_DECLARATION(updateRegistryBound_act, updateRegistryBound_act); \
  HPX_REGISTER_ACTION(updateRegistryBound_act, updateRegistryBound_act);             \
                                                                                     \
  struct setStopSearchFlag_act : hpx::actions::make_action<                          \
    decltype(&skeletons::BnB::Components::setStopSearchFlag<space COMMA bnd >),      \
      &skeletons::BnB::Components::setStopSearchFlag<space COMMA bnd >,              \
      setStopSearchFlag_act>::type {};                                               \
                                                                                     \
  HPX_REGISTER_ACTION_DECLARATION(setStopSearchFlag_act, setStopSearchFlag_act);     \
  HPX_REGISTER_ACTION(setStopSearchFlag_act, setStopSearchFlag_act);                 \

#endif
