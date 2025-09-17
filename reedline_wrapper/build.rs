use std::env;
use std::path::PathBuf;
use std::fs;
use std::os::unix::fs::symlink;

fn main() {
    // 告诉 Cargo 如果 rfm.c 发生变化需要重新编译
    println!("cargo:rerun-if-changed=../rfm.c");
    
    // 获取版本号，默认为 2.80.0
    let version = env::var("RFM_VERSION").unwrap_or_else(|_| "2.80.0".to_string());
    let lib_name = format!("librfm.so.{}", version);
    
    // 检查上级目录中的 librfm.so.2.80.0 文件
    let parent_dir = PathBuf::from("..");
    let lib_file = parent_dir.join(&lib_name);
    
    if lib_file.exists() {
        // 创建符号链接 librfm.so 指向实际文件
        let symlink_path = parent_dir.join("librfm.so");
        
        // 如果符号链接已存在，先删除它
        if symlink_path.exists() {
            let _ = fs::remove_file(&symlink_path);
        }
        
        // 创建新的符号链接
        if let Err(e) = symlink(&lib_name, &symlink_path) {
            eprintln!("Failed to create symbolic link: {}", e);
        } else {
            println!("Created symbolic link: {} -> {}", symlink_path.display(), lib_name);
        }
        
        // 添加库搜索路径
        println!("cargo:rustc-link-search=native=../");
    } else {
        eprintln!("Warning: {} not found in parent directory", lib_name);
    }
    
    // 链接 librfm.so 和 readline
    println!("cargo:rustc-link-lib=dylib=rfm");
    println!("cargo:rustc-link-lib=readline");
}
