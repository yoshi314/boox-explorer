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

#include "obx_explorer.h"
#include "explorer_view.h"
#include "database_utils.h"

#include "onyx/sys/sys.h"
#include "onyx/screen/screen_proxy.h"

namespace obx
{

ObxExplorer::ObxExplorer(int &argc, char **argv)
    : QApplication(argc, argv)
    , view_(argc > 1 && QString::fromLocal8Bit(argv[1]) == "--mainui")
{
    uint seed;
    QFile seedFile("/root/seed");
    if (seedFile.open(QIODevice::ReadOnly))
    {
        seed = QString(seedFile.readLine()).toUInt();
        seedFile.close();
    }
    else
    {
        bool ok;
        WpaConnectionManager manager;
        seed = manager.hardwareAddress().remove(':').remove(0, 4).toInt(&ok, 16);
    }
    qsrand(seed);

    connect(qApp->desktop(), SIGNAL(resized(int)), this, SLOT(onScreenSizeChanged()), Qt::QueuedConnection);

    view_.show();
    sys::SysStatus::instance().setSystemBusy(false);
    onyx::screen::instance().enableUpdate(true);
    onyx::screen::instance().flush(&view_, onyx::screen::ScreenProxy::GC);
}

ObxExplorer::~ObxExplorer()
{
}

void ObxExplorer::onScreenSizeChanged()
{
    QSize size = qApp->desktop()->screenGeometry().size();

//    qDebug() << "Rotation:" << SysStatus::instance().screenTransformation();
//    qDebug() << "Dimensions:" << size.width() << "x" << size.height();

    view_.resize(size);
    if (view_.isActiveWindow())
    {
        onyx::screen::instance().flush(&view_, onyx::screen::ScreenProxy::GC);
    }
}

bool initializeDatabase()
{
    const QStringList queries = QStringList()
        << "CREATE TABLE categories (id INTEGER PRIMARY KEY, position NUMERIC, visible NUMERIC, name TEXT, handler_id NUMERIC, handler_data TEXT, icon TEXT)"
        << "INSERT INTO categories VALUES(1,1,1,'Internal Flash',1,'/media/flash','/usr/share/explorer/images/middle/directory.png')"
        << "INSERT INTO categories VALUES(2,2,1,'SD Card',1,'/media/sd','/usr/share/explorer/images/middle/sd.png')"
        << "INSERT INTO categories VALUES(3,3,1,'Books',2,NULL,'/usr/share/explorer/images/middle/library.png')"
        << "INSERT INTO categories VALUES(4,4,1,'Applications',3,NULL,'/usr/share/explorer/images/middle/applications.png')"
        << "INSERT INTO categories VALUES(5,5,1,'Games',3,NULL,'/usr/share/explorer/images/middle/games.png')"
        << "INSERT INTO categories VALUES(6,6,1,'Websites',4,NULL,'/usr/share/explorer/images/middle/websites.png')"
        << "INSERT INTO categories VALUES(7,7,1,'Scribbles',1,'/root/notes','/usr/share/explorer/images/middle/notes.png')"
        << "CREATE TABLE associations (id INTEGER PRIMARY KEY, extension TEXT, viewer TEXT, editor TEXT, icon TEXT, handler_id NUMERIC)"
        << "INSERT INTO associations VALUES(1,'chm','/opt/onyx/arm/bin/html_reader',NULL,'/usr/share/explorer/images/middle/chm.png',2)"
        << "INSERT INTO associations VALUES(2,'djvu','/opt/onyx/arm/bin/djvu_reader',NULL,'/usr/share/explorer/images/middle/djvu.png',2)"
        << "INSERT INTO associations VALUES(3,'epub','/opt/onyx/arm/bin/naboo_reader',NULL,'/usr/share/explorer/images/middle/epub.png',2)"
        << "INSERT INTO associations VALUES(4,'mobi','/opt/onyx/arm/bin/onyx_reader',NULL,'/usr/share/explorer/images/middle/mobi.png',2)"
        << "INSERT INTO associations VALUES(5,'pdf','/opt/onyx/arm/bin/naboo_reader',NULL,'/usr/share/explorer/images/middle/pdf.png',2)"
        << "INSERT INTO associations VALUES(6,'fb2','/opt/onyx/arm/bin/onyx_reader',NULL,'/usr/share/explorer/images/middle/fb2.png',2)"
        << "INSERT INTO associations VALUES(7,'html','/opt/onyx/arm/bin/web_browser',NULL,'/usr/share/explorer/images/middle/html.png',2)"
        << "INSERT INTO associations VALUES(8,'txt','/opt/onyx/arm/bin/onyx_reader','/opt/onyx/arm/bin/text_editor','/usr/share/explorer/images/middle/txt.png',2)"
        << "INSERT INTO associations VALUES(9,'rtf','/opt/onyx/arm/bin/onyx_reader',NULL,'/usr/share/explorer/images/middle/rtf.png',2)"
        << "INSERT INTO associations VALUES(10,'doc','/opt/onyx/arm/bin/office_viewer',NULL,'/usr/share/explorer/images/middle/doc.png',2)"
        << "INSERT INTO associations VALUES(11,'docx','/opt/onyx/arm/bin/office_viewer',NULL,'/usr/share/explorer/images/middle/doc.png',2)"
        << "INSERT INTO associations VALUES(12,'xls','/opt/onyx/arm/bin/office_viewer',NULL,'/usr/share/explorer/images/middle/xls.png',NULL)"
        << "INSERT INTO associations VALUES(13,'xlsx','/opt/onyx/arm/bin/office_viewer',NULL,'/usr/share/explorer/images/middle/xls.png',NULL)"
        << "INSERT INTO associations VALUES(14,'sketch','/opt/onyx/arm/bin/scribbler',NULL,'/usr/share/explorer/images/middle/note.png',NULL)"
        << "INSERT INTO associations VALUES(15,'png','/opt/onyx/arm/bin/image_reader',NULL,'/usr/share/explorer/images/middle/png.png',NULL)"
        << "INSERT INTO associations VALUES(16,'jpg','/opt/onyx/arm/bin/image_reader',NULL,'/usr/share/explorer/images/middle/jpg.png',NULL)"
        << "INSERT INTO associations VALUES(17,'gif','/opt/onyx/arm/bin/image_reader',NULL,'/usr/share/explorer/images/middle/gif.png',NULL)"
        << "INSERT INTO associations VALUES(18,'bmp','/opt/onyx/arm/bin/image_reader',NULL,'/usr/share/explorer/images/middle/bmp.png',NULL)"
        << "INSERT INTO associations VALUES(19,'tif','/opt/onyx/arm/bin/image_reader',NULL,'/usr/share/explorer/images/middle/tiff.png',NULL)"
        << "CREATE TABLE book_views (id INTEGER PRIMARY KEY, position NUMERIC, visible NUMERIC, name TEXT, handler_id NUMERIC, handler_data TEXT, icon TEXT)"
        << "INSERT INTO book_views VALUES(1,1,1,'New',NULL,'SELECT DISTINCT file, title, author FROM books WHERE add_date > :week_ago ORDER BY add_date DESC, upper(title), title','/usr/share/explorer/images/middle/new.png')"
        << "INSERT INTO book_views VALUES(2,2,1,'Recently Read',NULL,'SELECT DISTINCT file, title, author FROM books WHERE read_date > :week_ago ORDER BY read_date DESC, upper(title), title','/usr/share/explorer/images/middle/book.png')"
        << "INSERT INTO book_views VALUES(3,3,1,'By Title',NULL,'SELECT DISTINCT file, title, author FROM books ORDER BY upper(title), title','/usr/share/explorer/images/middle/dictionary.png')"
        << "INSERT INTO book_views VALUES(4,4,1,'By Author',NULL,'SELECT DISTINCT author FROM books ORDER BY upper(author), author;SELECT DISTINCT file, title, author FROM books WHERE author = :author ORDER BY upper(title), title','/usr/share/explorer/images/middle/author.png')"
        << "INSERT INTO book_views VALUES(5,5,1,'By Publisher',NULL,'SELECT DISTINCT publisher FROM books ORDER BY upper(publisher), publisher;SELECT DISTINCT file, title, author FROM books WHERE publisher = :publisher ORDER BY upper(title), title','/usr/share/explorer/images/middle/publisher.png')"
        << "INSERT INTO book_views VALUES(6,6,1,'By Year',NULL,'SELECT DISTINCT year FROM books ORDER BY year;SELECT DISTINCT file, title, author FROM books WHERE year = :year ORDER BY upper(title), title','/usr/share/explorer/images/middle/calendar.png')"
        << "INSERT INTO book_views VALUES(7,7,1,'By Series',NULL,'SELECT DISTINCT series FROM books WHERE series <> '''' ORDER BY upper(series), series;SELECT DISTINCT file, title, author FROM books WHERE series = :series ORDER BY series_index','/usr/share/explorer/images/middle/series.png')"
        << "INSERT INTO book_views VALUES(8,8,1,'Unread',NULL,'SELECT DISTINCT file, title, author FROM books WHERE read_count = 0 ORDER BY upper(title), title','/usr/share/explorer/images/middle/unread.png')"
        << "CREATE TABLE applications (id INTEGER PRIMARY KEY, name TEXT, executable TEXT, options TEXT, icon TEXT, category_id NUMERIC)"
        << "INSERT INTO applications VALUES(1,'Sudoku','/opt/onyx/arm/bin/sudoku',NULL,'/usr/share/explorer/images/middle/suduko.png',5)"
        << "INSERT INTO applications VALUES(2,'Dictionary','/opt/onyx/arm/bin/dict_tool',NULL,'/usr/share/explorer/images/middle/dictionary.png',4)"
        << "INSERT INTO applications VALUES(3,'Text Editor','/opt/onyx/arm/bin/text_editor',NULL,'/usr/share/explorer/images/middle/write_pad.png',4)"
        << "INSERT INTO applications VALUES(4,'Scribbler','/opt/onyx/arm/bin/scribbler',NULL,'/usr/share/explorer/images/middle/note.png',4)"
        << "INSERT INTO applications VALUES(5,'Onyx Explorer','/opt/onyx/arm/bin/onyx_explorer',NULL,'/usr/share/explorer/images/middle/onyx.png',4)"
        << "CREATE TABLE websites (id INTEGER PRIMARY KEY, name TEXT, url TEXT, icon TEXT)"
        << "INSERT INTO websites VALUES(1,'Google','http://www.google.com','/usr/share/explorer/images/middle/google.png')"
        << "INSERT INTO websites VALUES(2,'Wikipedia','http://en.wikipedia.org','/usr/share/explorer/images/middle/wiki.png')";

    return (DatabaseUtils::execQueries(queries) == queries.size());
}

}
