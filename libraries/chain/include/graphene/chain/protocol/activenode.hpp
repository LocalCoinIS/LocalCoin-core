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
#include <graphene/chain/protocol/base.hpp>
#include <fc/network/ip.hpp>

namespace graphene { namespace chain { 

  /**
    * @brief Create an activenode object, as a bid to hold a witness position on the network.
    * @ingroup operations
    *
    * Accounts which wish to become witnesses may use this operation to create a witness object which stakeholders may
    * vote on to approve its position as a witness.
    */
   struct activenode_create_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset             fee;
      /// The account which owns the activenode. This account pays the fee for this operation.
      account_id_type   activenode_account;

      account_id_type fee_payer()const { return activenode_account; }
      void            validate()const;
   };


  /**
    * @brief Sending activity operation (I'm active)
    * @ingroup operations
    *
    */
   struct activenode_send_activity_operation : public base_operation
   {
      struct fee_parameters_type { uint64_t fee = 10 * GRAPHENE_BLOCKCHAIN_PRECISION; };

      asset             fee;
      /// The account which owns the activenode. This account pays the fee for this operation.
      account_id_type   activenode_account;
      activenode_id_type   activenode;
      fc::time_point_sec timepoint;
      fc::ip::endpoint endpoint;

      account_id_type fee_payer()const { return activenode_account; }
      void            validate()const;
   };

} } // graphene::chain

FC_REFLECT( graphene::chain::activenode_create_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::activenode_create_operation, (fee)(activenode_account) )


FC_REFLECT( graphene::chain::activenode_send_activity_operation::fee_parameters_type, (fee) )
FC_REFLECT( graphene::chain::activenode_send_activity_operation, (fee)(activenode_account)(activenode)(timepoint)(endpoint) )
