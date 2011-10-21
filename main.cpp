/*  Copyright (C) 2011  OpenBOOX
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <QtSql/QSqlDatabase>

#include "obx_explorer.h"
#include "database_utils.h"

using namespace obx;

static bool createConnections();

int main(int argc, char *argv[])
{
    createConnections();

    ObxExplorer app(argc, argv);

    Q_INIT_RESOURCE(onyx_ui_images);

    return app.exec();
}

static bool createConnections()
{
    const int MAJOR_REV = 0;
    const int MINOR_REV = 1;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("/root/obx_explorer.db");
    if (!db.open())
    {
        qDebug() << "Database Error";
        return false;
    }

    QSqlQuery query("SELECT major, minor FROM revision WHERE id = 1");

    if (!query.next() || query.value(0).toInt() != MAJOR_REV || query.value(1).toInt() != MINOR_REV)
    {
        query.finish();

        if (db.tables().size())
        {
            qDebug() << "clearing database...";
            if (!DatabaseUtils::clearDatabase())
            {
                qDebug() << "...clearing failed!";
                return false;
            }
        }

        qDebug() << "initializing database...";

        const QStringList queries = QStringList()
            << "CREATE TABLE revision (id INTEGER PRIMARY KEY, major NUMERIC, minor NUMERIC)"
            << QString("INSERT INTO revision VALUES(1,%1,%2)").arg(MAJOR_REV).arg(MINOR_REV)
            << "CREATE TABLE books (id INTEGER PRIMARY KEY, file TEXT, title TEXT, author TEXT, publisher TEXT,"
               "year TEXT, series TEXT, series_index NUMERIC, add_date TEXT, read_date TEXT, read_count NUMERIC, cover TEXT)";

        if (DatabaseUtils::execQueries(queries) != queries.size())
        {
            qDebug() << "...initialization failed!";
            return false;
        }

        if (!obx::initializeDatabase())
        {
            qDebug() << "...initialization failed!";
            return false;
        }
    }

    db = QSqlDatabase::addDatabase("QSQLITE", "ONYX");
    db.setDatabaseName("/root/content.db");
    if (!db.open())
    {
        qDebug() << "Database Error";
        return false;
    }

    return true;
}
