#ifndef DB_CONTROLLER_H
#define DB_CONTROLLER_H

#include <QString>
#include <QtSql>

class DbController : public QObject
{
    Q_OBJECT
public:
    explicit DbController(QObject*);
    ~DbController();
    bool checkIfTableExists(QString);
    bool checkIfConnected();

public slots:
    void connectToServerRequested(QString, QString, QString, int, QString, QString, QString, bool);
    void disconnectFromServerRequested();
    void selectTableRequested(QString);
    void selectTableRequested_old(QString);
    void getTablesNamesRequested();
    void Import6AxisCSV(QStringList);
    void Import2AxisCSV(QStringList);
    void deleteDBreceived();

signals:
    void serverConnected();
    void serverErrorWithConnection(QString);
    void tableCreateError(QString);
    void csvImportMessage(QString);
    void serverDisconnected();
    void tableSelected(QSqlQueryModel*, QStringList);
    void tableSelected_old(QSqlQueryModel*, QStringList);
    void gotTablesNames(QStringList);
    void updateCurNum(int);

private:
    bool connectToServerMSSQL(QString, QString, int, QString, QString, QString);
    bool connectToServerMSSQL(QString, QString, int, QString);
    bool connectToServerMySQL(QString, int, QString, QString, QString);
    bool connectToServerPostgreSQL(QString, int, QString, QString, QString);
    void disconnectFromServer();
    QSqlQueryModel* selectTable(QString);
    QSqlError       getLastError();

    QSqlDatabase db;
    static const QString connection_string_sqlauth;
    static const QString connection_string_winauth;
};

#endif // DB_CONTROLLER_H
