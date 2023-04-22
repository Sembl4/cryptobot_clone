# import websockets
import hashlib, hmac
import datetime
import requests
import base64
import json
import urllib.parse
from typing import Any, Dict, List, Optional, Union

def get_timestamp() -> str:
	return (
		datetime.datetime.utcnow().isoformat(timespec="milliseconds") + "Z"
	)  

def prehash_str(
	timestamp: str,
	method: str,
	request_path: str,
	body: Optional[Union[Dict[str, Any], List[dict]]] = None,
) -> str:
	if body:
		if method == "GET":
			body_str = "?"
			body_str += urllib.parse.urlencode(body)  # type: ignore
		elif method == "POST":
			body_str = json.dumps(body)
	else:
		body_str = ""
	return f"{timestamp}{method}{request_path}{body_str}"

def hashing(secret: str,prehash: str) -> str:
	hm = hmac.new(
			bytes(secret, "ascii"),
			msg=bytes(prehash, "ascii"),
			digestmod=hashlib.sha256,
	)
	return base64.b64encode(hm.digest()).decode("ascii")

api_url = 'https://aws.okx.com'
withdrawal_url = '/api/v5/asset/withdrawal'

api_key = 'cfc752d5-9259-4e6d-861c-3ce413175761'
secret = '80B9907A875920045093E94544809712'
pass_phrase = 'Withdrawals_enabled1'

address = 'TDE5YWLmHH7HNe2V5Sc8LLS1APMV489oTd' #TRC20
req_body = {
    'amt':'5',
    'fee':'1',
    'dest':'4',
    'ccy':'USDT',
    'chain':'USDT-TRC20',
    'toAddr':address
}
ts = get_timestamp()
req_headers = {
    'OK-ACCESS-KEY': api_key,
    'OK-ACCESS-TIMESTAMP' : ts,
    'OK-ACCESS-PASSPHRASE' : pass_phrase,
    'OK-ACCESS-SIGN' : hashing(secret,prehash_str(ts,"POST",withdrawal_url,req_body))
}

ans = requests.post(url=str(api_url + withdrawal_url),headers=req_headers,json=req_body)
