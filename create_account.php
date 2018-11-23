<?php
const HOST              = "http://194.63.142.61:8091/";
const ACCOUNT_BASE_NAME = "testassets";
const LOCALCOIN_ACCOUNT = "localcoin-wallet";
const REFFER_ACCOUNT    = "localcoin-wallet";
const BRAIN_KEY         = "FILINGS THEREOF ENSILE JAW OVERBID RETINAL PILULAR RYPE CHITTY RAFFERY HANDGUN ERANIST UNPILE TWISTER BABYDOM CIBOL";
const COUNT             = 2000/50;

for($i = 1; $i <= COUNT; $i++) {
    $account = ACCOUNT_BASE_NAME . $i;
    $json = json_encode([
        "jsonrpc" => "2.0",
        "method"  => "create_account_with_brain_key",
        "params"  => [
            BRAIN_KEY,
            $account,
            LOCALCOIN_ACCOUNT,
            REFFER_ACCOUNT,
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

    echo $account . "\n";
}