#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <QApplication>
#include <QLocalServer>
#include <QLocalSocket>

class SingleApplication : public QApplication
{
    Q_OBJECT
public:
    SingleApplication(int &argc, char **argv, const QString &serverName);
    bool isRunning();
    void closeInstance();

signals:
    void instanceStarted();

private slots:
    void newLocalConnection();

private:
    void initLocalConnection();
    void newLocalServer();

    QLocalServer *m_localServer;
    QString m_serverName;
    bool m_isRunning;
};

#endif // SINGLEAPPLICATION_H
