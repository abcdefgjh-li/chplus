定义(空类型) 主函数() {
    // 写入文件
    定义(字符串) content = "Hello, File!";
    文件写入("test.txt", content);
    
    // 读取文件
    定义(字符串) fileContent;
    文件读取("test.txt", fileContent);
    控制台输出("读取的内容: " + fileContent);
    
    // 追加内容
    文件追加("test.txt", "\n这是追加的内容");
}