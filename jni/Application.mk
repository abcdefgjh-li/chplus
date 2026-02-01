# 应用配置
APP_ABI := arm64-v8a
APP_PLATFORM := android-21
APP_STL := c++_static
APP_CPPFLAGS := -std=c++17 -frtti -fexceptions

# # 启用对 C++ 的Ollvm支持
 APP_CXXFLAGS += -mllvm -irobf-indbr -mllvm -irobf-icall -mllvm -irobf-indgv -mllvm -irobf-cse -mllvm -irobf-cff

# # 启用对 C 的Ollvm支持
 APP_CFLAGS += -mllvm -irobf-indbr -mllvm -irobf-icall -mllvm -irobf-indgv -mllvm -irobf-cse -mllvm -irobf-cff