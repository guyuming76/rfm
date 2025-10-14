from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer
import threading
import time

class FTPManager:
    def __init__(self):
        self.server = None
        self.thread = None
        self.running = False
        self._stop_event = threading.Event()  # 添加停止事件
        
    def start_server(self, host='0.0.0.0', port=2121, 
                    username='user', password='password', 
                    directory='/tmp'):
        # 如果服务器正在运行，先停止它
        if self.running:
            self.stop_server()
            # 等待一段时间确保服务器完全停止
            time.sleep(0.5)
        
        # 重置停止事件
        self._stop_event.clear()
        
        authorizer = DummyAuthorizer()
        authorizer.add_user(username, password, directory, perm='elradfmw')
        authorizer.add_anonymous(directory)
        
        handler = FTPHandler
        handler.authorizer = authorizer
        
        self.server = FTPServer((host, port), handler)
        
        def serve():
            try:
                self.running = True
                # 使用超时循环而不是 serve_forever
                while not self._stop_event.is_set():
                    self.server.serve_forever(timeout=1, blocking=False)
            except Exception as e:
                print(f"FTP服务器异常: {e}")
            finally:
                self.running = False
        
        self.thread = threading.Thread(target=serve,name="ftp_server")
        self.thread.daemon = True
        self.thread.start()
        
        # 等待服务器确认启动
        time.sleep(0.1)
        return True
    
    def stop_server(self):
        if self.server and self.running:
            try:
                # 设置停止事件
                self._stop_event.set()
                
                # 关闭所有客户端连接
                self.server.close_all()
                
                # 关闭服务器
                self.server.close()
                
                # 等待线程结束，但设置超时时间
                if self.thread and self.thread.is_alive():
                    self.thread.join(timeout=2.0)
                    if self.thread.is_alive():
                        print("警告: FTP服务器线程没有在超时时间内停止")
                
            except Exception as e:
                print(f"停止FTP服务器时出错: {e}")
            finally:
                self.running = False
                self.server = None
                self.thread = None
            return True
        return False

ftp_manager = FTPManager()
