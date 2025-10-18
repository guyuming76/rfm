from threading import Thread
import time
import os
import platform
import subprocess
import socketserver
from abc import ABC, abstractmethod

class BaseServiceManager(ABC):
    """基础服务管理器，包含通用功能"""
    
    def __init__(self, service_type):
        self.service_type = service_type
        self.server = None
        self.thread = None
        self.running = False
        self._changing_directory = False
        self.current_config = {
            'host': '0.0.0.0',
            'port': -1,
            'username': None,
            'password': None,
            'directory': '.',
            'firewall': False
        }
    
    def _execute_firewall_command(self, action, port):
        """
        执行防火墙命令来打开或关闭端口
        
        参数:
        action (str): 'open' 或 'close'
        port (int): 端口号
        """
        try:
            system = platform.system().lower()
            
            if system == 'windows':
                # Windows 使用 netsh
                if action == 'open':
                    cmd = f'netsh advfirewall firewall add rule name=\"{self.service_type.upper()} Server Port {port}\" dir=in action=allow protocol=TCP localport={port}'
                else: # close
                    cmd = f'netsh advfirewall firewall delete rule name=\"{self.service_type.upper()} Server Port {port}\" protocol=TCP localport={port}'
                
                result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
                
            elif system in ['linux', 'darwin']: # Linux 或 macOS
                # Linux 使用 iptables 或 ufw, macOS 使用 pfctl
                if system == 'linux':
                    # 尝试使用 ufw (Ubuntu)
                    try:
                        if action == 'open':
                            cmd = f'sudo ufw allow {port}'
                        else: # close
                            cmd = f'sudo ufw delete allow {port}'
                        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
                    except:
                        # 回退到 iptables
                        if action == 'open':
                            cmd = f'iptables -A INPUT -p tcp --dport {port} -j ACCEPT'
                        else: # close
                            cmd = f'iptables -D INPUT -p tcp --dport {port} -j ACCEPT'
                        result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
                    try:
                        if action == 'open':
                            cmd1 = f'ifconfig'
                            subprocess.run(cmd1, shell=True, text=True)
                    finally:
                        print("")

                elif system == 'darwin': # macOS
                    # macOS 使用 pfctl，需要更复杂的配置
                    print(f"macOS防火墙配置需要手动设置，请使用: sudo pfctl 来管理端口 {port}")
                    return True
                
            else:
                print(f"不支持的平台: {system}")
                return False
            
            if result.returncode == 0:
                print(f"成功{action}防火墙端口 {port}")
                return True
            else:
                print(f"防火墙命令执行失败: {result.stderr}")
                return False
            
        except Exception as e:
            print(f"执行防火墙命令时出错: {e}")
            return False
    
    def _open_firewall_port(self, port):
        """打开防火墙端口"""
        return self._execute_firewall_command('open', port)
    
    def _close_firewall_port(self, port):
        """关闭防火墙端口"""
        return self._execute_firewall_command('close', port)
    
    def _validate_directory(self, directory):
        """验证目录是否存在且可读"""
        if not os.path.exists(directory):
            print(f"错误: 目录 '{directory}' 不存在")
            return False
        
        if not os.access(directory, os.R_OK):
            print(f"错误: 对目录 '{directory}' 没有读取权限")
            return False
        
        return True
    
    def _serve_forever(self):
        """通用的服务器运行循环"""
        try:
            self.running = True
            self._serve_forever_specific()
        except Exception as e:
            print(f"{self.service_type.upper()}服务器异常: {e}")
        finally:
            self.running = False
    
    @abstractmethod
    def _serve_forever_specific(self):
        """子类实现的特定服务器运行逻辑"""
        pass
    
    @abstractmethod
    def _create_server(self, host, port, directory, username=None, password=None, **kwargs):
        """子类实现的创建服务器实例逻辑"""
        pass
    
    @abstractmethod
    def _stop_server_specific(self):
        """子类实现的特定服务器停止逻辑"""
        pass
    
    def start_server(self, host='0.0.0.0', port=None, 
                    username=None, password=None,
                    directory='.', firewall=False, **kwargs):
        """
        启动服务的通用方法
        
        参数:
        host (str): 监听地址
        port (int): 监听端口
        username (str): 用户名（FTP使用）
        password (str): 密码（FTP使用）
        directory (str): 根目录
        firewall (bool): 是否配置防火墙
        **kwargs: 其他特定参数
        """
        # 设置默认端口
        if port is None or port<0:
            port = 2121 if self.service_type == 'ftp' else 8080
        
        # 验证目录
        if not self._validate_directory(directory):
            return False
        
        # 如果不是在更改目录过程中，打开防火墙端口
        if not self._changing_directory and firewall:
            self._open_firewall_port(port)

        # 如果服务器正在运行，先停止它
        if self.running:
            self.stop_server()
            time.sleep(0.5)
        
        # 保存当前配置
        self.current_config = {
            'host': host,
            'port': port,
            'username': username,
            'password': password,
            'directory': directory,
            'firewall': firewall,
            'kwargs': kwargs.copy() # 保存kwargs的副本
        }
        
        # 创建服务器实例
        self.server = self._create_server(host, port, directory, username, password, **kwargs)
        if not self.server:
            return False
        
        # 启动服务线程
        self.thread = Thread(target=self._serve_forever, name=f"{self.service_type}_server")
        self.thread.daemon = True
        self.thread.start()
        
        # 等待服务器确认启动
        time.sleep(0.1)
        print(f"{self.service_type.upper()}服务器已启动在 {host}:{port}, 根目录: {directory}")
        return True
    
    def stop_server(self):
        """停止服务的通用方法"""
        if self.server and self.running:
            try:
                # 调用子类的特定停止逻辑
                self._stop_server_specific()
                
                # 等待线程结束，但设置超时时间
                if self.thread and self.thread.is_alive():
                    self.thread.join(timeout=2.0)
                    if self.thread.is_alive():
                        print(f"警告: {self.service_type.upper()}服务器线程没有在超时时间内停止")
                
            except Exception as e:
                print(f"停止{self.service_type.upper()}服务器时出错: {e}")
            finally:
                self.running = False
            
            # 如果不是在更改目录过程中，关闭防火墙端口
            if not self._changing_directory and self.current_config['firewall']:
                self._close_firewall_port(self.current_config['port'])
            
            self.server = None
            self.thread = None
            print(f"{self.service_type.upper()}服务器已停止")
            return True
        return False
    
    def change_root_directory(self, new_directory):
        """
        更改服务的根目录
        
        参数:
        new_directory (str): 新的根目录完整路径
        """
        if not self._validate_directory(new_directory):
            return False
        
        print(f"正在更改{self.service_type.upper()}根目录为: {new_directory}")
        
        # 设置标志，表示正在更改目录
        self._changing_directory = True
        
        # 如果服务器正在运行，使用当前配置重新启动服务器
        was_running = self.running
        if was_running:
            self.stop_server()
            time.sleep(0.5)
        
        # 更新配置中的目录
        self.current_config['directory'] = new_directory
        
        # 如果之前服务器在运行，重新启动
        if was_running:
            success = self._start_with_current_config()
            if success:
                print(f"{self.service_type.upper()}根目录已成功更改为: {new_directory}")
            else:
                print(f"{self.service_type.upper()}根目录更改失败")
        else:
            print(f"{self.service_type.upper()}根目录配置已更新为: {new_directory} (服务器未运行)")
            success = True
        
        # 重置标志
        self._changing_directory = False
        return success
    
    def _start_with_current_config(self):
        """使用当前配置启动服务"""
        config = self.current_config
        return self.start_server(
            host=config['host'],
            port=config['port'],
            username=config['username'],
            password=config['password'],
            directory=config['directory'],
            firewall=config['firewall'],
            **config.get('kwargs', {}) # 传递保存的kwargs
        )


class FTPManager(BaseServiceManager):
    """FTP服务管理器"""
    
    def __init__(self):
        super().__init__('ftp')
    
    def _create_server(self, host, port, directory, username=None, password=None, **kwargs):
        """创建FTP服务器实例"""
        try:
            # 延迟导入 pyftpdlib
            from pyftpdlib.authorizers import DummyAuthorizer
            from pyftpdlib.handlers import FTPHandler
            from pyftpdlib.servers import FTPServer
            
            # 从 kwargs 中获取 perm 参数，如果没有则使用默认值
            perm = kwargs.get('perm', 'elradfmw')
            
            authorizer = DummyAuthorizer()
            # 处理用户名密码为None的情况
            if username and password:
                authorizer.add_user(username, password, directory, perm)
            else:
                # 如果未提供凭据，只启用匿名访问
                print("未提供用户名密码，启用匿名FTP访问")

            authorizer.add_anonymous(directory)
            
            handler = FTPHandler
            handler.authorizer = authorizer
            
            return FTPServer((host, port), handler)
        except ImportError:
            print("错误: 未安装 pyftpdlib，无法启动FTP服务")
            print("请使用: pip install pyftpdlib 安装")
            return None
        except Exception as e:
            print(f"创建FTP服务器失败: {e}")
            return None
    
    def _serve_forever_specific(self):
        """FTP服务器运行逻辑"""
        self.server.serve_forever()
    
    def _stop_server_specific(self):
        """FTP服务器停止逻辑"""
        # 关闭所有客户端连接
        self.server.close_all()
        # 关闭服务器
        self.server.close()


class HTTPManager(BaseServiceManager):
    """HTTP服务管理器"""
    
    def __init__(self):
        super().__init__('http')
    
    def _create_server(self, host, port, directory, username=None, password=None, **kwargs):
        """创建HTTP服务器实例"""
        try:
            # 延迟导入 http.server
            from http.server import HTTPServer, SimpleHTTPRequestHandler
            
            # 定义自定义HTTP处理器，支持设置根目录
            class CustomHTTPHandler(SimpleHTTPRequestHandler):
                def __init__(self, *args, **kwargs):
                    self.base_directory = directory
                    super().__init__(*args, **kwargs)
                
                def translate_path(self, path):
                    """重写路径转换方法，使用指定的根目录"""
                    path = super().translate_path(path)
                    
                    if self.base_directory:
                        # 获取相对于原文档根目录的相对路径
                        relpath = os.path.relpath(path, os.getcwd())
                        # 组合新的完整路径
                        new_path = os.path.join(self.base_directory, relpath)
                        return new_path
                    
                    return path
            
            return HTTPServer((host, port), CustomHTTPHandler)
        except ImportError:
            print("错误: 无法导入 http.server 模块")
            return None
        except Exception as e:
            print(f"创建HTTP服务器失败: {e}")
            return None
    
    def _serve_forever_specific(self):
        """HTTP服务器运行逻辑"""
        self.server.serve_forever()
    
    def _stop_server_specific(self):
        """HTTP服务器停止逻辑"""
        # 关闭服务器
        self.server.shutdown()
        self.server.server_close()


class ServiceManagerFactory:
    """服务管理器工厂"""
    
    @staticmethod
    def create_manager(service_type):
        """创建指定类型的服务管理器"""
        if service_type.lower() == 'ftp':
            return FTPManager()
        elif service_type.lower() == 'http':
            return HTTPManager()
        else:
            raise ValueError(f"不支持的服务类型: {service_type}")


# 使用示例:
if __name__ == "__main__":
    # 创建FTP服务管理器
    ftp_mgr = ServiceManagerFactory.create_manager('ftp')
    ftp_mgr.start_server(port=2121, directory='.', perm='elradfmw')
    
    # 创建HTTP服务管理器 
    http_mgr = ServiceManagerFactory.create_manager('http')
    http_mgr.start_server(port=8080, directory='.')
    
    try:
        # 保持服务运行
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("正在停止服务...")
        ftp_mgr.stop_server()
        http_mgr.stop_server()
