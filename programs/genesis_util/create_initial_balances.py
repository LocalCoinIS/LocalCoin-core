# import json
# genesisfilepath = '../../genesis.json'
# outfilepath = 'initial_balances.json'
# default_amount = 1000000000000000
# issuer_owner_map = {
#     'localcoin-fiat' : 'LLC7tKsrEe2hfsPTs88ecG7EgH9ivHp3a4ay',
#     'localcoin-wallet' : 'LLC5upDqvcRoFVS4RZvtjy6WEmSUr1BejB1H'
# }
# infile = open(genesisfilepath)
# genesisdict = json.load(infile)
# balances = []
# try:
#     assets = genesisdict['initial_assets']
#     for asset in assets:
#         issuer = asset['issuer_name']
#         symbol = asset['symbol']
#         balances.append({
#             "owner": issuer_owner_map[issuer],
#             "asset_symbol": symbol,
#             "amount": str(default_amount)
#         })
# except KeyError as ke:
#     pass
# with open(outfilepath, 'w') as outfile:
#     json.dump(balances, outfile)