/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#ifndef __AppLogger_h
#define __AppLogger_h

// Radiance includes
#include "qRadianceAppExport.h"

#include <QObject>
#include <QString>
#include <QFile>
#include <QMutex>
#include <QDateTime>

/**
 * @brief 应用程序日志管理器
 * 
 * 提供应用级别的日志记录功能：
 * - 记录用户操作（打开文件、保存、导出等）
 * - 记录错误和警告
 * - 日志文件存储至安装目录下的 logs/（若不可写则回退到 %APPDATA%/VisionMagic/logs/）
 * - 支持日志轮转（按日期）
 */
class Q_RADIANCE_APP_EXPORT AppLogger : public QObject
{
  Q_OBJECT

public:
  /**
   * @brief 日志级别
   */
  enum LogLevel
  {
    Debug = 0,
    Info = 1,
    Warning = 2,
    Error = 3,
    Critical = 4
  };
  Q_ENUM(LogLevel)

  /**
   * @brief 获取单例实例
   */
  static AppLogger& instance();

  /**
   * @brief 初始化日志系统
   * @return true 初始化成功
   */
  bool initialize();

  /**
   * @brief 关闭日志系统
   */
  void shutdown();

  /**
   * @brief 记录日志
   * @param level 日志级别
   * @param message 日志消息
   * @param category 日志分类（可选）
   */
  void log(LogLevel level, const QString& message, const QString& category = QString());

  // 便捷方法
  void debug(const QString& message, const QString& category = QString());
  void info(const QString& message, const QString& category = QString());
  void warning(const QString& message, const QString& category = QString());
  void error(const QString& message, const QString& category = QString());
  void critical(const QString& message, const QString& category = QString());

  /**
   * @brief 记录用户操作
   * @param action 操作类型
   * @param details 详细信息
   */
  void logUserAction(const QString& action, const QString& details = QString());

  /**
   * @brief 获取日志目录路径
   */
  QString logDirectory() const;

  /**
   * @brief 获取当前日志文件路径
   */
  QString currentLogFile() const;

  /**
   * @brief 设置最大日志文件保留天数
   */
  void setMaxLogAgeDays(int days);

  /**
   * @brief 清理过期的日志文件
   */
  void cleanupOldLogs();

signals:
  /**
   * @brief 新日志条目信号
   */
  void logEntryAdded(LogLevel level, const QString& message, const QString& category);

protected:
  AppLogger();
  virtual ~AppLogger();

private:
  Q_DISABLE_COPY(AppLogger)

  void ensureLogDirectory();
  void rotateLogFileIfNeeded();
  QString levelToString(LogLevel level) const;
  void writeToFile(const QString& entry);

  QFile m_logFile;
  QMutex m_mutex;
  QString m_logDirectory;
  QString m_currentLogPath;
  QDate m_currentLogDate;
  int m_maxLogAgeDays;
  bool m_initialized;
};

// 便捷宏定义
#define APP_LOG_DEBUG(msg) AppLogger::instance().debug(msg)
#define APP_LOG_INFO(msg) AppLogger::instance().info(msg)
#define APP_LOG_WARNING(msg) AppLogger::instance().warning(msg)
#define APP_LOG_ERROR(msg) AppLogger::instance().error(msg)
#define APP_LOG_CRITICAL(msg) AppLogger::instance().critical(msg)
#define APP_LOG_ACTION(action, details) AppLogger::instance().logUserAction(action, details)

#endif // __AppLogger_h

