#pragma once
#include <graphene/chain/protocol/types.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>

namespace graphene { namespace chain {

class activenode_schedule_object;

class activenode_schedule_object : public graphene::db::abstract_object<activenode_schedule_object>
{
   public:
      static const uint8_t space_id = implementation_ids;
      static const uint8_t type_id = impl_activenode_schedule_object_type;

      vector< activenode_id_type > current_shuffled_activenodes;
};

} }

FC_REFLECT_DERIVED(
   graphene::chain::activenode_schedule_object,
   (graphene::db::object),
   (current_shuffled_activenodes)
)
