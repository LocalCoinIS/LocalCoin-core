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

fc::optional<activenode_id_type> database::get_scheduled_activenode( uint32_t slot_num )const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const activenode_schedule_object& aso = activenode_schedule_id_type()(*this);
   uint64_t current_aslot = dpo.current_aslot + slot_num;

   if (aso.current_shuffled_activenodes.size() == 0) {
      return fc::optional<activenode_id_type>();
   }

   return aso.current_shuffled_activenodes[ current_aslot % aso.current_shuffled_activenodes.size() ];
}

fc::time_point_sec database::get_activenode_slot_time(uint32_t slot_num)const
{
   if( slot_num == 0 )
      return fc::time_point_sec();
   //TODO: if head_block used - maybe we don't need it
   auto interval = block_interval();
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();

   if( head_block_num() == 0 )
   {
      // n.b. first block is at genesis_time plus one block interval
      fc::time_point_sec genesis_time = dpo.time;
      return genesis_time + slot_num * interval;
   }

   int64_t head_block_abs_slot = head_block_time().sec_since_epoch() / interval;
   fc::time_point_sec head_slot_time(head_block_abs_slot * interval);
   // ilog("!#dbg database::get_activenode_slot_time slot_num=${slot_num} head_block_abs_slot=${head_block_abs_slot} head_slot_time=${head_slot_time}, return_val=${return_val}", ("slot_num", slot_num)("head_block_abs_slot",head_block_abs_slot)("head_slot_time", head_slot_time)("return_val", head_slot_time + (slot_num * interval)));

   const global_property_object& gpo = get_global_properties();

   if( dpo.dynamic_flags & dynamic_global_property_object::maintenance_flag )
      slot_num += gpo.parameters.maintenance_skip_slots;

   // "slot 0" is head_slot_time
   // "slot 1" is head_slot_time,
   //   plus maint interval if head block is a maint block
   //   plus block interval if head block is not a maint block
   // ilog("!ANODE get_activenode_slot_time head_block_time = ${hbt}, head_slot_time = ${hst}, hbt + slot*interval = ${hbtplus}", ("hbt", head_block_time().sec_since_epoch())("hst", head_slot_time.sec_since_epoch())("hbtplus", (head_slot_time + (slot_num * interval)).sec_since_epoch()));


   return head_slot_time + (slot_num * interval);
}

uint32_t database::get_activenode_slot_at_time(fc::time_point_sec when)const
{
   fc::time_point_sec first_slot_time = get_activenode_slot_time( 0 );
   // ilog("!ANODE get_activenode_slot_at_time when = ${when}, first_slot_time = ${fst}", ("when", when.sec_since_epoch())("fst", first_slot_time.sec_since_epoch()));

   if( when < first_slot_time )
      return 0;
   return (when - first_slot_time).to_seconds() / block_interval() + 1;
}

void database::update_activenode_schedule()
{
   const activenode_schedule_object& wso = activenode_schedule_id_type()(*this);
   const global_property_object& gpo = get_global_properties();

   if( gpo.current_activenodes.size() != 0 && head_block_num() % gpo.current_activenodes.size() == 0 )
   {
      modify( wso, [&]( activenode_schedule_object& _wso )
      {
         _wso.current_shuffled_activenodes.clear();
         _wso.current_shuffled_activenodes.reserve( gpo.current_activenodes.size() );

         for( const activenode_id_type& w : gpo.current_activenodes )
            _wso.current_shuffled_activenodes.push_back( w );

         auto now_hi = uint64_t(head_block_time().sec_since_epoch()) << 32;
         for( uint32_t i = 0; i < _wso.current_shuffled_activenodes.size(); ++i )
         {
            /// High performance random generator
            /// http://xorshift.di.unimi.it/
            uint64_t k = now_hi + uint64_t(i)*2685821657736338717ULL;
            k ^= (k >> 12);
            k ^= (k << 25);
            k ^= (k >> 27);
            k *= 2685821657736338717ULL;

            uint32_t jmax = _wso.current_shuffled_activenodes.size() - i;
            uint32_t j = i + k%jmax;
            std::swap( _wso.current_shuffled_activenodes[i],
                       _wso.current_shuffled_activenodes[j] );
         }
      });
   }
}

} }


