<?php
$curl = curl_init();
const RPC = 'http://194.63.142.61:8091/rpc';

$json = file_get_contents('initial_data.json');
$data = json_decode($json, true);

echo "count initial_assets: " . count($data['initial_assets']) . "\n";
echo "count initial_balances: " . count($data['initial_balances']) . "\n";

//выпустим ассеты
foreach($data['initial_balances'] as $i) {
    $query =  '{"jsonrpc": "2.0", "method": "issue_asset", "params": [' .
                '"'.$i['owner']        .'", '.
                '"'.$i['amount']       .'", '.
                '"'.$i['asset_symbol'] .'", '.
                '"", true], "id": 1}';

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
