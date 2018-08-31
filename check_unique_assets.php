<?php
$json = file_get_contents('genesis.json');
$data = json_decode($json, true);

echo "count all assets: "   . count($data['initial_assets']) . "\n";

$duplicates = [];
$groupAssets = [];
foreach($data['initial_assets'] as $i)
    if(isset($groupAssets[$i['symbol']]))
        $duplicates[] = $i['symbol'];
    else $groupAssets[$i['symbol']] = $i;

echo "count unique assets: "   . count($groupAssets) . "\n";

if($duplicates)
    echo "duplicates: "   . implode(',', $duplicates) . "\n";