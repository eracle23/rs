/*==============================================================================

  Copyright (c) Vision Magic Ecosystem

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0.txt

==============================================================================*/

#include "AppLogger.h"

// Qt includes
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QMutexLocker>
#include <QDebug>
#include <cstdio>

//-----------------------------------------------------------------------------
AppLogger& AppLogger::instance()
{
  static AppLogger instance;
  return instance;
}

//-----------------------------------------------------------------------------
AppLogger::AppLogger()
  : QObject(nullptr)
  , m_maxLogAgeDays(30)
  , m_initialized(false)
{
}

//-----------------------------------------------------------------------------
AppLogger::~AppLogger()
{
  shutdown();
}

//-----------------------------------------------------------------------------
bool AppLogger::initialize()
{
  QMutexLocker locker(&m_mutex);
  
  if (m_initialized)
  {
    return true;
  }

  ensureLogDirectory();
  rotateLogFileIfNeeded();

  // 在用户临时目录写入日志路径标记，便于查找
  QString markerPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/VisionMagic_log_path.txt";
  QFile marker(markerPath);
  if (marker.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    QTextStream ts(&marker);
    ts.setCodec("UTF-8");
    ts << m_logDirectory << "\n" << m_currentLogPath;
    marker.close();
  }

  m_initialized = true;
  
  // 记录应用启动
  locker.unlock();
  info(QString::fromUtf8("应用程序启动"), "System");
  info(QString("Version: %1").arg(QCoreApplication::applicationVersion()), "System");
  
  return true;
}

//-----------------------------------------------------------------------------
void AppLogger::shutdown()
{
  QMutexLocker locker(&m_mutex);
  
  if (!m_initialized)
  {
    return;
  }

  if (m_logFile.isOpen())
  {
    // 记录应用关闭
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString entry = QString("[%1] [INFO] [System] %2\n")
                    .arg(timestamp)
                    .arg(QString::fromUtf8("应用程序关闭"));
    
    QTextStream stream(&m_logFile);
    stream.setCodec("UTF-8");
    stream << entry;
    stream.flush();
    
    m_logFile.close();
  }

  m_initialized = false;
}

//-----------------------------------------------------------------------------
void AppLogger::ensureLogDirectory()
{
  // 优先使用 AppData（在 setApplicationName 后调用时路径正确，且用户总有写权限）
  QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if (!appDataPath.isEmpty())
  {
    m_logDirectory = appDataPath + "/logs";
    QDir dir(m_logDirectory);
    if (!dir.exists())
    {
      dir.mkpath(".");
    }
    // 验证可写
    QFile testFile(m_logDirectory + "/.write_test");
    if (testFile.open(QIODevice::WriteOnly))
    {
      testFile.close();
      testFile.remove();
      return;
    }
  }

  // 回退：安装目录
  QString installDir = QCoreApplication::applicationDirPath();
  if (!installDir.isEmpty())
  {
    QString installLogDir = installDir + "/logs";
    QDir installDirObj(installLogDir);
    if (!installDirObj.exists())
    {
      installDirObj.mkpath(".");
    }
    QFile testFile(installLogDir + "/.write_test");
    if (testFile.open(QIODevice::WriteOnly))
    {
      testFile.close();
      testFile.remove();
      m_logDirectory = installLogDir;
      return;
    }
  }

  // 最后回退：系统临时目录
  QString tempPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  m_logDirectory = tempPath + "/VisionMagic_logs";
  QDir tempDir(m_logDirectory);
  if (!tempDir.exists())
  {
    tempDir.mkpath(".");
  }
}

//-----------------------------------------------------------------------------
void AppLogger::rotateLogFileIfNeeded()
{
  QDate today = QDate::currentDate();
  
  // 如果日期变化或文件未打开，创建新的日志文件
  if (m_currentLogDate != today || !m_logFile.isOpen())
  {
    if (m_logFile.isOpen())
    {
      m_logFile.close();
    }
    
    m_currentLogDate = today;
    m_currentLogPath = QString("%1/VisionMagic_%2.log")
                       .arg(m_logDirectory)
                       .arg(today.toString("yyyy-MM-dd"));
    
    m_logFile.setFileName(m_currentLogPath);
    if (!m_logFile.open(QIODevice::Append | QIODevice::Text))
    {
      // 避免 qWarning 触发 messageHandler -> AppLogger 导致死锁
      fprintf(stderr, "AppLogger: Failed to open log file: %s\n", qPrintable(m_currentLogPath));
    }
    
    // 清理旧日志
    cleanupOldLogs();
  }
}

//-----------------------------------------------------------------------------
void AppLogger::log(LogLevel level, const QString& message, const QString& category)
{
  if (!m_initialized)
  {
    initialize();
  }

  QString entry;
  {
    QMutexLocker locker(&m_mutex);
    rotateLogFileIfNeeded();
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString levelStr = levelToString(level);
    QString categoryStr = category.isEmpty() ? "General" : category;
    entry = QString("[%1] [%2] [%3] %4\n")
            .arg(timestamp)
            .arg(levelStr)
            .arg(categoryStr)
            .arg(message);
    writeToFile(entry);
  }
  // mutex 已释放后再调用 qInfo/qDebug，避免 radianceQtMessageHandler 回调 AppLogger 时死锁
  switch (level)
  {
    case Debug:
      qDebug().noquote() << entry.trimmed();
      break;
    case Info:
      qInfo().noquote() << entry.trimmed();
      break;
    case Warning:
      qWarning().noquote() << entry.trimmed();
      break;
    case Error:
    case Critical:
      qCritical().noquote() << entry.trimmed();
      break;
  }
  emit logEntryAdded(level, message, category.isEmpty() ? QString("General") : category);
}

//-----------------------------------------------------------------------------
void AppLogger::writeToFile(const QString& entry)
{
  if (m_logFile.isOpen())
  {
    QTextStream stream(&m_logFile);
    stream.setCodec("UTF-8");
    stream << entry;
    stream.flush();
  }
}

//-----------------------------------------------------------------------------
QString AppLogger::levelToString(LogLevel level) const
{
  switch (level)
  {
    case Debug:    return "DEBUG";
    case Info:     return "INFO";
    case Warning:  return "WARNING";
    case Error:    return "ERROR";
    case Critical: return "CRITICAL";
    default:       return "UNKNOWN";
  }
}

//-----------------------------------------------------------------------------
void AppLogger::debug(const QString& message, const QString& category)
{
  log(Debug, message, category);
}

//-----------------------------------------------------------------------------
void AppLogger::info(const QString& message, const QString& category)
{
  log(Info, message, category);
}

//-----------------------------------------------------------------------------
void AppLogger::warning(const QString& message, const QString& category)
{
  log(Warning, message, category);
}

//-----------------------------------------------------------------------------
void AppLogger::error(const QString& message, const QString& category)
{
  log(Error, message, category);
}

//-----------------------------------------------------------------------------
void AppLogger::critical(const QString& message, const QString& category)
{
  log(Critical, message, category);
}

//-----------------------------------------------------------------------------
void AppLogger::logUserAction(const QString& action, const QString& details)
{
  QString message = action;
  if (!details.isEmpty())
  {
    message += QString(" - %1").arg(details);
  }
  log(Info, message, "UserAction");
}

//-----------------------------------------------------------------------------
QString AppLogger::logDirectory() const
{
  return m_logDirectory;
}

//-----------------------------------------------------------------------------
QString AppLogger::currentLogFile() const
{
  return m_currentLogPath;
}

//-----------------------------------------------------------------------------
void AppLogger::setMaxLogAgeDays(int days)
{
  m_maxLogAgeDays = days;
}

//-----------------------------------------------------------------------------
void AppLogger::cleanupOldLogs()
{
  QDir dir(m_logDirectory);
  QStringList filters;
  filters << "VisionMagic_*.log";
  
  QFileInfoList fileList = dir.entryInfoList(filters, QDir::Files, QDir::Time);
  QDate cutoffDate = QDate::currentDate().addDays(-m_maxLogAgeDays);
  
  for (const QFileInfo& fileInfo : fileList)
  {
    // 从文件名解析日期
    QString fileName = fileInfo.baseName();
    QString dateStr = fileName.mid(12); // 跳过 "VisionMagic_"
    QDate fileDate = QDate::fromString(dateStr, "yyyy-MM-dd");
    
    if (fileDate.isValid() && fileDate < cutoffDate)
    {
      QFile::remove(fileInfo.absoluteFilePath());
      // 避免 qDebug 触发 messageHandler -> AppLogger 导致死锁（cleanupOldLogs 在 log() 持有 mutex 时调用）
      fprintf(stderr, "AppLogger: Removed old log file: %s\n", qPrintable(fileInfo.fileName()));
    }
  }
}

