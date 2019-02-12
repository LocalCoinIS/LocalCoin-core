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

#include <graphene/app/plugin.hpp>
#include <graphene/chain/database.hpp>

#include <fc/thread/future.hpp>
#include <fc/optional.hpp>

namespace graphene { namespace activenode_plugin {
using namespace fc;
using namespace chain;
namespace activenode_condition
{
   enum activenode_condition_enum
   {
      performed_activity = 0,
      not_synced = 1,
      not_my_turn = 2,
      not_time_yet = 3,
      no_private_key = 4,
      lag = 5,
      no_scheduled_activenodes = 6,
      deleted = 7,
      not_sending = 8,
      exception_perform_activity = 9
   };
}

class activenode_plugin : public graphene::app::plugin {
public:
   ~activenode_plugin() {
      try {
         if( _activity_task.valid() )
            _activity_task.cancel_and_wait(__FUNCTION__);
      } catch(fc::canceled_exception&) {
         //Expected exception. Move along.
      } catch(fc::exception& e) {
         edump((e.to_detail_string()));
      }
   }

   std::string plugin_name()const override;

   virtual void plugin_set_program_options(
      boost::program_options::options_description &command_line_options,
      boost::program_options::options_description &config_file_options
      ) override;

   void set_activenode_plugin_enabled(bool allow) { _activenode_plugin_enabled = allow; }

   virtual void plugin_initialize( const boost::program_options::variables_map& options ) override;
   virtual void plugin_startup() override;
   virtual void plugin_shutdown() override;

private:
   void schedule_activity_loop();
   void on_new_block_applied(const signed_block& new_block);

   activenode_condition::activenode_condition_enum activity_loop();
   activenode_condition::activenode_condition_enum maybe_send_activity( fc::limited_mutable_variant_object& capture );

   activenode_condition::activenode_condition_enum send_activity( fc::limited_mutable_variant_object& capture );

   boost::program_options::variables_map _options;
   bool _activenode_plugin_enabled = true;

   std::pair<chain::public_key_type, fc::ecc::private_key> _private_key;
   optional<chain::activenode_id_type> _activenode = optional<chain::activenode_id_type>();
   std::string _activenode_account_name;
   chain::account_id_type _activenode_account_id;
   fc::future<void> _activity_task;
};

} } //graphene::activenode_plugin
