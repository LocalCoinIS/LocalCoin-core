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
#pragma once
#include <graphene/chain/protocol/asset.hpp>
#include <graphene/db/object.hpp>
#include <graphene/db/generic_index.hpp>
#include <graphene/chain/protocol/types.hpp>


namespace graphene { namespace chain {
   using namespace graphene::db;
   class activenode_object;

   class activenode_object : public abstract_object<activenode_object>
   {
      public:
         static const uint8_t space_id = protocol_ids;
         static const uint8_t type_id = activenode_object_type;

         account_id_type  activenode_account;
         fc::time_point_sec last_activity;
         
         uint32_t activities_sent = 0; // amount of activities sent in this interval
         uint8_t penalty_left = 0; // how many maintenance intervals do we need to wait before we'll become active
         uint8_t max_penalty = 0; // max penalty that we've got

         bool is_new = true; // have the activenode already participated in scheduling

         fc::ip::endpoint endpoint;
         optional< vesting_balance_id_type > pay_vb;
         activenode_object() {}
   };

   struct by_account;
   using activenode_multi_index_type = multi_index_container<
      activenode_object,
      indexed_by<
         ordered_unique< tag<by_id>,
            member<object, object_id_type, &object::id>
         >,
         ordered_unique< tag<by_account>,
            member<activenode_object, account_id_type, &activenode_object::activenode_account>
         >
      >
   >;
   using activenode_index = generic_index<activenode_object, activenode_multi_index_type>;
} } // graphene::chain

FC_REFLECT_DERIVED( graphene::chain::activenode_object, (graphene::db::object),
                    (activenode_account)
                    (last_activity)
                    (endpoint)
                    (pay_vb)
                    (activities_sent)
                    (penalty_left)
                    (max_penalty)
                    (is_new)
                  )