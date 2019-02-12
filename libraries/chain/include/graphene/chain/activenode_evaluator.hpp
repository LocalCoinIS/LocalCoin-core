#pragma once
#include <graphene/chain/evaluator.hpp>
#include <graphene/chain/activenode_object.hpp>
#include <graphene/chain/protocol/activenode.hpp>
namespace graphene { namespace chain {

   class activenode_create_evaluator : public evaluator<activenode_create_evaluator>
   {
      public:
         typedef activenode_create_operation operation_type;

         void_result do_evaluate( const activenode_create_operation& o );
         object_id_type do_apply( const activenode_create_operation& o );
   };

      class activenode_activity_evaluator : public evaluator<activenode_activity_evaluator>
   {
      public:
         typedef activenode_send_activity_operation operation_type;

         void_result do_evaluate( const activenode_send_activity_operation& o );
         void_result do_apply( const activenode_send_activity_operation& o );
   };
} } // graphene::chain
