<?php
const HOST = "http://194.63.142.61:8091/";
const CORE = "LLC";
$genesis =  json_decode(file_get_contents('genesis.json'), true);

$accounts = require("accounts.php");

foreach($genesis['initial_balances'] as $balance) {
  $privKey = '';
  foreach($accounts['all'] as $account)
    if($account['key_addr'] == $balance['owner'])
      $privKey = $account['wif_priv_key'];

  $login = $accounts['names'][$balance['owner']];

  echo "\n";
  echo 'import_key '.$login.' "'.$privKey.'" true';
  echo "\n";

  echo 'import_balance '.$login.' ["'.$privKey.'"] true true';
  echo "\n";
}    