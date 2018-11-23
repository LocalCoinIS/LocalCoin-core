<?php
const HOST = "http://194.63.142.61:8091/";
const ACCOUNT_BASE_NAME = "testassets";
const AMOUNT = 10;
const PART_SIZE = 50;

$list = [];
$assets = [];
$genesis =  json_decode(file_get_contents('genesis.json'), true);
foreach($genesis["initial_assets"] as $id => $item) $assets[] = $item["symbol"];
$countAccounts = ceil(count($assets) / PART_SIZE);

for($accountId = 1; $accountId <= $countAccounts; $accountId++) {
    $account = ACCOUNT_BASE_NAME . $accountId;    
    $list[$account] = [];

    for($i=(($accountId-1) * PART_SIZE); $i <= $accountId * PART_SIZE; $i++) {
        if(!empty($assets[$i])) {
            $list[$account][] = $assets[$i];
        }
    }
}

foreach($list as $account => $assetsForAccounts) {
    foreach($assetsForAccounts as $asset) {
        $json = json_encode([
            "jsonrpc" => "2.0",
            "method"  => "issue_asset",
            "params"  => [
                $account,
                AMOUNT,
                $asset,
                "init ".AMOUNT." coins",
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
    
        echo $account . " - " . $asset . "\n";
    }
}