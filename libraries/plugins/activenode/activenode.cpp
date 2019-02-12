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
#include <graphene/activenode/activenode.hpp>
#include <graphene/chain/protocol/transaction.hpp>
#include <graphene/chain/protocol/types.hpp>
#include <graphene/chain/activenode_schedule_object.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/activenode_object.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>
#include <algorithm>

using namespace graphene::activenode_plugin;
using std::string;
using std::vector;

namespace bpo = boost::program_options;

void activenode_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   auto default_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("nathan")));
   command_line_options.add_options()
         ("activenode-account", bpo::value<string>()->composing()->multitoken(),
          "Name of account that is an activenode")
         ("activenode-private-key", bpo::value<vector<string>>()->composing()->multitoken()->
          DEFAULT_VALUE_VECTOR(std::make_pair(chain::public_key_type(default_priv_key.get_public_key()), graphene::utilities::key_to_wif(default_priv_key))),
          "Tuple of [PublicKey, WIF private key] (for account that will get rewards for being an activenode)")
         ;
   config_file_options.add(command_line_options);
}

std::string activenode_plugin::plugin_name()const
{
   return "activenode";
}

void activenode_plugin::plugin_initialize(const boost::program_options::variables_map& options)
{ try {
   ilog("activenode plugin:  plugin_initialize() begin");
   _options = &options;
   if (options.count("activenode-account")) {
      _activenode_account_name = options["activenode-account"].as<std::string>();
   } else {
      elog("activenode-account field is empty! Please add activenode account to configuration");
   }
   if( options.count("activenode-private-key") )
   {
      const std::vector<std::string> key_id_to_wif_pair_strings = options["activenode-private-key"].as<std::vector<std::string>>();
      for (const std::string& key_id_to_wif_pair_string : key_id_to_wif_pair_strings)
      {
         auto key_id_to_wif_pair = graphene::app::dejsonify<std::pair<chain::public_key_type, std::string> >(key_id_to_wif_pair_string, GRAPHENE_MAX_NESTED_OBJECTS);
         idump((key_id_to_wif_pair));
         fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(key_id_to_wif_pair.second);
         if (!private_key)
         {
            // the key isn't in WIF format; see if they are still passing the old native private key format.  This is
            // just here to ease the transition, can be removed soon
            try
            {
               private_key = fc::variant( key_id_to_wif_pair.second, GRAPHENE_MAX_NESTED_OBJECTS ).as<fc::ecc::private_key>( GRAPHENE_MAX_NESTED_OBJECTS );
            }
            catch (const fc::exception&)
            {
               FC_THROW("Invalid WIF-format private key ${key_string}", ("key_string", key_id_to_wif_pair.second));
            }
         }
         _private_key = std::make_pair(key_id_to_wif_pair.first, *private_key);
      }
   } else {
      elog("activenode-private-key field is empty! Please add activenode private key to configuration");
   }
   ilog("activenode plugin:  plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void activenode_plugin::plugin_startup()
{ try {
   ilog("activenode plugin:  plugin_startup() begin");
   if (_activenode_account_name.size()) {
      chain::database& db = database();
      const auto& account_idx = db.get_index_type<account_index>().indices().get<by_name>();
      const auto anode_account_itr = account_idx.find(_activenode_account_name);
      if (anode_account_itr != account_idx.end()) {
         _activenode_account_id = anode_account_itr->id;
         db.new_block_applied.connect( [&]( const signed_block& b){ on_new_block_applied(b); } );
      }
      else {
         elog("activenode plugin: invalid activenode-account provided - no activenode associated with account with name ${activenode_account}", ("activenode_account", _activenode_account_name));
      }
   }
   ilog("activenode plugin:  plugin_startup() end");
} FC_CAPTURE_AND_RETHROW() }

void activenode_plugin::plugin_shutdown()
{
   // nothing to do
}

void activenode_plugin::on_new_block_applied(const signed_block& new_block)
{
   chain::database& db = database();

   // check if still exists
   if (_activenode){
      if ( db.find(*_activenode) == nullptr ) {
         ilog("RESET DELETED ACTIVENODE");
         _activenode.reset();
      }
   }

   if (!_activenode) {
      // get activenode from account
      const auto& anode_idx = db.get_index_type<activenode_index>().indices().get<by_account>();
      const auto& anode_itr = anode_idx.find(_activenode_account_id);
      if (anode_itr != anode_idx.end()) {
         _activenode = anode_itr->id;
      }
      else {
         // didn't find any node for account
         return;
      }
   }

   // if we are scheduled - send activity
   
   fc::optional<activenode_id_type> scheduled_activenode = db.get_scheduled_activenode(db.head_block_num());
   fc::limited_mutable_variant_object capture( GRAPHENE_MAX_NESTED_OBJECTS );
   activenode_condition::activenode_condition_enum result;

   if (_activenode != scheduled_activenode)
      return;
   if ((*_activenode)(db).last_activity == fc::time_point::now())
      //double block appied
      return;
   try
   {
      result = send_activity(capture);
   }
   catch( const fc::canceled_exception& )
   {
      //We're trying to exit. Go ahead and let this one out.
      throw;
   }
   catch( const fc::exception& e )
   {
      elog("Got exception while sending activity:\n${e}", ("e", e.to_detail_string()));
      result = activenode_condition::exception_perform_activity;
   }
   switch( result )
   {
      case activenode_condition::performed_activity:
         ilog("Sent activity from anode ${activenode}, endpoint ${endpoint} with timestamp ${timestamp}", (capture));
         break;
      case activenode_condition::not_synced:
         ilog("Not sending activity because activity is disabled until we receive a recent block (see: --enable-stale-activity)");
         break;
      case activenode_condition::not_my_turn:
         break;
      case activenode_condition::deleted:
         return;
      case activenode_condition::not_time_yet:
         break;
      case activenode_condition::no_private_key:
         ilog("Not sending activity because I don't have the private key for ${scheduled_key}", (capture) );
         break;
      case activenode_condition::lag:
         elog("Not sending activity because node didn't wake up within 500ms of the slot time. scheduled=${scheduled_time} now=${now}", (capture));
         break;
      case activenode_condition::no_scheduled_activenodes:
         dlog("No scheduled activenodes");
         break;
      case activenode_condition::not_sending:
         break;
      case activenode_condition::exception_perform_activity:
         elog( "exception sending activity" );
         break;
   }
}

activenode_condition::activenode_condition_enum
activenode_plugin::send_activity( fc::limited_mutable_variant_object& capture )
{
   chain::database& db = database();

   const dynamic_global_property_object& dpo = db.get_dynamic_global_properties();

   fc::variant_object network_info = app().p2p_node()->network_get_info();

   fc::ip::endpoint endpoint = fc::ip::endpoint::from_string(network_info["listening_on"].as_string());
   graphene::net::firewalled_state node_firewalled_state;
   fc::from_variant(network_info["firewalled"], node_firewalled_state, 1);

   if (!endpoint.get_address().is_public_address() || endpoint.get_address() == fc::ip::address("127.0.0.1")) {
      elog("ERROR: node's ip is local ${endpoint}", ("endpoint", endpoint.get_address()));
      return activenode_condition::not_sending;
   }

   if (node_firewalled_state == graphene::net::firewalled_state::firewalled) {
      elog("ERROR: node is firewalled ${endpoint}", ("endpoint", endpoint.get_address()));
      return activenode_condition::not_sending;
   }

   chain::activenode_send_activity_operation send_activity_operation;
   fc::time_point_sec now = db.head_block_time();
   send_activity_operation.timepoint = now;
   send_activity_operation.endpoint = endpoint;
   send_activity_operation.activenode = *_activenode;
   send_activity_operation.activenode_account = (*_activenode)(db).activenode_account;

   graphene::chain::signed_transaction tx;
   tx.operations.push_back( send_activity_operation );

   const graphene::chain::fee_schedule& fee_shed = db.current_fee_schedule();
   for( auto& op : tx.operations )
      fee_shed.set_fee(op);

   tx.validate();
   auto dyn_props = db.get_dynamic_global_properties();
   tx.set_reference_block( dyn_props.head_block_id );
   tx.set_expiration( dyn_props.time + fc::seconds(30) );

   tx.sign( _private_key.second, db.get_chain_id() );
   app().chain_database()->push_transaction(tx);
   if( app().p2p_node() != nullptr )
      app().p2p_node()->broadcast_transaction(tx);

   capture("timestamp", now)("endpoint", endpoint)("activenode", *_activenode);

   return activenode_condition::performed_activity;
}