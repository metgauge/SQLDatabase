#include "db_controller.h"

DbController::DbController(QObject* parent) :
    QObject(parent)
{

}

DbController::~DbController()
{
    if (db.isOpen())
        db.close();
}

void DbController::connectToServerRequested(QString engine, QString driver, QString server, int port, QString database,
                                            QString login, QString password, bool is_sql_authentication)
{
    db = QSqlDatabase();
    db.removeDatabase("example-connection"); // remove old connection if exists

    if (engine == "mysql")
    {
        db = QSqlDatabase::addDatabase("QMYSQL", "example-connection");
    }
    else if (engine == "mssql")
    {
        db = QSqlDatabase::addDatabase("QODBC", "example-connection");
    }
    else if (engine == "postsql")
    {
        db = QSqlDatabase::addDatabase("QPSQL", "PostgreSQL-connection");
    }
    else
    {
        emit serverErrorWithConnection("Unknown database engine");
        return;
    }

    bool connection_succesfull;

    if (engine == "mysql")
    {
        connection_succesfull = connectToServerMySQL(server, port, database, login, password);
    }
    else if (engine == "mssql")
    {
        connection_succesfull =
                (is_sql_authentication ? connectToServerMSSQL(driver, server, port, database, login, password) :
                                         connectToServerMSSQL(driver, server, port, database));
    }
    else if(engine == "postsql")
    {
        connection_succesfull = connectToServerPostgreSQL(server, port, database, login, password);
    }
    else
    {
        emit serverErrorWithConnection("Unknown database engine");
        return;
    }

    if (connection_succesfull)
        emit serverConnected();
    else
        emit serverErrorWithConnection(getLastError().driverText());
}

void DbController::disconnectFromServerRequested()
{
    disconnectFromServer();

    emit serverDisconnected();
}

bool DbController::checkIfTableExists(QString table)
{
    return db.tables().contains(table);
}

bool DbController::checkIfConnected()
{
    return db.isOpen();
}

void DbController::selectTableRequested(QString table)
{
    QSqlQueryModel* model = selectTable(table);
    QStringList StrList;
    double *Output_array = new double [4*6]();
    QSqlQuery query(db);
    query.setForwardOnly(true);

    for(int j = 1; j <= 6; j++)
    {
        QString axis_num = "axis_"+ QString::number(j);

        if(!query.exec("SELECT MIN("+axis_num+"), AVG("+axis_num+"), MAX("+axis_num+"), STDDEV("+axis_num+") FROM "+ table + " WHERE "+axis_num+">0"))
        {
            emit tableCreateError(query.lastError().text());
        }
        query.next();

        for(int i =0; i <4; i++)
        {
            //Get the query result
            double value = query.value(i).toDouble();
            StrList.append(QString::number(value, 'd', 4)+" mm");
            Output_array[(j - 1)*4 + i] = value;
        }

    }
    if(!query.exec("SELECT (axis_1+axis_2+axis_3+axis_4+axis_5+axis_6)/6 FROM " + table))
    {
        emit tableCreateError(query.lastError().text());
    }
    query.next();
    double allAvg = query.value(0).toDouble();

    for(int i = 0; i <4; i++)
    {
        double max = std::numeric_limits<double>::min();
        double min = std::numeric_limits<double>::max();
        double avg = 0;
        double ova = 0;
        for(int j = 0; j < 6; j ++)
        {
            double curr = Output_array[i + j*4];
            avg += curr;
            if(curr > max)
                max = curr;
            if(curr < min)
                min = curr;
        }
        avg /= 6.0;
        ova = max - min;
        if(i!=1)
            StrList.append(QString::number(avg, 'd' ,4) + " mm");
        else
            StrList.append(QString::number(allAvg, 'd',4) + " mm");

        StrList.append(QString::number(ova, 'd', 4) + " mm");
        StrList.append(QString::number(min, 'd', 4) + " mm");
        StrList.append(QString::number(max, 'd', 4) + " mm");
    }



    emit tableSelected(model, StrList);
}
void DbController::selectTableRequested_old(QString table)
{
    QSqlQueryModel* model = selectTable(table);
    QStringList StrList;
    double *Output_array = new double [4*6]();
    QSqlQuery query(db);
    query.setForwardOnly(true);

    for(int j = 1; j <= 6; j++)
    {
        QString axis_num = "axis_"+ QString::number(j);
        //Get the statistic result from the SQL query
        if(!query.exec("SELECT MIN("+axis_num+"), AVG("+axis_num+"), MAX("+axis_num+"), STDDEV("+axis_num+") FROM "+ table + " WHERE "+axis_num+">0"))
        {
            emit tableCreateError(query.lastError().text());
        }
        query.next();

        for(int i =0; i <4; i++)
        {
            //Get the query result
            double value = query.value(i).toDouble();
            StrList.append(QString::number(value, 'd', 4)+" mm");
            Output_array[(j - 1)*4 + i] = value;
        }

    }
    if(!query.exec("SELECT (axis_1+axis_2+axis_3+axis_4+axis_5+axis_6)/6 FROM " + table))
    {
        emit tableCreateError(query.lastError().text());
    }
    query.next();
    double allAvg = query.value(0).toDouble();

    for(int i = 0; i <4; i++)
    {
        double max = std::numeric_limits<double>::min();
        double min = std::numeric_limits<double>::max();
        double avg = 0;
        double ova = 0;
        for(int j = 0; j < 6; j ++)
        {
            double curr = Output_array[i + j*4];
            avg += curr;
            if(curr > max)
                max = curr;
            if(curr < min)
                min = curr;
        }
        ova = max - min;
        avg /= 6.0;
        if(i!=1)
            StrList.append(QString::number(avg, 'd' ,4) + " mm");
        else
            StrList.append(QString::number(allAvg, 'd',4) + " mm");



        StrList.append(QString::number(ova, 'd', 4) + " mm");
        StrList.append(QString::number(min, 'd', 4) + " mm");
        StrList.append(QString::number(max, 'd', 4) + " mm");
    }



    emit tableSelected_old(model, StrList);
}


void DbController::getTablesNamesRequested()
{
    emit gotTablesNames(db.tables());
}

bool DbController::connectToServerMSSQL(QString driver, QString server, int port, QString database,
                                   QString login, QString password)
{
    db.setDatabaseName(connection_string_sqlauth.arg(driver).arg(server).arg(port).arg(database)
                       .arg(login).arg(password));

    return db.open();
}

bool DbController::connectToServerMSSQL(QString driver, QString server, int port, QString database)
{
    db.setDatabaseName(connection_string_winauth.arg(driver).arg(server).arg(port).arg(database));

    return db.open();
}

bool DbController::connectToServerMySQL(QString server, int port, QString database,
                                        QString login, QString password)
{
    db.setHostName(server);
    db.setPort(port);
    db.setDatabaseName(database);
    db.setUserName(login);
    db.setPassword(password);

    return db.open();
}
bool DbController::connectToServerPostgreSQL(QString server, int port, QString database,
                                             QString login, QString password)
{
    db.setHostName(server);
    db.setPort(port);
    db.setDatabaseName(database);
    db.setUserName(login);
    db.setPassword(password);

    return db.open();
}



void DbController::disconnectFromServer()
{
    db.close();
}

QSqlQueryModel* DbController::selectTable(QString name)
{
    QSqlQueryModel* model = new QSqlQueryModel;

    model->setQuery("SELECT * FROM " + name, db);


    return model;
}

QSqlError DbController::getLastError()
{
    return db.lastError();
}

void DbController::Import6AxisCSV(QStringList CSVNames)
{
    QSqlQuery query(db);
    QElapsedTimer timer;
    int j = 0;
    timer.start();

    for(int i = 0; i<CSVNames.size(); i++ )
    {
       QString path = CSVNames.at(i);
       QString Table_Name = path.section('/',-1);
       QString Table_Name_Crop = Table_Name.section(".",0,0);
       QFile CSVFile(path);
       if(!query.exec("CREATE TABLE _" + Table_Name_Crop + "(axis_1 REAL, axis_2 REAL, axis_3 REAL, axis_4 REAL, axis_5 REAL, axis_6 REAL)"))
       {
           emit tableCreateError(query.lastError().text());
       }
       else
       {
           j++;
           if(!query.exec("COPY _" + Table_Name_Crop + " FROM '" + path + "' WITH (FORMAT csv)"))
           {
               emit tableCreateError(query.lastError().text());
           }
           else
               emit updateCurNum(i);
       }
/*Code for the line by line insert*/
//       if(!CSVFile.open(QIODevice::ReadOnly))
//       {
//           emit tableCreateError(CSVFile.errorString());
//       }
//       else
//       {
//           QTextStream stream(&CSVFile);
//           QString req = "INSERT INTO _"+Table_Name_Crop+ " VALUES ";
//           while(!stream.atEnd())
//           {
//               // split every lines on comma
//               QString line = stream.readLine();
//               /*for every values on a line,
//                            append it to the INSERT request*/
//               req.append("(");
//               req.append(line);
//               req.append("),"); // close the "VALUES([...]" with a ");"
//           }
//           req.chop(1); // remove the trailing comma
//           req.append(";");
//           if(!query.exec(req))
//           {
//               emit tableCreateError(query.lastError().text());
//           }
//           CSVFile.close();
//       }

    }
    emit updateCurNum(CSVNames.size());
    //emit csvImportMessage("Imported " + QString::number(j) + " csv files with " + QString::number(timer.elapsed()) + "ms");
    qDebug() << "The total input with took" << timer.elapsed() << "milliseconds (6-axis)";

}

void DbController::Import2AxisCSV(QStringList CSVNames)
{
    QSqlQuery query(db);
    QElapsedTimer timer;
    QString csvString;
    timer.start();
    int j = 0;
    for(int i = 0; i<CSVNames.size(); i++ )
    {
       QString path = CSVNames.at(i);
       QString Table_Name = path.section('/',-1);
       QString Table_Name_Crop = Table_Name.section(".",0,0);
       QFile CSVFile(path);
       if(!query.exec("CREATE TABLE _" + Table_Name_Crop + "(axis_1 REAL, axis_2 REAL)"))
       {
           emit tableCreateError(query.lastError().text());
       }
       else
       {
           j++;
           if(!query.exec("COPY _" + Table_Name_Crop + " FROM '" + path + "' WITH (FORMAT csv)"))
           {
               emit tableCreateError(query.lastError().text());
           }
           else
               emit updateCurNum(i);
       }
/*Code for the line by line insert*/
//       if(!CSVFile.open(QIODevice::ReadOnly))
//       {
//           emit tableCreateError(CSVFile.errorString());
//       }
//       else
//       {
//           QTextStream stream(&CSVFile);
//           QString req = "INSERT INTO _"+Table_Name_Crop+ " VALUES ";

//           while(!stream.atEnd())
//           {
//               // split every lines on comma
//               QString line = stream.readLine();
//               /*for every values on a line,
//                            append it to the INSERT request*/
//               req.append("(");
//               req.append(line);
//               req.append("),"); // close the "VALUES([...]" with a ");"
//           }
//           req.chop(1); // remove the trailing comma
//           req.append(";");
//           if(!query.exec(req))
//           {
//               emit tableCreateError(query.lastError().text());
//           }
//           CSVFile.close();
//       }
    }
    emit updateCurNum(CSVNames.size());
//    emit csvImportMessage("Imported " + QString::number(j) + " csv files with " + QString::number(timer.elapsed()) + "ms");
    qDebug() << "The total input with took" << timer.elapsed() << "milliseconds (2-axis)";

}

void DbController::deleteDBreceived()
{
    QSqlQuery query(db);
    //Delete public SCHEMA
    if(!query.exec("DROP SCHEMA public CASCADE"))
    {
        emit tableCreateError(query.lastError().text());
    }
    //Create new public SCHEMA
    if(!query.exec("CREATE SCHEMA public"))
    {
        emit tableCreateError(query.lastError().text());
    }
}




const QString DbController::connection_string_sqlauth =
        QString("DRIVER={%1};SERVER=%2;PORT=%3;DATABASE=%4;UID=%5;PWD=%6");

const QString DbController::connection_string_winauth =
        QString("DRIVER={%1};SERVER=%2;PORT=%3;DATABASE=%4");
