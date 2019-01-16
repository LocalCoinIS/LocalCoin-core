/*
 * Copyright (c) 2015 Cryptonomex, Inc., and contributors.
 *
 * The MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <graphene/chain/database.hpp>
#include <graphene/chain/global_property_object.hpp>
#include <graphene/chain/activenode_object.hpp>
#include <graphene/chain/activenode_schedule_object.hpp>

namespace graphene { namespace chain {

using boost::container::flat_set;


fc::optional<activenode_id_type> database::get_scheduled_activenode(uint32_t block_num)const
{
   fc::optional<activenode_id_type> scheduled_node();
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const activenode_schedule_object& aso = activenode_schedule_id_type()(*this);
   if (head_block_num() == 0) { //GENESIS
      return fc::optional<activenode_id_type>();
   }
   if (aso.current_shuffled_activenodes.size() == 0)
      return fc::optional<activenode_id_type>();

   FC_ASSERT(block_num == head_block_num() || block_num == head_block_num() - 1);   
   return aso.current_shuffled_activenodes[block_num - dpo.current_scheduling_block_num];
}

void database::update_activenode_schedule()
{
   const activenode_schedule_object& aso = activenode_schedule_id_type()(*this);
   const global_property_object& gpo = get_global_properties();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   // what happens if/and when there is only one activenode and it's removed from current_activenodes

   // we should update it on maintanance 
   if ( dpo.dynamic_flags & dynamic_global_property_object::maintenance_flag  ||
       (gpo.current_activenodes.size() != 0 && head_block_num() % gpo.current_activenodes.size() == 0) )
   {
      modify( aso, [&]( activenode_schedule_object& _aso )
      {
         _aso.current_shuffled_activenodes.clear();
         _aso.current_shuffled_activenodes.reserve( gpo.current_activenodes.size() );

         for( const activenode_id_type& w : gpo.current_activenodes )
            _aso.current_shuffled_activenodes.push_back( w );

         auto now_hi = uint64_t(head_block_time().sec_since_epoch()) << 32;
         for( uint32_t i = 0; i < _aso.current_shuffled_activenodes.size(); ++i )
         {
            /// High performance random generator
            /// http://xorshift.di.unimi.it/
            uint64_t k = now_hi + uint64_t(i)*2685821657736338717ULL;
            k ^= (k >> 12);
            k ^= (k << 25);
            k ^= (k >> 27);
            k *= 2685821657736338717ULL;

            uint32_t jmax = _aso.current_shuffled_activenodes.size() - i;
            uint32_t j = i + k%jmax;
            std::swap( _aso.current_shuffled_activenodes[i],
                       _aso.current_shuffled_activenodes[j] );
         }
      });
      wlog("scheduled nodes:");
      for( auto& node : aso.current_shuffled_activenodes) {
         wlog("${node}", ("node", node));
      }

      modify( dpo, [&]( dynamic_global_property_object& _dpo )
      {
         _dpo.current_scheduling_block_num = head_block_num();
      } );
   }
}

} }


