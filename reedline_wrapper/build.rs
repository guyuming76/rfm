use std::env;
use std::path::PathBuf;

fn main() {
    // 告诉 Cargo 如果 rfm.c 发生变化需要重新编译
    println!("cargo:rerun-if-changed=../rfm.c");
    // 假设 librfm.so 已经编译并安装在系统路径中，或者指定库路径
    // 这里我们假设 librfm.so 在 ../ 目录下（由 Makefile 生成）
    let out_dir = env::var("OUT_DIR").unwrap();
    println!("cargo:rustc-link-search=native=../");
    println!("cargo:rustc-link-lib=dylib=rfm"); // 链接 librfm.so
    // 添加系统库链接
    println!("cargo:rustc-link-lib=readline");
}
