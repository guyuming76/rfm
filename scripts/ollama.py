#!/usr/bin/python

import requests
import json

def stream_ollama_response(prompt, model_name="deepseek-r1:1.5b", base_url="http://localhost:11434"):
    """
    Ollama 流式调用核心函数
    :param prompt: 用户输入的提示词
    :param model_name: 已部署的模型名称
    :param base_url: Ollama 服务地址
    :return: 生成器，逐块返回响应内容
    """
    endpoint = f"{base_url}/api/generate"
    payload = {
        "model": model_name,
        "prompt": prompt,
        "stream": True,  # 启用流式输出
        "options": {
            "temperature": 0.7,   # 控制随机性 (0.1~1.0)
            #"max_tokens": 512,    # 最大生成token数
            #"top_p": 0.9          # 采样阈值
        }
    }

    try:
        # 发送流式请求
        response = requests.post(
            endpoint,
            json=payload,
            stream=True,  # 关键参数：保持连接打开
            timeout=30    # 超时时间（秒）
        )
        response.raise_for_status()  # 自动触发HTTP错误异常

        # 逐块处理响应内容
        for chunk in response.iter_lines():
            if chunk:
                # 解析JSON数据
                chunk_data = json.loads(chunk.decode('utf-8'))
                
                # 提取有效内容（兼容不同模型响应格式）
                content = chunk_data.get("response", "")
                
                # 返回生成内容（生成器模式）
                yield content

    except requests.exceptions.ConnectionError:
        print(f"\n连接失败，请检查 Ollama 服务是否运行在 {base_url}")
    except requests.exceptions.Timeout:
        print("\n请求超时，请尝试增大 timeout 参数或检查模型响应速度")
    except json.JSONDecodeError:
        print("\n响应解析失败，请检查模型是否正常输出 JSON 格式")

# 使用示例
if __name__ == "__main__":
    user_input = "量子计算如何改变人工智能？"
    
    print(f"用户提问：{user_input}\n回答生成：")
    
    full_response = []
    try:
        # 逐块获取并打印响应
        for chunk in stream_ollama_response(user_input):
            print(chunk, end="", flush=True)
            full_response.append(chunk)
        
        # 最终完整响应
        print(f"\n\n完整响应：\n{''.join(full_response)}")
    
    except KeyboardInterrupt:
        print("\n用户中断生成")
