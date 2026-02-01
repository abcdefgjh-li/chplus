LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

# 模块名称
LOCAL_MODULE := chplus

# C++标准
LOCAL_CPPFLAGS := -std=c++17

# 包含路径
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../include

# 源文件列表
LOCAL_SRC_FILES := \
    ../main.cpp \
    ../src/lexer.cpp \
    ../src/parser.cpp \
    ../src/interpreter.cpp \
    ../src/CodeFormatter.cpp \
    ../src/CHFormatter.cpp \
    ../src/asm.cpp

# 静态库依赖
LOCAL_STATIC_LIBRARIES := 

# 共享库依赖
LOCAL_SHARED_LIBRARIES := 

# 链接器标志
LOCAL_LDLIBS := -llog

# 构建为共享库
include $(BUILD_EXECUTABLE)