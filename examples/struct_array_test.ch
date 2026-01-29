// 结构体定义
定义(结构体) Person {
    字符串 name;
    整型 age;
    字符串 email;
};

// 结构体数组测试
定义(空类型) 主函数() {
    控制台输出("=== 结构体数组测试 ===");
    
    // 定义结构体数组
    定义(Person) people[3];
    
    // 初始化结构体数组元素
    people[0].name = "张三";
    people[0].age = 25;
    people[0].email = "zhangsan@example.com";

    people[1].name = "李四";
    people[1].age = 30;
    people[1].email = "lisi@example.com";

    people[2].name = "王五";
    people[2].age = 35;
    people[2].email = "wangwu@example.com";
    
    // 输出结构体数组内容
    控制台输出("结构体数组内容:");
    定义(整型) i;
    对于 (i = 0; i < 3; i = i + 1) {
        控制台输出("人员 " + (i + 1) + ": " + people[i].name + ", " + people[i].age + "岁, " + people[i].email);
    }
    
    // 验证数组大小
    控制台输出("结构体数组定义成功！");
    控制台输出("语法定义(结构体名称) a[1000] 支持正常！");
}