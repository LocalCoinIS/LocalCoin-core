<?php
$curl = curl_init();

const RPC    = 'http://194.63.142.61:8091/rpc';
const DELTA  = 100000;
const LLC_ID = '1.3.0';

$json = file_get_contents('initial_data.json');
$data = json_decode($json, true);

echo "count initial_assets: "   . count($data['initial_assets'])   . "\n";
echo "count initial_balances: " . count($data['initial_balances']) . "\n";

//выпустим ассеты (не работает)
// foreach($data['initial_balances'] as $i) {
//     $query =  '{"jsonrpc": "2.0", "method": "issue_asset", "params": ["' . implode('", "', [
//                     $i['owner'], amount($i['amount']), LLC_ID
//                 ]) . '", "", true], "id": 1}';

//     $request = send_json(
//         $curl, RPC, [
//             [
//                 CURLOPT_HTTPHEADER,
//                 ['Content-Type: application/json', 'Content-Length: ' .
//                     strlen($query)
//                 ]
//             ]
//         ], $query
//     );

//     echo $request . "\n" . "\n" . "\n";
// }

//создадим ассеты
$permissions = [
    "charge_market_fee"    => true,
    "white_list"           => true,
    "override_authority"   => true,
    "transfer_restricted"  => true,
    "disable_force_settle" => true,
    "global_settle"        => true,
    "disable_confidential" => true,
    "witness_fed_asset"    => true,
    "committee_fed_asset"  => true,
];

$flags = [
    // "charge_market_fee"        => true,
    // "white_list"               => true,
    // "override_authority"       => true,
    // "transfer_restricted"      => true,
    // "disable_force_settle"     => true,
    // //"global_settle"            => False,
    // "disable_confidential"     => true,
    // //"witness_fed_asset"        => False,
    // "committee_fed_asset"      => true,
];

foreach($data['initial_assets'] as $i) {
    $options = '{"max_supply" : 1000000000,
        "market_fee_percent" : 0,
        "max_market_fee" : 0,
        "issuer_permissions" : ' . 79 . ',
        "flags" : '.count($flags).',
        "core_exchange_rate" : {
        "base": {
            "amount": 0.0000001,
            "asset_id": "1.3.0"
        },
        "quote": {
            "amount": 0.0000001,
            "asset_id": "1.3.1"}
        },
        "whitelist_authorities" : [],
        "blacklist_authorities" : [],
        "whitelist_markets" : [],
        "blacklist_markets" : [],
        "description" : "My fancy description"
        }';

    $params =  '"localcoin-wallet", "QQQ", 3, ' . $options . ', {}, true';
    $query =  '{"jsonrpc": "2.0", "method": "create_asset", "params": ['.$params.'], "id": 1}';

    $request = send_json(
        $curl, RPC, [
            [
                CURLOPT_HTTPHEADER,
                ['Content-Type: application/json', 'Content-Length: ' .
                    strlen($query)
                ]
            ]
        ], $query
    );

    echo $request . "\n" . "\n" . "\n";
    return;
}

function amount($val) {
    return floatval($val) / floatval(DELTA);
}

function send_json($curl, $url, $hearder = array(), $post = array(), $postType = true) {
    curl_setopt($curl, CURLOPT_URL, $url);
    curl_setopt($curl, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($curl, CURLOPT_POST, $postType);
    foreach ($hearder as $head) curl_setopt($curl, $head[0], $head[1]);
    if (!empty($post)) curl_setopt($curl, CURLOPT_POSTFIELDS, $post);
    $out = curl_exec($curl);
    return $out;
}
