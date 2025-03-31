import jwt
import requests
import time

def generate_jwt_token(private_key, client_id):
    payload = {
        'iat': int(time.time()),
        'exp': int(time.time()) + 600, 
        'iss': client_id
    }

    encoded_jwt = jwt.encode(payload, private_key, algorithm='RS256')
    return encoded_jwt

def generate_access_token(jwt_token, app_id):
    url = f"https://api.github.com/app/installations/{app_id}/access_tokens"
    headers = {
        "Accept": "application/vnd.github+json",
        "Authorization": f"Bearer {jwt_token}",
        "X-GitHub-Api-Version": "2022-11-28"
    }
    try:
        # 发送 POST 请求
        response = requests.post(url, headers=headers)

        # 检查响应状态码
        if response.status_code == 201:
            return response.json().get('token')
        else:
            raise Exception(f"请求失败，状态码：{response.status_code}，响应内容：{response.text}")
    except requests.RequestException as e:
        print(f"请求发生错误：{e}")
        raise e

def add_reviewers(access_token, repo_name, pull_request_number, reviewers: list):
    url = f"https://api.github.com/repos/{repo_name}/pulls/{pull_request_number}/requested_reviewers"
    headers = {
        "Accept": "application/vnd.github+json",
        "Authorization": f"Bearer {access_token}",
        "X-GitHub-Api-Version": "2022-11-28"
    }
    data = {
        "reviewers": reviewers
    }
    try:
        # 发送 POST 请求
        response = requests.post(url, headers=headers, json=data)
        # 检查响应状态码
        if response.status_code == 201:
            return response.json()
        else:
            raise Exception(f"请求失败，状态码：{response.status_code}，响应内容：{response.text}")
    except requests.RequestException as e:
        print(f"请求发生错误：{e}")
        raise e