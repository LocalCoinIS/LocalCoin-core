<?php
const HOST = "http://194.63.142.61:8091/";
const CORE = "LLC";
$genesis =  json_decode(file_get_contents('genesis.json'), true);


// <?php return [
//     "account"        => "localcoin",
//     "brain_priv_key" => "",
//     "wif_priv_key"   => "",
//     "pub_key"        => "",
//     "key_addr"       => ""
// ];
$localcoin = require("accounts.php");

foreach($accountForTransfer as $item) {
    if($item['login'] == $item['amount']) continue;

    $json = json_encode([
        "jsonrpc" => "2.0",
        "method"  => "transfer",
        "params"  => [            
            $localcoin['account'],
            $item['login'],
            intval(intval($item['amount']) / 100000),
            CORE,
            "initial amount",
            true
        ],
        "id" => 1
    ]);
    
    $ch = curl_init(HOST);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $json);
    curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: application/json')); 
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
    $result = curl_exec($ch);

    print_r($result);

    return;
}