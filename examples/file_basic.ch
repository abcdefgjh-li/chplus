导入("ch_Lib/file.ch");

定义(空类型) 主函数() {
    控制台输出("=== 文件操作基础示例 ===");
    
    // 检查文件是否存在
    如果 (文件存在("config.txt")) {
        控制台输出("配置文件存在");
    } 否则 {
        控制台输出("配置文件不存在，将创建新文件");
    }
    
    // 写入文件
    定义(字符串) content = "Hello, World!\n这是中文内容。";
    写入文件("output.txt", content);
    控制台输出("已写入内容到 output.txt");
    
    // 读取文件
    定义(字符串) file_content = 读取文件("output.txt");
    控制台输出("文件内容: " + file_content);
}