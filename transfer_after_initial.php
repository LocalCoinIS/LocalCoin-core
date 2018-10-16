<?php
const HOST = "http://194.63.142.61:8091/";
$genesis =  json_decode(file_get_contents('genesis.json'), true);

$returnValue = json_decode('[
    {
      "login": "ratamahatta",
      "amount": 10000000000
    },
    {
      "login": "adamgottie",
      "amount": 10000000000
    },
    {
      "login": "winstonsmith",
      "amount": 10000000000
    },
    {
      "login": "sentriusfounders",
      "amount": 9000000000
    },
    {
      "login": "julia",
      "amount": 10000000000
    },
    {
      "login": "obrien",
      "amount": 10000000000
    },
    {
      "login": "aaronson",
      "amount": 10000000000
    },
    {
      "login": "ampleforth",
      "amount": 10000000000
    },
    {
      "login": "charrington",
      "amount": 10000000000
    },
    {
      "login": "katharine_smith",
      "amount": 10000000000
    },
    {
      "login": "tom_parsons",
      "amount": 10000000000
    },
    {
      "login": "syme",
      "amount": 10000000000
    },
    {
      "login": "emmanuelgoldstein",
      "amount": 10000000000
    },
    {
      "login": "ingsoc",
      "amount": 10000000000
    },
    {
      "login": "newspeak",
      "amount": 10000000000
    },
    {
      "login": "oceania",
      "amount": 10000000000
    },
    {
      "login": "mrbig",
      "amount": 10000000000
    },
    {
      "login": "ministries",
      "amount": 10000000000
    },
    {
      "login": "atlant",
      "amount": 10000000000
    },
    {
      "login": "felix",
      "amount": 10000000000
    },
    {
      "login": "spikemilligan",
      "amount": 10000000000
    },
    {
      "login": "ericsykes",
      "amount": 10000000000
    },
    {
      "login": "davidsilveria",
      "amount": 10000000000
    },
    {
      "login": "integral",
      "amount": 10000000000
    },
    {
      "login": "d503",
      "amount": 10000000000
    },
    {
      "login": "i303",
      "amount": 10000000000
    },
    {
      "login": "murphy",
      "amount": 10000000000
    },
    {
      "login": "millie",
      "amount": 10000000000
    },
    {
      "login": "stoneman",
      "amount": 10000000000
    },
    {
      "login": "faber",
      "amount": 10000000000
    },
    {
      "login": "granger",
      "amount": 10000000000
    },
    {
      "login": "montag",
      "amount": 10000000000
    },
    {
      "login": "sirius",
      "amount": 10000000000
    },
    {
      "login": "hades",
      "amount": 10000000000
    },
    {
      "login": "aphaea",
      "amount": 10000000000
    },
    {
      "login": "germut",
      "amount": 80000000000000
    },
    {
      "login": "notteler",
      "amount": 20000000000000
    },
    {
      "login": "localcoin-ico",
      "amount": 10000000000000
    },
    {
      "login": "localcoin-airdrop",
      "amount": 1000000000000
    },
    {
      "login": "localcoin-bounty",
      "amount": 1000000000000
    },
    {
      "login": "localcoin",
      "amount": 1000000000000
    }
]');
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