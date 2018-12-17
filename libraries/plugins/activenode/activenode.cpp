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
   chain::database& db = database();
   const auto& account_idx = db.get_index_type<account_index>().indices().get<by_name>();
   const auto anode_account_itr = account_idx.find(_activenode_account_name);
   if (anode_account_itr != account_idx.end()) {
      // get activenode from account
      const auto& anode_idx = db.get_index_type<activenode_index>().indices().get<by_account>();
      const account_object& acc_obj = *anode_account_itr;
      const auto& anode_itr = anode_idx.find(acc_obj.id);
      if (anode_itr != anode_idx.end()) {
         _activenode = anode_itr->id;
      } else {
         database().new_objects.connect([&]( const vector<object_id_type>& ids, const flat_set<account_id_type>& impacted_accounts ){
            const auto impacted_account = impacted_accounts.find(acc_obj.id);
            if (impacted_account != impacted_accounts.end()) {
               // check if we have corresponding activenode
               const auto& anode_idx = db.get_index_type<activenode_index>().indices().get<by_account>();
               const auto& acc_obj = *impacted_account;
               const auto& anode_itr = anode_idx.find(acc_obj);
               if (anode_itr != anode_idx.end() && std::find(ids.begin(), ids.end(), anode_itr->id) != ids.end()) {
                  _activenode = anode_itr->id;
                  schedule_activity_loop();
               }
            }
         });
         ilog("Account ${acc} is not a registered activenode, can't start sending activity", ("acc", _activenode_account_name));
      }
   }
   if (_activenode)
   {
      ilog("Launching activity for an activenodes.");
      schedule_activity_loop();
   } else
      elog("No activenodes configured! Please add activenode account and private key to configuration or register an activenode");
   ilog("activenode plugin:  plugin_startup() end");
} FC_CAPTURE_AND_RETHROW() }

void activenode_plugin::plugin_shutdown()
{
   // nothing to do
}

void activenode_plugin::schedule_activity_loop()
{
   //Schedule for the next second's tick regardless of chain state
   // If we would wait less than 50ms, wait for the whole second.
   fc::time_point now = fc::time_point::now();
   int64_t time_to_next_second = 1000000 - (now.time_since_epoch().count() % 1000000);
   if( time_to_next_second < 50000 )      // we must sleep for at least 50ms
       time_to_next_second += 1000000;

   fc::time_point next_wakeup( now + fc::microseconds( time_to_next_second ) );
   _activity_task = fc::schedule([this]{activity_loop();},
                                         next_wakeup, "Node activity transaction");
}

activenode_condition::activenode_condition_enum activenode_plugin::activity_loop()
{
   activenode_condition::activenode_condition_enum result;
   fc::limited_mutable_variant_object capture( GRAPHENE_MAX_NESTED_OBJECTS );
   try
   {
      chain::database& db = database();

      auto maybe_found = db.find(*_activenode);
      if ( maybe_found == nullptr ) {
         _activenode.reset();
         result = activenode_condition::deleted;
      } else {
         if ((*_activenode)(db).is_enabled)
            result = maybe_send_activity(capture);
         else
            result = activenode_condition::exception_perform_activity;
      }
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
         return result;
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
      case activenode_condition::exception_perform_activity:
         elog( "exception sending activity" );
         break;
   }
   schedule_activity_loop();
   return result;
}

//todo complete rewrite
activenode_condition::activenode_condition_enum
activenode_plugin::maybe_send_activity( fc::limited_mutable_variant_object& capture )
{
   chain::database& db = database();

   const dynamic_global_property_object& dpo = db.get_dynamic_global_properties();

   if( dpo.dynamic_flags & dynamic_global_property_object::maintenance_flag )
      return activenode_condition::not_time_yet;

   fc::time_point now_fine = fc::time_point::now();
   fc::time_point_sec now = now_fine;

   // If the next send activity opportunity is in the present or future, we're synced.
   if( !_activenode_plugin_enabled )
   {
      // TODO: check if it is okay???!?!??!
      // need special slots for sending activity
      if( db.get_activenode_slot_time(1) >= now )
         _activenode_plugin_enabled = true;
      else
         return activenode_condition::not_synced;
   }

   // is anyone scheduled to produce now or one second in the future?

   //always zero
   uint32_t slot = db.get_activenode_slot_at_time( now );

   if( slot == 0 )
   {
      capture("next_time", db.get_activenode_slot_time(1));
      return activenode_condition::not_time_yet;
   }
   //
   // this assert should not fail, because now <= db.head_block_time()
   // should have resulted in slot == 0.
   //
   // if this assert triggers, there is a serious bug in get_slot_at_time()
   // which would result in allowing a later block to have a timestamp
   // less than or equal to the previous block
   //

   fc::optional<activenode_id_type> scheduled_activenode = db.get_scheduled_activenode( slot );
   if (!scheduled_activenode) {
      return activenode_condition::no_scheduled_activenodes;
   }
   if( *scheduled_activenode != *_activenode )
   {
      capture("scheduled_activenode", *scheduled_activenode);
      return activenode_condition::not_my_turn;
   }

   fc::time_point_sec scheduled_time = db.get_slot_time( slot );
   // TODO: check if we need to change it
   // if( llabs((scheduled_time - now).count()) > fc::milliseconds( 500 ).count() )
   // {
   //    capture("scheduled_time", scheduled_time)("now", now);
   //    return activenode_condition::lag;
   // }

   fc::variant_object network_info = app().p2p_node()->network_get_info();

   fc::ip::endpoint endpoint = fc::ip::endpoint::from_string(network_info["listening_on"].as_string());
   graphene::net::firewalled_state node_firewalled_state;
   fc::from_variant(network_info["firewalled"], node_firewalled_state, 1);
   FC_ASSERT(endpoint.get_address().is_public_address() && node_firewalled_state != graphene::net::firewalled_state::firewalled);
   // FC_ASSERT(endpoint.get_address().is_public_address());
   if (!endpoint.get_address().is_public_address())
      elog("ERROR: node's ip is local ${endpoint}", ("endpoint", endpoint.get_address()));
   if (node_firewalled_state == graphene::net::firewalled_state::firewalled)
      elog("ERROR: node is firewalled ${endpoint}", ("endpoint", endpoint.get_address()));

   chain::activenode_send_activity_operation send_activity_operation;
   now = fc::time_point::now();
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
   dlog("maybe_send_activity");
   app().chain_database()->push_transaction(tx);

   capture("timestamp", now)("endpoint", endpoint)("activenode", *_activenode);

   return activenode_condition::performed_activity;
}
