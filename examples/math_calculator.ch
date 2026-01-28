导入("ch_Lib/math.ch");

定义(空类型) 主函数() {
    // 计算圆的面积
    定义(小数) radius = 5.0;
    定义(小数) area = PI * radius * radius;
    控制台输出("半径为 " + radius + " 的圆面积 = " + area);
    
    // 计算复利
    定义(小数) principal = 1000.0;
    定义(小数) rate = 0.05;
    定义(整型) years = 3;
    定义(小数) amount = principal * (1 + rate) * (1 + rate) * (1 + rate);
    控制台输出("3年后本息 = " + amount);
    
    // 角度转换
    定义(小数) degrees = 180.0;
    定义(小数) radians = PI * degrees / 180.0;
    控制台输出(degrees + "° = " + radians + " 弧度");
}