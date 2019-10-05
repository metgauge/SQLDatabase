#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "db_controller.h"

#include <QMainWindow>
#include <QProgressDialog>

#define BACKUP_WINDOW_SHOW_THD 95

namespace Ui
{
    class SQLWindow;
}

class SQLWindow : public QWidget
{
    Q_OBJECT
    
public:
    explicit SQLWindow(QWidget* parent, DbController* dbc, QThread* dbt);
    ~SQLWindow();
    QProgressDialog *process;


public slots:
    void connectToServerRequested();
    void disconnectFromServerRequested();
    void engineChanged();
    void authenticationMethodChanged();
    void showTableRequested();
    void showTableRequested_old();

    void serverConnected();
    void serverErrorWithConnection(QString);
    void serverDisconnected();
    void displayTable(QSqlQueryModel*, QStringList);
    void displayTable_old(QSqlQueryModel*, QStringList);
    void fillTablesNames(QStringList);
    void tableCreateError(QString);
    void csvImportMessage(QString);
    void getCSVroute();
    void initProcessDialog(int);
    void updateProcessDialog(int);
    void dumpDb();
    void storageStatus();
    void deleteDatabase();

signals:
    void connectToServer(QString, QString, QString, int, QString, QString, QString, bool);
    void disconnectFromServer();
    void selectTable(QString);
    void selectTable_old(QString);
    void getTablesNames();
    void newTableGen(QString);
    void CSVFileRead6Axis(QStringList);
    void CSVFileRead2Axis(QStringList);
    void setFileNum(int);
    void deleteDBrequested();

private:
    Ui::SQLWindow* ui;
    DbController*   db_controller;
    QThread*        db_thread;

protected:
    virtual void keyPressEvent(QKeyEvent*);

};

#endif // MAINWINDOW_H
