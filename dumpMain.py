import subprocess
import threading
import os
from contextlib import suppress

# 配置参数
TIMEOUT = 10  # 单位：秒
OUTPUT_FILE = "outfileDEX"

def start_mitmdump():
    # 每次运行前清空抓包文件
    open(OUTPUT_FILE, "w").close()
    print(f"[*] 已清空抓包文件: {OUTPUT_FILE}")

    cmd = [
        "C:\Program Files\Python313\Scripts\mitmdump",
        "-w", OUTPUT_FILE,
        "--mode", "transparent",
        "--ssl-insecure",
        "--set", "upstream_cert=false",
        "--flow-detail", "3"
    ]

  # 隐藏 CMD 窗口（仅适用于 Windows）
    startupinfo = None
    if os.name == 'nt':  # Windows
        startupinfo = subprocess.STARTUPINFO()
        startupinfo.dwFlags |= subprocess.STARTF_USESHOWWINDOW
        startupinfo.wShowWindow = subprocess.SW_HIDE  # 隐藏窗口

    process = subprocess.Popen(
        cmd,
        startupinfo=startupinfo  # 关键：传入 startupinfo
    )
    print("[*] 正在启动 mitmdump...")

    def timeout_handler():
        print(f"\n[!] 已运行 {TIMEOUT} 秒，准备优雅终止 mitmdump...")
        try:
            # 发送 Ctrl+C 信号（模拟用户按下 Ctrl+C）
            import signal
            process.send_signal(signal.SIGINT)  # Windows/Linux 都支持
        except Exception as e:
            #print(f"[!] 无法发送 SIGINT：{e}")
            process.terminate()
        process.wait()

    timer = threading.Timer(TIMEOUT, timeout_handler)
    timer.start()

    try:
        process.wait()
    except KeyboardInterrupt:
        print("\n[!] 用户中断，正在终止 mitmdump...")
        with suppress(Exception):
            process.terminate()
            process.wait()
    finally:
        timer.cancel()

    print("[*] mitmdump 已关闭")

def encode():
    import sys
    from mitmproxy.io import FlowReader
    import json
    output_file = open("outfileDecode", "w", encoding="utf-8")
    # output_file = open("outfileDecode123", "w", encoding="utf-8")
    sys.stdout = output_file

    filename = "outfileDEX"  

    # filename = "C:\\Users\\Administrator\\flowTXT"
    with open(filename, "rb") as fp:

        flows_json = []
        reader = FlowReader(fp)
        for flow in reader.stream():
            try:
                flow_data = {}

                # 请求部分
                req = {
                    "url": flow.request.url,
                    "method": flow.request.method,
                    "headers": dict(flow.request.headers),
                }

                if flow.request.content:
                    try:
                        req["content"] = flow.request.content.decode("utf-8", errors="ignore")
                    except:
                        req["content"] = "<无法解码>"

                flow_data["request"] = req

                # 响应部分（可能不存在）
                if flow.response:
                    resp = {
                        "status_code": flow.response.status_code,
                        "headers": dict(flow.response.headers),
                    }

                    if flow.response.content:
                        try:
                            resp["content"] = flow.response.content.decode("utf-8", errors="ignore")
                        except:
                            resp["content"] = "<无法解码>"

                    flow_data["response"] = resp
                else:
                    flow_data["response"] = None

                flows_json.append(flow_data)
                
            except Exception as e:
                #print(f"[!] 跳过一个 flow，原因：{e}")
                continue

    json_output = json.dumps(flows_json, ensure_ascii=False, indent=2)
    print(json_output)    
# 关闭文件
    output_file.close()


if __name__ == "__main__":
    start_mitmdump()
    encode()
