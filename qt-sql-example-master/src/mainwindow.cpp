#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QMessageBox>
#include <QKeyEvent>
#include <QSettings>
#include <QFileDialog>

SQLWindow::SQLWindow(QWidget* parent, DbController* dbc, QThread* dbt) :
    QWidget(parent),
    ui(new Ui::SQLWindow)
{
    ui->setupUi(this);

    db_controller = dbc;
    db_thread = dbt;

    // check Qt SQL drivers

    QStringList sql_drivers = QSqlDatabase::drivers();
    ui->label_qmysql_icon->setPixmap(
                QPixmap(":/icons/" + QString(sql_drivers.contains("QMYSQL") ? "success" : "failure") + ".ico"));
    ui->label_qodbc_icon->setPixmap(
                QPixmap(":/icons/" + QString(sql_drivers.contains("QODBC") ? "success" : "failure") + ".ico"));
    ui->label_qpsql_icon->setPixmap(
                QPixmap(":/icons/" + QString(sql_drivers.contains("QPSQL") ? "success" : "failure") + ".ico"));

    ui->radio_mysql->setEnabled(sql_drivers.contains("QMYSQL"));
    ui->radio_mssql->setEnabled(sql_drivers.contains("QODBC"));
    ui->radio_mssql->setEnabled(sql_drivers.contains("QPSQL"));

    if (!sql_drivers.contains("QMYSQL") && !sql_drivers.contains("QODBC") && !sql_drivers.contains("QPSQL"))
        ui->groupBox_sql_connect->setEnabled(false);

    //set timer for the storage status update
    QTimer *timer = new QTimer(this);
    timer->start(1000);//update per 1 sec


    // connect signals with slots
    // ui => ui
    connect(ui->button_connect, SIGNAL(clicked()), this, SLOT(connectToServerRequested()));
    connect(ui->radio_mssql, SIGNAL(clicked()), this, SLOT(engineChanged()));
    connect(ui->radio_mysql, SIGNAL(clicked()), this, SLOT(engineChanged()));
    connect(ui->radio_Postsql, SIGNAL(clicked()), this, SLOT(engineChanged()));
    connect(ui->radio_sql_authentication, SIGNAL(clicked()), this, SLOT(authenticationMethodChanged()));
    connect(ui->radio_windows_authentication, SIGNAL(clicked()), this, SLOT(authenticationMethodChanged()));
    connect(ui->button_show_table, SIGNAL(clicked()), this, SLOT(showTableRequested()));
    connect(ui->button_show_table_old, SIGNAL(clicked()), this, SLOT(showTableRequested_old()));
    connect(ui->button_ImportCSV_6_axis, SIGNAL(clicked()), this, SLOT(getCSVroute()));
    connect(ui->button_ImportCSV_2_axis, SIGNAL(clicked()), this, SLOT(getCSVroute()));
    connect(ui->button_dump, SIGNAL(clicked()), this, SLOT(dumpDb()));
    connect(ui->button_delete_data, SIGNAL(clicked()), this, SLOT(deleteDatabase()));
    connect(ui->buttton_Exit, SIGNAL(clicked()), this, SLOT(close()));
    connect(this, SIGNAL(setFileNum(int)), this, SLOT(initProcessDialog(int)));
    connect(timer, SIGNAL(timeout()), this, SLOT(storageStatus()));

    // ui => db_controller

    connect(this, SIGNAL(connectToServer(QString,QString,QString,int,QString,QString,QString,bool)),
            db_controller, SLOT(connectToServerRequested(QString,QString,QString,int,QString,QString,QString,bool)));
    connect(this, SIGNAL(disconnectFromServer()), db_controller, SLOT(disconnectFromServerRequested()));
    connect(this, SIGNAL(selectTable(QString)), db_controller, SLOT(selectTableRequested(QString)));
    connect(this, SIGNAL(selectTable_old(QString)), db_controller, SLOT(selectTableRequested_old(QString)));
    connect(this, SIGNAL(getTablesNames()), db_controller, SLOT(getTablesNamesRequested()));
    connect(this , SIGNAL(CSVFileRead6Axis(QStringList)), db_controller, SLOT(Import6AxisCSV(QStringList)));
    connect(this , SIGNAL(CSVFileRead2Axis(QStringList)), db_controller, SLOT(Import2AxisCSV(QStringList)));
    connect(this, SIGNAL(deleteDBrequested()), db_controller, SLOT(deleteDBreceived()));
     // db_controller => ui

    connect(db_controller, SIGNAL(serverConnected()), this, SLOT(serverConnected()));
    connect(db_controller, SIGNAL(serverErrorWithConnection(QString)), this, SLOT(serverErrorWithConnection(QString)));
    connect(db_controller, SIGNAL(tableCreateError(QString)), this, SLOT(tableCreateError(QString)));
    connect(db_controller, SIGNAL(csvImportMessage(QString)), this, SLOT(csvImportMessage(QString)));
    connect(db_controller, SIGNAL(serverDisconnected()), this, SLOT(serverDisconnected()));
    connect(db_controller, SIGNAL(tableSelected(QSqlQueryModel*, QStringList)), this, SLOT(displayTable(QSqlQueryModel*,QStringList)));
    connect(db_controller, SIGNAL(tableSelected_old(QSqlQueryModel*, QStringList)), this, SLOT(displayTable_old(QSqlQueryModel*,QStringList)));
    connect(db_controller, SIGNAL(gotTablesNames(QStringList)), this, SLOT(fillTablesNames(QStringList)));
    connect(db_controller, SIGNAL(updateCurNum(int)), this, SLOT(updateProcessDialog(int)));

}

SQLWindow::~SQLWindow()
{
    db_thread->exit();
    db_thread->wait();
    delete ui;
}

void SQLWindow::connectToServerRequested()
{
    QString engine;
    if (ui->radio_mysql->isChecked())
        engine = "mysql";
    else if (ui->radio_mssql->isChecked())
        engine = "mssql";
    else if(ui->radio_Postsql->isChecked())
        engine = "postsql";
    else
    {
        QMessageBox::information(this,
                                 "Invalid Engine",
                                 "Choose database engine",
                                 QMessageBox::Ok);
        return;
    }

    QString driver   = ui->lineEdit_driver->text(),
            server   = ui->lineEdit_server_address->text(),
            database = ui->lineEdit_database_name->text(),
            login    = ui->lineEdit_login->text(),
            password = ui->lineEdit_password->text();
    int port = ui->spinBox_server_port->value();

    if (server == "")
    {
        QMessageBox::information(this,
                                 "Invalid Connection Data",
                                 "Insert server address to connect",
                                 QMessageBox::Ok);
        return;
    }

    bool is_sql_authentication = ui->radio_sql_authentication->isChecked();

    if (is_sql_authentication && login == "")
    {
        QMessageBox::information(this,
                                 "Invalid Connection Data",
                                 "Insert login to connect",
                                 QMessageBox::Ok);
        return;
    }

    if (database == "")
    {
        QMessageBox::information(this,
                                 "Invalid Connection Data",
                                 "Insert database name to connect",
                                 QMessageBox::Ok);
        return;
    }

    ui->button_connect->setEnabled(false);
    ui->statusBar->showMessage("Connecting...");

    emit connectToServer(engine, driver, server, port, database, login, password, is_sql_authentication);
}

void SQLWindow::disconnectFromServerRequested()
{
    ui->button_connect->setEnabled(false);

    delete ui->tableView_database_table->model();

    emit disconnectFromServer();
}

void SQLWindow::authenticationMethodChanged()
{
    bool is_sql_authentication = ui->radio_sql_authentication->isChecked();

    ui->lineEdit_login->setEnabled(is_sql_authentication);
    ui->lineEdit_password->setEnabled(is_sql_authentication);
}

void SQLWindow::engineChanged()
{
    bool is_mssql_engine = ui->radio_mssql->isChecked();
//    bool is_postsql_engine = ui->radio_Postsql->isChecked();

    ui->lineEdit_driver->setEnabled(is_mssql_engine);
    ui->radio_windows_authentication->setEnabled(is_mssql_engine    );

    ui->spinBox_server_port->setValue(is_mssql_engine ? 1433 : 5432);

    if (!is_mssql_engine)
    {
        ui->radio_sql_authentication->setChecked(true);
        emit authenticationMethodChanged();
    }

}

void SQLWindow::showTableRequested()
{
    ui->button_show_table->setEnabled(false);

    delete ui->tableView_database_table->model(); // remove old model

    QString table_name = ui->comboBox_table_name->currentText();

    emit selectTable(table_name);
}

void SQLWindow::showTableRequested_old()
{
    ui->button_show_table_old->setEnabled(false);

    delete ui->tableView_database_table_old->model(); // remove old model

    QString table_name = ui->comboBox_table_name_old->currentText();

    emit selectTable_old(table_name);
}

void SQLWindow::serverConnected()
{
    ui->button_connect->setEnabled(true);

    disconnect(ui->button_connect, SIGNAL(clicked()), this, SLOT(connectToServerRequested()));
    connect(ui->button_connect, SIGNAL(clicked()), this, SLOT(disconnectFromServerRequested()));

    ui->button_connect->setText("Disconnect");
    ui->groupBox_database_browser->setEnabled(true);
    ui->groupBox_ImportBtn->setEnabled(true);

    //set backup on after connected
    ui->button_dump->setEnabled(true);
    //set delete off before backup
    ui->button_delete_data->setEnabled(false);

    ui->statusBar->showMessage("Connected", 3000);

    emit getTablesNames();
}

void SQLWindow::fillTablesNames(QStringList tables_names)
{
    if (tables_names.length() == 0)
        QMessageBox::warning(this,
                             "Tables",
                             "There are no tables to display in the database",
                             QMessageBox::Ok);
    else
    {
        ui->comboBox_table_name->addItems(tables_names);

        ui->comboBox_table_name->setEnabled(true);
        ui->comboBox_table_name->setFocus();

        //set old combobox
        ui->comboBox_table_name_old->addItems(tables_names);
        ui->comboBox_table_name_old->setEnabled(true);
    }
}

void SQLWindow::serverErrorWithConnection(QString message)
{
    QMessageBox::critical(this,
                          "Connection failed",
                          message,
                          QMessageBox::Ok);

    ui->button_connect->setEnabled(true);

    ui->statusBar->showMessage("Connection failed", 3000);
}

void SQLWindow::tableCreateError(QString message)
{
    QMessageBox::critical(this,
                          "Table Crete failed",
                          message,
                          QMessageBox::Ok);

    ui->button_connect->setEnabled(true);

    ui->statusBar->showMessage("Table Crete failed", 3000);
}

void SQLWindow::csvImportMessage(QString message)
{
    QMessageBox::information(this,
                          "Import CSV Done!!",
                          message,
                          QMessageBox::Ok);

    ui->button_connect->setEnabled(true);

    ui->statusBar->showMessage("Table Crete failed", 3000);
}


void SQLWindow::serverDisconnected()
{
    disconnect(ui->button_connect, SIGNAL(clicked()), this, SLOT(disconnectFromServerRequested()));
    connect(ui->button_connect, SIGNAL(clicked()), this, SLOT(connectToServerRequested()));

    ui->tableView_database_table->setModel(nullptr);
    ui->tableView_database_table_old->setModel(nullptr);

    ui->button_connect->setEnabled(true);
    ui->button_connect->setText("Connect");

    ui->comboBox_table_name->clear();
    ui->comboBox_table_name->setEnabled(false);

    //old table combobox
    ui->comboBox_table_name_old->clear();
    ui->comboBox_table_name_old->setEnabled(false);

    ui->groupBox_database_browser->setEnabled(false);

    ui->button_connect->setFocus();

    //Import button contorl
    ui->groupBox_ImportBtn->setEnabled(false);

    //Statistic table control
    ui->tableWidget_Static->clearContents();
    ui->tableWidget_Spec->clearContents();
    ui->tableWidget_Static_old->clearContents();
    ui->tableWidget_Spec->clearContents();

    //set backup on after disconnected
    ui->button_dump->setEnabled(false);

}

void SQLWindow::displayTable(QSqlQueryModel* model, QStringList StrListIn)
{
    if (!model->lastError().isValid())
        ui->tableView_database_table->setModel(model);
    else
        QMessageBox::critical(this,
                              "Select failed",
                              model->lastError().databaseText(),
                              QMessageBox::Ok);
    //Write in the 6-axis result
    for(int j =0 ; j < 6; j++)
    {
        for(int i = 0; i < 4; i++)
        {
            ui->tableWidget_Static->setItem(i + 1, j, new QTableWidgetItem(StrListIn.at(j*4+i)));
        }
    }
    //Write in the other item
    for(int i = 0; i < 4; i++)
    {
        int col = i + 6;
        int strListIdx = 24 + i;
        ui->tableWidget_Static->setItem(1, col, new QTableWidgetItem(StrListIn.at(strListIdx)));
        ui->tableWidget_Static->setItem(2, col, new QTableWidgetItem(StrListIn.at(strListIdx + 4*1)));
        ui->tableWidget_Static->setItem(3, col, new QTableWidgetItem(StrListIn.at(strListIdx + 4*2)));
        ui->tableWidget_Static->setItem(4, col, new QTableWidgetItem(StrListIn.at(strListIdx + 4*3)));
    }

    ui->button_show_table->setEnabled(true);
    ui->comboBox_table_name->setFocus();
}

void SQLWindow::displayTable_old(QSqlQueryModel* model, QStringList StrListIn)
{
    if (!model->lastError().isValid())
        ui->tableView_database_table_old->setModel(model);
    else
        QMessageBox::critical(this,
                              "Select failed",
                              model->lastError().databaseText(),
                              QMessageBox::Ok);
    //Write in the 6-axis result
    for(int j =0 ; j < 6; j++)
    {
        for(int i = 0; i < 4; i++)
        {
            ui->tableWidget_Static_old->setItem(i + 1, j, new QTableWidgetItem(StrListIn.at(j*4+i)));
        }
    }
    //Write in the other item
    for(int i = 0; i < 4; i++)
    {
        int col = i + 6;
        int strListIdx = 24 + i;
        ui->tableWidget_Static_old->setItem(1, col, new QTableWidgetItem(StrListIn.at(strListIdx)));
        ui->tableWidget_Static_old->setItem(2, col, new QTableWidgetItem(StrListIn.at(strListIdx + 4*1)));
        ui->tableWidget_Static_old->setItem(3, col, new QTableWidgetItem(StrListIn.at(strListIdx + 4*2)));
        ui->tableWidget_Static_old->setItem(4, col, new QTableWidgetItem(StrListIn.at(strListIdx + 4*3)));
    }

    ui->button_show_table_old->setEnabled(true);
    ui->comboBox_table_name_old->setFocus();
}

void SQLWindow::keyPressEvent(QKeyEvent* pe)
{
    if (pe->key() == Qt::Key_Enter || pe->key() == Qt::Key_Return)
    {
        if (!db_controller->checkIfConnected())
            emit connectToServerRequested();
        else if (ui->comboBox_table_name->isEnabled() && ui->comboBox_table_name->hasFocus())
            emit showTableRequested();
    }
}


void SQLWindow::getCSVroute()
{
    QPushButton *btnSrc = static_cast<QPushButton*>(sender());
    QStringList paths = QFileDialog::getOpenFileNames(this, tr("Open one or more CSV file"), ".", tr("CSV Files(*.csv)"));

    if(paths.size() == 0)
    {
        emit tableCreateError("Please selecting csv file.");
    }
    else
    {
        if(btnSrc == ui->button_ImportCSV_6_axis)
            emit CSVFileRead6Axis(paths);
        else if(btnSrc == ui->button_ImportCSV_2_axis)
            emit CSVFileRead2Axis(paths);
        emit setFileNum(paths.size());
    }



}

void SQLWindow::initProcessDialog(int csvNum)
{
    process = new QProgressDialog("Importing "  + QString::number(csvNum) + " CSV files...", "OK", 0, csvNum, this);
    process->setWindowModality(Qt::WindowModal);
    process->setMinimumDuration(1);
    process->setAutoClose(1);
//    process->setAutoReset(1);
    process->setCancelButton(nullptr);
}

void SQLWindow::updateProcessDialog(int i)
{
    process->setValue(i);
}



void SQLWindow::dumpDb()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open Script"), ".", tr("Script File(*.sh)"));
    QString backUpDir;
    if(fileName.isEmpty())
    {
        QMessageBox::information(this, "Backup Message",
                    "Please select the sh script!!",
                    QMessageBox::Ok);
    }
    else
    {
        //Overwrite the current sh scirpt
        QByteArray fileData;
        QFile file(fileName);
        if(file.open(QIODevice::ReadWrite)) // open for read and write
        {
            fileData = file.readAll(); // read all the data into the byte array
            QString text(fileData); // add to text string for easy string replace
            backUpDir = QFileDialog::getExistingDirectory(this, tr("Select Backup Dest."), ".");
            if(!backUpDir.isEmpty())
            {
                text.replace(QString("_DB_STORAGE"), backUpDir); // replace text in string
            }
            else
            {
                return;
            }
            text.replace(QString("_DB_INSTANCE"), ui->lineEdit_database_name->text());  // replace text in string
            text.replace(QString("_DB_HOST"), ui->lineEdit_server_address->text());     // replace text in string
            text.replace(QString("_DB_USER"), ui->lineEdit_login->text());              // replace text in string
            text.replace(QString("_DB_PASSWORD"), ui->lineEdit_password->text());       // replace text in string
            file.close();
            QStringList fileNameList = fileName.split('/');
            QString fileNameTemp;
            fileNameList.last() = "_"+fileNameList.last();
            fileNameTemp = fileNameList.join('/');


            //Creating the temp script for excute
            QFile fileTemp(fileNameTemp);
            if(fileTemp.open(QIODevice::ReadWrite))
            {
                fileTemp.seek(0);               // go to the beginning of the file
                fileTemp.write(text.toUtf8());  // write the new text back to the file
                fileTemp.close();               // close the file handle.
                QProgressDialog *dialog = new QProgressDialog("Backup Processing..", "OK", 0, 0, this);
                QProcess runScript;
                dialog->setWindowModality(Qt::WindowModal);
                dialog->setCancelButton(nullptr);
                dialog->setWindowTitle("Backuping....");

                connect(&runScript, SIGNAL(finished(int)), dialog, SLOT(close()));
                dialog->show();
                if(runScript.execute("/bin/sh", QStringList{fileNameTemp}) < 0)
                {
                    QMessageBox::information(this, "Backup Message",
                                "Unable to run backup script!!",
                                QMessageBox::Ok);
                }
                else
                {
                    //Set delete button on after backup complete
                    ui->button_delete_data->setEnabled(true);
                }
                dialog->close();
                fileTemp.remove();

            }
            else
            {
                QMessageBox::information(this, "Backup Message",
                            "Temp File open fail!!",
                            QMessageBox::Ok);
            }


        }
        else
        {
            QMessageBox::information(this, "Backup Message",
                        "File open fail!!",
                        QMessageBox::Ok);
        }

    }

}


void SQLWindow::storageStatus()
{
    QString style_0_60      = "QProgressBar::chunk{ background-color: green; }";
    QString style_60_80     = "QProgressBar::chunk{ background-color: orange; }";
    QString style_80_100    = "QProgressBar::chunk{ background-color: red; }";

    QStorageInfo si = QStorageInfo::root();
    qint64 diskAva = si.bytesAvailable();
    qint64 diskTot = si.bytesTotal();
    int diskcur = static_cast<int>(((diskTot - diskAva)*100/diskTot));

    ui->SSDstorageBar->setMaximum(100);
    ui->SSDstorageBar->setValue(diskcur);
    ui->SSDstorageBar->setAlignment(Qt::AlignCenter);

    //Changing the progressbar color with different cases
    if(diskcur <= 60)
        ui->SSDstorageBar->setStyleSheet(style_0_60);
    else if(diskcur <= 80)
    {
        ui->SSDstorageBar->setStyleSheet(style_60_80);
    }
    else
    {
        ui->SSDstorageBar->setStyleSheet(style_80_100);
    }
    if(diskcur>=BACKUP_WINDOW_SHOW_THD)
    {
        QString strDiskcur = QString::number(diskcur);
        int ret = QMessageBox::warning(this, "Storage status warning",
                                                   "SSD is "+strDiskcur+"% full,a backup is suggested!!\n"
                                                   "Do you want to backup now?",
                                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if(ret == QMessageBox::Yes)
        {
            SQLWindow::dumpDb();
        }

    }
    QList<QStorageInfo> list = QStorageInfo::mountedVolumes();
//        qDebug() << "Volume Num: " << list.size();
        for(QStorageInfo& HDDsi : list)
        {
            if(HDDsi.isRoot())
                continue;
            diskAva = HDDsi.bytesAvailable();
            diskTot = HDDsi.bytesTotal();
            diskcur = static_cast<int>(((diskTot - diskAva)*100/diskTot));
            if(diskcur>=BACKUP_WINDOW_SHOW_THD)
            {
                QString strDiskcur = QString::number(diskcur);
                int ret = QMessageBox::warning(this, "Storage status warning",
                                                           "HDD is "+strDiskcur+"% full,a backup is suggested!!\n"
                                                           "Do you want to backup now?",
                                                           QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
                if(ret == QMessageBox::Yes)
                {
                    SQLWindow::dumpDb();
                }

            }

            ui->HDDstorageBar->setMaximum(100);
            ui->HDDstorageBar->setValue(diskcur);
            ui->HDDstorageBar->setAlignment(Qt::AlignCenter);
            if(diskcur <= 60)
                ui->HDDstorageBar->setStyleSheet(style_0_60);
            else if(diskcur <= 80)
                ui->HDDstorageBar->setStyleSheet(style_60_80);
            else
                ui->HDDstorageBar->setStyleSheet(style_80_100);
        }

}

void SQLWindow::deleteDatabase()
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setText("All the table in connected database will be delete!!");
    msgBox.setInformativeText("Are you sure you want to delete all the tables?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    int ret = msgBox.exec();
    switch (ret)
    {
        case QMessageBox::Yes:
        {
            int retre = QMessageBox::warning(this, tr("Did you back up all the tables?"),
                                           tr("Are you sure you want to delete all the tables?"),
                                           QMessageBox::No | QMessageBox::Yes, QMessageBox::No);
            if(retre == QMessageBox::Yes)
            {
                emit deleteDBrequested();
                break;
            }
            else
            {
                return;
            }

        }
        case QMessageBox::No:
        {

            return;

        }
    }



}
