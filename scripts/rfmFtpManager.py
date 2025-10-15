from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer
import threading
import time
import os

class FTPManager:
    def __init__(self):
        self.server = None
        self.thread = None
        self.running = False
        self._stop_event = threading.Event() # 添加停止事件
        self.current_config = {  # 保存当前配置
            'host': '0.0.0.0',
            'port': 2121,
            'username': 'user',
            'password': 'password',
            'directory': '/tmp'
        }
    
    def start_server(self, host='0.0.0.0', port=2121, 
                    username='user', password='password', 
                    directory='/tmp'):
        # 如果服务器正在运行，先停止它
        if self.running:
            self.stop_server()
        # 等待一段时间确保服务器完全停止
        time.sleep(0.5)
        
        # 保存当前配置
        self.current_config = {
            'host': host,
            'port': port,
            'username': username,
            'password': password,
            'directory': directory
        }
        
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
    
    def change_root_directory(self, new_directory):
        """
        更改FTP服务的根目录
        
        参数:
            new_directory (str): 新的根目录完整路径
        """
        # 检查目录是否存在
        if not os.path.exists(new_directory):
            print(f"错误: 目录 '{new_directory}' 不存在")
            return False
        
        # 检查是否有读取权限
        if not os.access(new_directory, os.R_OK):
            print(f"错误: 对目录 '{new_directory}' 没有读取权限")
            return False
        
        print(f"正在更改FTP根目录为: {new_directory}")
        
        # 如果服务器正在运行，使用当前配置重新启动服务器
        was_running = self.running
        if was_running:
            self.stop_server()
            time.sleep(0.5)
        
        # 更新配置中的目录
        self.current_config['directory'] = new_directory
        
        # 如果之前服务器在运行，重新启动
        if was_running:
            success = self.start_server(
                host=self.current_config['host'],
                port=self.current_config['port'],
                username=self.current_config['username'],
                password=self.current_config['password'],
                directory=self.current_config['directory']
            )
            if success:
                print(f"FTP根目录已成功更改为: {new_directory}")
            return success
        else:
            print(f"FTP根目录配置已更新为: {new_directory} (服务器未运行)")
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
