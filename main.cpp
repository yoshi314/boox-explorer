/*  Copyright (C) 2011-2012 OpenBOOX
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

#include <QSqlDatabase>

#include "obx_explorer.h"
#include "database_utils.h"

using namespace obx;

static bool createConnections();

int main(int argc, char *argv[])
{
    createConnections();

    ObxExplorer app(argc, argv);
    ObxExplorerAdaptor adaptor(&app);

    Q_INIT_RESOURCE(onyx_ui_images);

    return app.exec();
}

static bool createConnections()
{
    const int MAJOR_REV = 0;
    const int MINOR_REV = 3;

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(QString(qgetenv("HOME")) + "/obx_explorer.db");
    if (!db.open())
    {
        qDebug() << "Database Error";
        return false;
    }

    qDebug() << "checking database revision...";
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
			/* structures */
            << "CREATE TABLE revision (id INTEGER PRIMARY KEY, major NUMERIC, minor NUMERIC)"
            << QString("INSERT INTO revision VALUES(1, %1, %2)").arg(MAJOR_REV).arg(MINOR_REV)
            
            << "CREATE TABLE books (id INTEGER PRIMARY KEY, file TEXT, title TEXT, author TEXT, publisher TEXT, "
               "year TEXT, series TEXT, series_index NUMERIC, add_date TEXT, read_date TEXT, read_count NUMERIC, cover TEXT)"
            << "CREATE TABLE music (id INTEGER PRIMARY KEY, file TEXT, title TEXT, artist TEXT, album TEXT, "
               "album_artist TEXT, track NUMERIC, genre TEXT, year TEXT, add_date TEXT, play_date TEXT, play_count NUMERIC)"
            << "CREATE TABLE categories (id INTEGER PRIMARY KEY, position NUMERIC, visible NUMERIC, name TEXT, handler_id NUMERIC, handler_data TEXT, icon TEXT)"
            << "CREATE TABLE associations (id INTEGER PRIMARY KEY, extension TEXT, viewer NUMERIC, viewers TEXT, editor TEXT, icon TEXT, handler_id NUMERIC)"
            << "CREATE TABLE views (id INTEGER PRIMARY KEY, position NUMERIC, visible NUMERIC, name TEXT, handler_id NUMERIC, handler_data TEXT, icon TEXT)"
            << "CREATE TABLE applications (id INTEGER PRIMARY KEY, name TEXT, executable TEXT, options TEXT, icon TEXT, category_id NUMERIC)"
            << "CREATE TABLE websites (id INTEGER PRIMARY KEY, name TEXT, url TEXT, icon TEXT)"   
            /* indexes */
            << "CREATE INDEX books_i_file ON books ( file ) "
            << "CREATE INDEX books_i_rcount ON books ( read_count ) "
            << "CREATE INDEX books_i_year ON books ( year )"
            << "CREATE INDEX books_i_author ON books ( author )"
            << "CREATE INDEX books_i_pub ON books ( publisher )"
            << "CREATE INDEX books_i_series ON books ( series )"
            << "CREATE INDEX books_i_series_idx ON books ( series_index )"
            << "CREATE INDEX books_i_rdate ON books ( read_date )"
            << "CREATE INDEX books_i_adate ON books ( add_date )"
            << "CREATE INDEX applications_i_hid ON applications ( category_id )"
            ;

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

    return true;
}
