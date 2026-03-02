# Slicer 构建错误：curl/curl.h 找不到（C1083）

## 错误现象

- **MSB8066**：Slicer 自定义生成步骤退出，代码为 1  
- **C1083**：`vtkHTTPHandler.cxx` 无法打开包含文件 `curl/curl.h`（RemoteIO 项目）

## 原因

RemoteIO 模块使用 `CURL_INCLUDE_DIRS` 和 `CURL_LIBRARIES`，而 SuperBuild 的 `External_curl` 只传递 `CURL_INCLUDE_DIR` 和 `CURL_LIBRARY`。当通过 CURLConfig.cmake（config 模式）查找 CURL 时，可能不会设置 `CURL_INCLUDE_DIRS`，导致编译时找不到头文件路径。

## 解决方案

### 方案一：使用本地 Slicer 源码（推荐）

若使用 `C:/b/slicersources-src` 作为 Slicer 源码，请确保该目录已包含 curl 修复（可通过补丁或手动应用）。

1. 清理并重新配置：

```powershell
cd E:\build
del CMakeCache.txt
cmake -G "Visual Studio 17 2022" -A x64 -Dslicersources_SOURCE_DIR=C:/b/slicersources-src -Dslicersources_BINARY_DIR=E:/build/slicersources-subbuild E:\GitHub\rs0\rs
```

2. 删除补丁标记，让补丁重新应用（若有其他补丁）：

```powershell
del C:\b\slicersources-src\.rs_patches_applied
```

3. 重新生成 Slicer 项目。

### 方案二：手动修改 C:\b\slicersources-src

若 Slicer 源码在 `C:\b\slicersources-src`，请手动编辑：

**文件**：`C:\b\slicersources-src\Libs\RemoteIO\CMakeLists.txt`

在 `find_package(CURL REQUIRED)` 之后、`# OpenSSL` 之前添加：

```cmake
# CURLConfig.cmake (config mode) may not set CURL_INCLUDE_DIRS/CURL_LIBRARIES,
# but SuperBuild passes CURL_INCLUDE_DIR and CURL_LIBRARY. Provide fallback.
if(NOT CURL_INCLUDE_DIRS AND CURL_INCLUDE_DIR)
  set(CURL_INCLUDE_DIRS ${CURL_INCLUDE_DIR})
endif()
if(NOT CURL_LIBRARIES AND CURL_LIBRARY)
  set(CURL_LIBRARIES ${CURL_LIBRARY})
endif()
```

修改后，删除 `E:\build\Slicer-build`，重新运行 CMake 配置，再重新生成 Slicer。

### 方案三：完全清理后重建

有时 Slicer 内部构建在 curl 尚未就绪时完成配置，会导致路径错误。

1. 删除 Slicer 相关构建目录：

```powershell
cd E:\build
Remove-Item -Recurse -Force Slicer-build -ErrorAction SilentlyContinue
```

2. 重新配置：

```powershell
cmake E:\GitHub\rs0\rs
```

3. 在 Visual Studio 中重新生成 Slicer 项目。

---

## 验证

确认 `E:\build\curl-install\include\curl\curl.h` 存在，且 CMake 配置时 `CURL_INCLUDE_DIR` 已正确传递到 Slicer 内部构建。
