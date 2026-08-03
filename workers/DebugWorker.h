#ifndef DEBUGWORKER_H
#define DEBUGWORKER_H

#include <QObject>
#include <QString>
#include <QDateTime>
#include <QMetaType>
#include <QStringList>

#include <windows.h>

struct LogTask {
    QString fileName;
    QString message;
    int level;
    bool isLast;
    QString timestamp;
};
Q_DECLARE_METATYPE(LogTask)

class DebugWorker : public QObject
{
    Q_OBJECT
public:
    explicit DebugWorker(QObject *parent = nullptr);

public slots:
    void handleWriteLog(const LogTask &task);

private:
    void ensureLogDirExists();
};

#endif // DEBUGWORKER_H
