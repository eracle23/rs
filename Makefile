# Alice/RadianceSuite 构建 Makefile (Windows)
# 使用方法:
#   make build      - 增量编译
#   make run        - 启动程序
#   make            - 编译并启动
#   make configure  - 仅配置（首次或切换配置时）
#   make install    - 安装到前缀目录
#   make package    - 打包 NSIS 安装器
#   make clean      - 清理内层构建目录
#   make help       - 显示帮助

# ============ 配置变量 ============
# 源码目录
SRC_DIR      := D:/work/RS

# 构建目录（与 CMakePresets.json 中 rs-dev 一致）
BUILD_ROOT   := $(SRC_DIR)/rs-build
INNER_BUILD  := $(BUILD_ROOT)/Slicer-build
INNER_SRC    := $(BUILD_ROOT)/slicersources-src
OUTER_BUILD  := $(BUILD_ROOT)/slicersources-build
INSTALL_DIR  := $(BUILD_ROOT)/install

# 构建配置
CONFIG       := RelWithDebInfo
JOBS         := 6

# 可执行文件（启动器，会自动设置库路径）
APP_EXE      := $(INSTALL_DIR)/Alice.exe
APP_EXE_DEV  := $(INNER_BUILD)/Alice.exe

# ============ 默认目标 ============
.PHONY: all build run configure install package clean clean-cpack help

all: build run

# ============ 构建目标 ============

# 增量编译（推荐日常使用）
build:
	@echo ========================================
	@echo 正在增量编译 ($(CONFIG))...
	@echo ========================================
	cmake --build "$(INNER_BUILD)" --config $(CONFIG) -- /m:$(JOBS) /v:m

# 完整构建（使用脚本，含翻译部署）
build-full:
	@echo ========================================
	@echo 正在完整构建 ($(CONFIG))...
	@echo ========================================
	pwsh -ExecutionPolicy Bypass -File Tools/Invoke-RadianceBuild.ps1 \
		-Preset rs-dev -InnerOnly -InnerConfig $(CONFIG) -Jobs $(JOBS)

# 仅配置（首次或需要重新配置时）
configure:
	@echo ========================================
	@echo 正在配置内层构建...
	@echo ========================================
	cmake -G "Visual Studio 17 2022" -A x64 \
		-S "$(INNER_SRC)" \
		-B "$(INNER_BUILD)" \
		-C "$(OUTER_BUILD)/Slicer-prefix/tmp/Slicer-cache-$(CONFIG).cmake"

# 顶层配置（仅首次需要）
configure-top:
	@echo ========================================
	@echo 正在配置顶层构建...
	@echo ========================================
	cmake --preset rs-dev

# ============ 安装与打包 ============

# 安装到前缀目录
install: build
	@echo ========================================
	@echo 正在安装到 $(INSTALL_DIR)...
	@echo ========================================
	cmake --build "$(INNER_BUILD)" --config $(CONFIG) --target INSTALL -- /m:$(JOBS)

# 打包 NSIS 安装器
package: install
	@echo ========================================
	@echo 正在打包...
	@echo ========================================
	cmake --build "$(INNER_BUILD)" --config $(CONFIG) --target PACKAGE -- /m:$(JOBS) /v:m

# ============ 运行目标 ============

# 启动程序（优先使用开发构建目录）
run:
	@echo ========================================
	@echo 正在启动 Alice Studio...
	@echo ========================================
	@if exist "$(subst /,\,$(APP_EXE_DEV))" ( \
		start "" "$(APP_EXE_DEV)" \
	) else if exist "$(subst /,\,$(APP_EXE))" ( \
		start "" "$(APP_EXE)" \
	) else ( \
		echo 错误: 未找到可执行文件，请先运行 make build \
		exit /b 1 \
	)

# 从安装目录启动
run-installed:
	@echo 正在从安装目录启动...
	start "" "$(APP_EXE)"

# ============ 清理目标 ============

# 清理 CPack 临时文件（打包失败后重试前使用）
clean-cpack:
	@echo 正在清理 CPack 临时文件...
	-rd /s /q "$(subst /,\,$(INNER_BUILD))\_CPack_Packages" 2>nul

# 清理内层构建目录（谨慎使用，需要重新编译）
clean:
	@echo ========================================
	@echo 警告: 这将删除整个内层构建目录！
	@echo 按 Ctrl+C 取消，或按任意键继续...
	@echo ========================================
	@pause
	-rd /s /q "$(subst /,\,$(INNER_BUILD))" 2>nul

# ============ 辅助目标 ============

# 部署翻译文件
deploy-translations:
	@echo 正在部署翻译文件...
	pwsh -ExecutionPolicy Bypass -File Tools/Deploy-Translations.ps1 \
		-BuildDir "$(INNER_BUILD)"

# 显示帮助
help:
	@echo.
	@echo Alice/RadianceSuite 构建系统
	@echo ============================================================
	@echo.
	@echo 常用命令:
	@echo   make              - 编译并启动程序
	@echo   make build        - 仅增量编译
	@echo   make run          - 仅启动程序
	@echo   make build-full   - 完整构建（含翻译部署）
	@echo.
	@echo 配置命令:
	@echo   make configure-top  - 顶层配置（仅首次）
	@echo   make configure      - 内层配置
	@echo.
	@echo 发布命令:
	@echo   make install      - 安装到前缀目录
	@echo   make package      - 打包 NSIS 安装器
	@echo.
	@echo 其他命令:
	@echo   make clean-cpack  - 清理打包临时文件
	@echo   make clean        - 清理内层构建目录
	@echo   make deploy-translations - 部署翻译文件
	@echo.
	@echo 当前配置:
	@echo   构建配置: $(CONFIG)
	@echo   并行度:   $(JOBS)
	@echo   构建目录: $(INNER_BUILD)
	@echo   安装目录: $(INSTALL_DIR)
	@echo.
