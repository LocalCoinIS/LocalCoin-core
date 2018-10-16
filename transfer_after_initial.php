<?php
const HOST = "http://194.63.142.61:8091/";
$genesis =  json_decode(file_get_contents('genesis.json'), true);

$localcoin = require("localcoin-data.php");

foreach($genesis["transfer_after_initial"] as $id => $item) {
    $json = json_encode([
        "jsonrpc" => "2.0",
        "method"  => "reserve_asset",
        "params"  => [            
            $item["symbol"],            
        ],
        "id" => 1
    ]);
    $ch = curl_init(HOST);
    curl_setopt($ch, CURLOPT_POST, 1);
    curl_setopt($ch, CURLOPT_POSTFIELDS, $json);
    curl_setopt($ch, CURLOPT_HTTPHEADER, array('Content-Type: application/json')); 
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
    $result = curl_exec($ch);

    echo count($genesis["initial_assets"]) - $id - 1;
    echo "\n";
}