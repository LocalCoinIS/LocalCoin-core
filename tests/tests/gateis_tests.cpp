// NOT WORKING, USE GATEIS_MANUAL_TESTS

#include <boost/test/unit_test.hpp>

#include <graphene/chain/database.hpp>
#include <graphene/chain/exceptions.hpp>

#include <graphene/chain/account_object.hpp>
#include <graphene/chain/asset_object.hpp>
#include <graphene/chain/committee_member_object.hpp>
#include <graphene/chain/witness_object.hpp>
#include <graphene/chain/proposal_object.hpp>
#include <graphene/chain/market_object.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"
#include <string>
#include <vector>
#include <set>
#include <boost/container/flat_set.hpp>


using namespace graphene::chain;
using namespace graphene::chain::test;

BOOST_AUTO_TEST_SUITE(gateis_tests)

const flat_set<vote_id_type> setup_votes() noexcept{
   auto num_wit = 21;
   auto start_id = 11;

   set<vote_id_type> votes;
   for (auto wit_id = start_id; wit_id < num_wit + start_id; wit_id++) {
         // auto& witness_id = *(active_witnesses.begin());
         auto vote_for = vote_id_type(vote_id_type::witness, wit_id);

         votes.insert(vote_for);
   }
   return flat_set<vote_id_type>(votes.begin(), votes.end());
}

account_create_operation make_account_gateis(
   const std::string& name /* = "nathan" */,
   public_key_type key /* = key_id_type() */,
   const graphene::chain::database &db
) {
try {
   account_create_operation create_account;
   create_account.registrar = account_id_type();

   create_account.name = name;
   create_account.owner = authority(123, key, 123);
   create_account.active = authority(321, key, 321);
   create_account.options.memo_key = key;
   create_account.options.voting_account = GRAPHENE_PROXY_TO_SELF_ACCOUNT;

   create_account.options.votes = setup_votes();//flat_set<vote_id_type>(votes.begin(), votes.end());
      
   create_account.options.num_witness = create_account.options.votes.size();
   create_account.fee = db.current_fee_schedule().calculate_fee( create_account );
   return create_account;
} FC_CAPTURE_AND_RETHROW() }

BOOST_FIXTURE_TEST_CASE( become_active_witness, database_fixture )
{
   try {
      generate_block();
      BOOST_CHECK_EQUAL(db.head_block_num(), 2);

      fc::time_point_sec maintenence_time = db.get_dynamic_global_properties().next_maintenance_time;
      BOOST_CHECK_GT(maintenence_time.sec_since_epoch(), db.head_block_time().sec_since_epoch());
      auto initial_properties = db.get_global_properties();

      //21
      std::vector<std::string> acc_names = {
         "inita",
         "initb",
         "initc",
         "initd",
         "inite",
         "initf",
         "initg",
         "inith",
         "initi",
         "initj",
         "initk",
         "initl",
         "initm",
         "initn",
         "inito",
         "initp",
         "initq",
         "initr",
         "inits",
         "initt",
         "initu"
      };

      std::set<uint64_t> poor_witnesses = {
         15, 16, 17
      }; 

      std::map<uint64_t, uint32_t> vote_for_witness = {
         {11,15}, {12,16}, {13,17}
      }; 

      std::vector<account_object> accounts;
      std::vector<witness_object> witnesses;

      const auto& acc_index = db.get_index_type<account_index>();
      const auto& account_objects = acc_index.indices();
      for (const account_object& acc: account_objects) {
         ilog("init account id:${id}", ("id", acc.id));
         if (acc.id.instance() > 0) {
            account_update_operation op;
            op.account = acc.id;
            op.new_options = acc.options;
            op.new_options->votes = setup_votes();
            op.new_options->num_witness = op.new_options->votes.size();
            trx.operations.push_back(op);
            PUSH_TX( db, trx, ~0 );
            trx.operations.clear();
         }
      }

      const auto& wit_index = db.get_index_type<witness_index>();
      const auto& wit_objects = wit_index.indices();
      for (const witness_object& wit: wit_objects) {
         ilog("init witness id:${id}, acc id:${acc_id}", ("id", wit.id)("acc_id", wit.witness_account));
      }
      for (auto name : acc_names) {
         fc::ecc::private_key private_key = generate_private_key(name);
         public_key_type public_key = private_key.get_public_key();
         trx.operations.push_back(make_account_gateis(name, public_key, db));
         trx.validate();
         processed_transaction ptx = db.push_transaction(trx, ~0);
         auto& account = db.get<account_object>(ptx.operation_results[0].get<object_id_type>());
         trx.operations.clear();


         // fund(account, asset(50000));
         upgrade_to_lifetime_member(account);
         const witness_object& wit = create_witness(account, private_key);
         witnesses.push_back(wit);
         accounts.push_back(account);
         ilog("witness_id - ${wit_id}, account_id - ${acc_id}", ("wit_id", wit.id)("acc_id", account.id));

         ilog("checking instance - ${witidinst}", ("witidinst", wit.id.instance()));
         auto it = poor_witnesses.find(wit.id.instance());
         if(it != poor_witnesses.end()) { // found
            ilog("^poor");
         }
         else {
            auto vote = vote_for_witness.find(wit.id.instance());
            vote_id_type vote_for;
            if(vote != vote_for_witness.end()) {
               // vote_for = vote_id_type(vote->second);
               vote_for = vote_id_type(vote_id_type::witness, (uint32_t)vote->second);
            }
            else {
               vote_for = wit.vote_id;
            }
            fund(account, asset(500 * 1000 * 1000 * int64_t(10000ll)));
            account_update_operation op;
            op.account = accounts[0].id;
            op.new_options = accounts[0].options;
            op.new_options->votes = setup_votes();
            op.new_options->num_witness = op.new_options->votes.size();
            trx.operations.push_back(op);
            PUSH_TX( db, trx, ~0 );
            trx.operations.clear();
         }
      }

      for (const account_object& acc: account_objects)
      {
         const account_balance_index& balance_index = db.get_index_type<account_balance_index>();
         auto range = balance_index.indices().get<by_account_asset>().equal_range(boost::make_tuple(acc.id));
         share_type total_balance = 0;
         for (const account_balance_object& balance : boost::make_iterator_range(range.first, range.second))
            if (balance.asset_type == asset_id_type(0))  // is CORE asset
               total_balance += balance.balance;
         ilog("account #${acc_id} balance - ${balance}", ("acc_id", acc.id)("balance", total_balance));
      }

      generate_blocks(maintenence_time - initial_properties.parameters.block_interval);
      BOOST_CHECK_EQUAL(db.get_global_properties().parameters.maximum_transaction_size,
                        initial_properties.parameters.maximum_transaction_size);
      BOOST_CHECK_EQUAL(db.get_dynamic_global_properties().next_maintenance_time.sec_since_epoch(),
                        db.head_block_time().sec_since_epoch() + db.get_global_properties().parameters.block_interval);
      BOOST_CHECK(db.get_global_properties().active_witnesses == initial_properties.active_witnesses);

      generate_block();

      auto new_properties = db.get_global_properties();
      BOOST_CHECK(new_properties.active_witnesses != initial_properties.active_witnesses);
      for (auto act_wit_it = new_properties.active_witnesses.begin(); act_wit_it != new_properties.active_witnesses.end(); ++act_wit_it) {
         ilog("active witness - ${id}", ("id", *act_wit_it));
      }
      for (auto poor_wit_it = poor_witnesses.begin(); poor_wit_it != poor_witnesses.end(); ++poor_wit_it) {      
         BOOST_CHECK(std::find(new_properties.active_witnesses.begin(),
                              new_properties.active_witnesses.end(), witness_id_type(*poor_wit_it)) ==
                     new_properties.active_witnesses.end());
      }
      BOOST_CHECK_EQUAL(db.get_dynamic_global_properties().next_maintenance_time.sec_since_epoch(),
                        maintenence_time.sec_since_epoch() + new_properties.parameters.maintenance_interval);
      maintenence_time = db.get_dynamic_global_properties().next_maintenance_time;
      BOOST_CHECK_GT(maintenence_time.sec_since_epoch(), db.head_block_time().sec_since_epoch());
      db.close();
   } catch (fc::exception& e) {
      edump((e.to_detail_string()));
      throw;
   }
}

BOOST_AUTO_TEST_SUITE_END()