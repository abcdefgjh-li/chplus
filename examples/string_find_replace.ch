导入("ch_Lib/string.ch");

定义(空类型) 主函数() {
    控制台输出("=== 字符串查找和替换 ===");
    
    定义(字符串) text = "The quick brown fox jumps over the lazy dog";
    控制台输出("原文: " + text);
    
    控制台输出("查找 'fox' 位置: " + 查找(text, "fox"));
    控制台输出("替换 'fox' 为 'cat': " + 替换(text, "fox", "cat"));
    
    定义(字符串) repeated = "Hello Hello Hello";
    控制台输出("全局替换 'Hello' 为 'Hi': " + 全部替换(repeated, "Hello", "Hi"));
}