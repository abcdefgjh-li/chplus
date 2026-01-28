导入("ch_Lib/math.ch");

定义(空类型) 主函数() {
    // 计算角度的三角函数值
    定义(小数) angle = 45.0;
    定义(小数) sin_val = sin(angle);
    定义(小数) cos_val = cos(angle);
    定义(小数) tan_val = tan(angle);
    
    控制台输出("角度 " + angle + "° 的三角函数值:");
    控制台输出("sin(" + angle + "°) = " + sin_val);
    控制台输出("cos(" + angle + "°) = " + cos_val);
    控制台输出("tan(" + angle + "°) = " + tan_val);
}