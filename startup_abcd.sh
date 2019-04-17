rm -rf test_abcd/witness_node_alpha/blockchain test_abcd/witness_node_alpha/p2p
rm -rf test_abcd/witness_node_bravo/blockchain test_abcd/witness_node_bravo/p2p
rm -rf test_abcd/witness_node_charlie/blockchain test_abcd/witness_node_charlie/p2p
rm -rf test_abcd/witness_node_delta/blockchain test_abcd/witness_node_delta/p2p
rm -rf test_abcd/wallet.json *.wallet


./programs/witness_node/witness_node --data-dir=test_abcd/witness_node_alpha
cat startup_abcd.txt | ./programs/cli_wallet/cli_wallet -s"ws://127.0.0.1:8091" -w"test_abcd/wallet.json"

./programs/witness_node/witness_node --data-dir=test_abcd/witness_node_bravo
# cat startup_empty.txt | ./programs/cli_wallet/cli_wallet -s"ws://127.0.0.1:8092"

./programs/witness_node/witness_node --data-dir=test_abcd/witness_node_charlie
# cat startup_empty.txt | ./programs/cli_wallet/cli_wallet -s"ws://127.0.0.1:8093"

./programs/witness_node/witness_node --data-dir=test_abcd/witness_node_delta
# cat startup_empty.txt | ./programs/cli_wallet/cli_wallet -s"ws://127.0.0.1:8094"


cat startup_alpha.txt | ./programs/cli_wallet/cli_wallet -s"ws://127.0.0.1:8091" -w"test_abcd/wallet.json"
