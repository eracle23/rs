# 解决 VTK Git Clone 失败问题

## 问题描述

构建时出现错误：
```
CMake Error at slicersources-build/VTK-prefix/tmp/VTK-gitclone.cmake:31 (message):
  Failed to clone repository: 'https://github.com/slicer/VTK.git'
```

这是网络连接问题，无法从 GitHub 克隆 VTK 仓库。

## 解决方案

### 方案 1: 手动下载 VTK（推荐）

如果网络不稳定，可以手动克隆 VTK 仓库：

```cmd
# 1. 切换到构建目录
cd /d V:\b

# 2. 手动克隆 VTK（使用指定的 commit）
git clone https://github.com/slicer/VTK.git VTK
cd VTK
git checkout 492821449a5f2a9a1f5c73c3c6dd4389f1059d66
cd ..

# 3. 继续构建
ninja -j8
```

**注意**：如果 `V:\b\VTK` 目录已存在且包含正确的源码，CMake 会自动跳过下载步骤。

### 方案 2: 使用 GitHub 镜像（如果可用）

如果可以使用 GitHub 镜像（如 `ghproxy.com` 或 `hub.fastgit.xyz`），可以配置 Git：

```cmd
# 配置 Git 使用镜像
git config --global url."https://ghproxy.com/https://github.com/".insteadOf "https://github.com/"

# 或者使用其他镜像
git config --global url."https://hub.fastgit.xyz/".insteadOf "https://github.com/"
```

然后重新运行构建。

### 方案 3: 配置 Git 代理

如果你有 HTTP/HTTPS 代理：

```cmd
# 设置 HTTP 代理
git config --global http.proxy http://proxy.example.com:8080
git config --global https.proxy https://proxy.example.com:8080

# 如果代理需要认证
git config --global http.proxy http://username:password@proxy.example.com:8080

# 重新运行构建
cd /d V:\b
ninja -j8
```

### 方案 4: 使用本地已存在的 VTK 源码

如果你之前已经下载过 VTK（例如在 `C:\S\vs-dev\VTK`），可以：

```cmd
# 1. 复制或创建符号链接到构建目录
cd /d V:\b
# 如果 VTK 在其他位置，可以复制
xcopy /E /I C:\S\vs-dev\VTK V:\b\VTK

# 或者使用 mklink（需要管理员权限）
# mklink /D V:\b\VTK C:\S\vs-dev\VTK

# 2. 确保切换到正确的 commit
cd VTK
git checkout 492821449a5f2a9a1f5c73c3c6dd4389f1059d66
cd ..

# 3. 继续构建
ninja -j8
```

### 方案 5: 修改 CMake 配置使用本地路径（高级）

如果 VTK 源码在其他位置，可以设置 `VTK_SOURCE_DIR`：

```cmd
# 配置时添加参数
cmake -DVTK_SOURCE_DIR:PATH=C:/path/to/VTK ...
```

## 推荐步骤

1. **首先尝试方案 1**（手动下载）：
   ```cmd
   cd /d V:\b
   git clone https://github.com/slicer/VTK.git VTK
   cd VTK
   git checkout 492821449a5f2a9a1f5c73c3c6dd4389f1059d66
   cd ..
   ninja -j8
   ```

2. **如果方案 1 也失败**（网络完全无法访问 GitHub），尝试：
   - 使用 VPN 或代理
   - 使用 GitHub 镜像（方案 2）
   - 从其他能访问 GitHub 的机器下载后复制过来

3. **验证 VTK 源码**：
   ```cmd
   cd /d V:\b\VTK
   git log -1 --oneline
   # 应该显示：4928214 slicer-v9.2.20230607-1ff325c54-2
   ```

## 注意事项

- VTK 仓库很大（几百 MB），下载可能需要较长时间
- 确保下载到正确的 commit：`492821449a5f2a9a1f5c73c3c6dd4389f1059d66`
- 如果 `V:\b\VTK` 目录已存在但不完整，删除后重新下载：
  ```cmd
  rmdir /s /q V:\b\VTK
  ```

## 自动下载脚本

如果需要，我可以创建一个 PowerShell 脚本来自动下载 VTK。
