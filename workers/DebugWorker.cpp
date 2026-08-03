#include "../workers/DebugWorker.h"
#include <QCoreApplication>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <cstdio>

DebugWorker::DebugWorker(QObject *parent) : QObject(parent) {}

void DebugWorker::handleWriteLog(const LogTask &task)
{
    // 1. 路径处理
    QString pureName = task.fileName;
    if (pureName.endsWith(".log")) pureName.chop(4);

    QString logDir = QCoreApplication::applicationDirPath() + "/logs";
    QString logPath = logDir + "/" + pureName + ".log";

    // 2. 确保目录存在
    QDir().mkpath(logDir);

    // 3. 物理写入
    QFile file(logPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setCodec("UTF-8");

        QString prefix = "";
        if (task.level > 0) {
            for (int i = 0; i < task.level - 1; ++i) {
                prefix += "│   ";
            }
            prefix += task.isLast ? "└── " : "├── ";
        }

        QString logEntry = QString("[%1] %2%3\n").arg(task.timestamp, prefix, task.message);

        stream << logEntry;
        file.close();

        QString fullConsoleMsg = QString("[%1] %2").arg(pureName.toUpper(), logEntry);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            std::wstring wMsg = fullConsoleMsg.toStdWString();
            DWORD written = 0;
            WriteConsoleW(hOut, wMsg.c_str(), (DWORD)wMsg.length(), &written, NULL);
        }
    }
}
