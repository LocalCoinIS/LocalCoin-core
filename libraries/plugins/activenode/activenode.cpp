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

#include <graphene/chain/database.hpp>
#include <graphene/chain/activenode_object.hpp>

#include <graphene/utilities/key_conversion.hpp>

#include <fc/smart_ref_impl.hpp>
#include <fc/thread/thread.hpp>

#include <iostream>

using namespace graphene::activenode_plugin;
using std::string;
using std::vector;

namespace bpo = boost::program_options;

void activenode_plugin::plugin_set_program_options(
   boost::program_options::options_description& command_line_options,
   boost::program_options::options_description& config_file_options)
{
   auto default_priv_key = fc::ecc::private_key::regenerate(fc::sha256::hash(std::string("nathan")));
   string activenode_id_example = fc::json::to_string(chain::activenode_id_type(5));
   command_line_options.add_options()
         ("activenode-id,w", bpo::value<vector<string>>()->composing()->multitoken(),
          ("ID of activenode controlled by this node (e.g. " + activenode_id_example + ", quotes are required").c_str())
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
   _activenode = GRAPHENE_NULL_ACTIVENODE;
   if (options.count("activenode-id"))
   {
      std::string activenode_id = options["activenode-id"].as<std::string>();
 
      _activenode = graphene::app::dejsonify<chain::activenode_id_type>(activenode_id);
   }

   if( options.count("activenode-private-key") )
   {
      const std::string key_id_to_wif_pair_string = options["activenode-private-key"].as<std::string>();
      auto key_id_to_wif_pair = graphene::app::dejsonify<std::pair<chain::public_key_type, std::string> >(key_id_to_wif_pair_string, 5);
      ilog("Public Key: ${public}", ("public", key_id_to_wif_pair.first));
      fc::optional<fc::ecc::private_key> private_key = graphene::utilities::wif_to_key(key_id_to_wif_pair.second);
      if (!private_key)
      {
         // the key isn't in WIF format; see if they are still passing the old native private key format.  This is
         // just here to ease the transition, can be removed soon
         try
         {
            private_key = fc::variant(key_id_to_wif_pair.second, 2).as<fc::ecc::private_key>(1);
         }
         catch (const fc::exception&)
         {
            FC_THROW("Invalid WIF-format private key ${key_string}", ("key_string", key_id_to_wif_pair.second));
         }
      }
      _private_key = std::make_pair(key_id_to_wif_pair.first, *private_key);
   }
   ilog("activenode plugin:  plugin_initialize() end");
} FC_LOG_AND_RETHROW() }

void activenode_plugin::plugin_startup()
{ try {
   ilog("activenode plugin:  plugin_startup() begin");

   if( _activenode != GRAPHENE_NULL_ACTIVENODE)
   {
      ilog("Launching activity for an activenodes.");
      schedule_activity_loop();
   } else
      elog("No activenodes configured! Please add activenode IDs and private keys to configuration.");
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
   ilog("!activenode_plugin::schedule_activity_loop now=${now}", ("now", now));
   int64_t time_to_next_second = 1000000 - (now.time_since_epoch().count() % 1000000);
   if( time_to_next_second < 50000 )      // we must sleep for at least 50ms
       time_to_next_second += 1000000;

   fc::time_point next_wakeup( now + fc::microseconds( time_to_next_second ) );
   ilog("!activenode_plugin::schedule_activity_loop next_wakeup=${next_wakeup}", ("next_wakeup", next_wakeup));
   _activity_task = fc::schedule([this]{_activity_loop();},
                                         next_wakeup, "Node activity transaction");
}

activenode_condition::activenode_condition_enum activenode_plugin::activity_loop()
{
   activenode_condition::activenode_condition_enum result;
   fc::limited_mutable_variant_object capture( GRAPHENE_MAX_NESTED_OBJECTS );
   try
   {
      chain::database& db = database();
      if (_activenode(db).is_enabled)
         result = maybe_send_activity(capture);
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
      case activenode_condition_enum::performed_activity:
         ilog("Sent activity #${n} with timestamp ${t} at time ${c}", (capture));
         break;
      case activenode_condition_enum::not_synced:
         ilog("Not sending activity because activity is disabled until we receive a recent block (see: --enable-stale-activity)");
         break;
      case activenode_condition_enum::not_my_turn:
         break;
      case activenode_condition_enum::not_time_yet:
         break;
      case activenode_condition_enum::no_private_key:
         ilog("Not sending activity because I don't have the private key for ${scheduled_key}", (capture) );
         break;
      case activenode_condition_enum::lag:
         elog("Not sending activity because node didn't wake up within 500ms of the slot time.");
         break;
      case activenode_condition_enum::exception_perform_activity:
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
   fc::time_point now_fine = fc::time_point::now();
   fc::time_point_sec now = now_fine + fc::microseconds( 500000 );
   ilog("!activenode_plugin::maybe_send_activity now_fine=${now_fine} now=${now}", ("now_fine", now_fine)("now", now));

   // If the next send activity opportunity is in the present or future, we're synced.
   if( !_activity_enabled )
   {
      // TODO: check if it is okay???!?!??!
      // need special slots for sending activity
      if( db.get_slot_time(1) >= now )
         _activity_enabled = true;
      else
         return activenode_condition::not_synced;
   }

   // is anyone scheduled to produce now or one second in the future?
   uint32_t slot = db.get_slot_at_time( now );
   ilog("!activenode_plugin::maybe_send_activitys slot=${slow} db.head_block_time()=${db_head_time}", ("slot", slot)("db_head_time", db.head_block_time()));

   if( slot == 0 )
   {
      capture("next_time", db.get_slot_time(1));
      return activenode_condition_enum::not_time_yet;
   }

   //
   // this assert should not fail, because now <= db.head_block_time()
   // should have resulted in slot == 0.
   //
   // if this assert triggers, there is a serious bug in get_slot_at_time()
   // which would result in allowing a later block to have a timestamp
   // less than or equal to the previous block
   //
   assert( now > db.head_block_time() );

   graphene::chain::activenode_id_type scheduled_activenode = db.get_scheduled_activenode( slot );
   // we must control the witness scheduled to produce the next block.
   if( scheduled_activenode == _activenode )
   {
      capture("scheduled_activenode", scheduled_activenode);
      return activenode_condition::not_my_turn;
   }

   fc::time_point_sec scheduled_time = db.get_slot_time( slot );
   graphene::chain::public_key_type scheduled_key = scheduled_activenode( db ).signing_key;

   if( _private_key.first != scheduled_key )
   {
      capture("scheduled_key", scheduled_key);
      return activenode_condition::no_private_key;
   }

   if( llabs((scheduled_time - now).count()) > fc::milliseconds( 500 ).count() )
   {
      capture("scheduled_time", scheduled_time)("now", now);
      return activenode_condition::lag;
   }



//    auto block = db.generate_block(
//       scheduled_time,
//       scheduled_witness,
//       private_key_itr->second,
//       _activity_skip_flags
//       );
//    capture("n", block.block_num())("t", block.timestamp)("c", now);
//    fc::async( [this,block](){ p2p_node().broadcast(net::block_message(block)); } );



   //todo here should send transaction, with spcial operation, 
   // with ip, ssl enabled status, timestamp, should also check if money is enough

   // account_object issuer_account = get_account( issuer );
   // FC_ASSERT(!find_asset(symbol).valid(), "Asset with that symbol already exists!");

   // if doesn't have 501 LLC should downgrade here?
   

   // asset_create_operation create_op;
   // create_op.issuer = issuer_account.id;
   // create_op.symbol = symbol;
   // create_op.precision = precision;
   // create_op.common_options = common;
   // create_op.bitasset_opts = bitasset_opts;

//    auto& account = _activenode(db).activenode_account;
//    const auto& stats = account.statistics(*this);

//    share_type total_activenode_balance = stats.total_core_in_orders.value
//       + (account.cashback_vb.valid() ? (*account.cashback_vb)(*this).balance.amount.value: 0) + get_balance(account.get_id(), asset_id_type()).amount.value;
         
//    bool enough_balance = (total_activenode_balance >= LLC_ACTIVENODE_MINIMAL_BALANCE);
//    if (!enough_balance) {

//    }
   fc:variant_object network_info = app().p2p_node()->network_get_info();
   //fc::ip::endpoint
   //fc::from_variant
   //ip::endpoint::from_string
   fc::ip::endpoint endpoint = ip::endpoint::from_string(network_info['listening_on'].as_string());
   //firewalled_state - uint8_t - unknown,firewalled, not firewalled
   graphene::net::firewalled_state node_firewalled_state;
   fc::from_variant<uint8_t, firewalled_state>(network_info['firewalled'], firewalled, 1);
   // network_info['firewalled']
   FC_ASSERT(endpoint.is_public() && node_firewalled_state == graphene::net::firewalled::not_firewalled);

   activenode_send_activity_operation send_activity_operation;
   // SHOULD??: send_act_op.timestamp
   fc::time_point now = fc::time_point::now();
   send_activity_operation.timepoint = now;
   send_activity_operation.endpoint = endpoint;
   send_activity_operation.activenode = _activenode;
   send_activity_operation.activenode_account = account;

   signed_transaction tx;
   tx.operations.push_back( create_op );
   set_operation_fees( tx, _remote_db->get_global_properties().parameters.current_fees);
   tx.validate();

   sign_transaction( tx, broadcast );

   return activenode_condition::performed_activity;
}
