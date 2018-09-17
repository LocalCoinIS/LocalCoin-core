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


namespace graphene { namespace chain {

void_result activenode_create_evaluator::do_evaluate( const activenode_create_operation& op )
{ try {
   FC_ASSERT(db().get(op.activenode_account).is_lifetime_member());
   return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }

object_id_type activenode_create_evaluator::do_apply( const activenode_create_operation& op )
{ try {
   const auto& new_activenode_object = db().create<activenode_object>( [&]( activenode_object& obj ){
         obj.activenode_account  = op.activenode_account;
         obj.is_enabled = true;
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

object_id_type activenode_activity_evaluator::do_apply( const activenode_send_activity_operation& op )
{ try {

   // DONT validate balance & remove 

   // get timestamp from ...
   fc::time_point now = fc::time_point::now();

   // modify activenode with new data

   database& _db = db();
   _db.modify(
      _db.get(op.activenode),
      [&]( activenode_object& activenode_object )
      {
         activenode_object.last_activity = op.timepoint;
         activenode_object.endpoint = op.endpoint;
      });
      return void_result();
} FC_CAPTURE_AND_RETHROW( (op) ) }
} } // graphene::chain
