定义(结构体) Point {
    整型 x;
    整型 y;
    字符串 color;
};

定义(空类型) 主函数() {
    // 创建结构体变量
    定义(Point) point1;
    
    // 直接对结构体成员赋值
    point1.x = 100;
    point1.y = 200;
    point1.color = "红色";
    
    // 输出结构体成员
    控制台输出("Point: x=" + point1.x + ", y=" + point1.y + ", color=" + point1.color);
}