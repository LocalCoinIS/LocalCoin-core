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
#include <graphene/chain/activenode_evaluator.hpp>
#include <graphene/chain/activenode_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/account_object.hpp>
#include <graphene/chain/database.hpp>
#include <graphene/chain/protocol/vote.hpp>
#include <graphene/chain/protocol/activenode.hpp>
#include <graphene/chain/vesting_balance_object.hpp>


namespace graphene { namespace chain {

void_result activenode_create_evaluator::do_evaluate( const activenode_create_operation& op )
{ try {

   database& d = db();
   auto& account_obj = d.get(op.activenode_account);
   FC_ASSERT(d.get(op.activenode_account).is_lifetime_member());

   share_type total_balance = d.get_total_account_balance(account_obj);
   FC_ASSERT(total_balance >= LLC_ACTIVENODE_MINIMAL_BALANCE_CREATE, "Insufficient Balance: ${a}'s balance of ${b} is less than minimal activenode balance required ${r}", 
                 ("a",account_obj.name)
                 ("b",d.to_pretty_string(asset(total_balance)))
                 ("r",d.to_pretty_string(asset(LLC_ACTIVENODE_MINIMAL_BALANCE_CREATE))));
                 
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type activenode_create_evaluator::do_apply( const activenode_create_operation& op )
{ try {
   const auto& new_activenode_object = db().create<activenode_object>( [&]( activenode_object& obj ){
         obj.activenode_account  = op.activenode_account;
   });
   return new_activenode_object.id;
} FC_CAPTURE_AND_RETHROW( (op) ) }


void_result activenode_activity_evaluator::do_evaluate( const activenode_send_activity_operation& op )
{ try {
   FC_ASSERT(db().get(op.activenode_account).is_lifetime_member());
   // here we validate that action is possible
   const auto& anode_idx = db().get_index_type<activenode_index>().indices().get<by_account>();
   FC_ASSERT(anode_idx.find(op.activenode_account) != anode_idx.end(), "Account ${acc} is not a registered activenode", ("acc", op.activenode_account));
   FC_ASSERT(db().get(op.activenode).activenode_account == op.activenode_account);

   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

void_result activenode_activity_evaluator::do_apply( const activenode_send_activity_operation& op )
{ try {
   database& _db = db();
   _db.modify(
      _db.get(op.activenode),
      [&]( activenode_object& activenode_object )
      {
         activenode_object.last_activity = db().head_block_time();
         activenode_object.endpoint = op.endpoint;
      });
      return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }
} } // graphene::chain
