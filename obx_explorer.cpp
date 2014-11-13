/* vim: set sw=4 sts=4 et foldmethod=syntax : */
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

#include <stdlib.h>
#include <onyx/sys/sys.h>
#include <onyx/screen/screen_proxy.h>

#include "obx_explorer.h"
#include "explorer_view.h"
#include "database_utils.h"
#include "mpdclient_dialog.h"

namespace obx
{

    ObxExplorer::ObxExplorer(int &argc, char **argv)
        : QApplication(argc, argv)
          , mainUI_(QString::fromLocal8Bit(argv[0]) == "explorer")
          , view_(mainUI_)
    {
        uint seed;
        QFile seedFile(QString(qgetenv("HOME")) + "/seed");
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
        if (mainUI_)
        {
            SysStatus &sys_status = SysStatus::instance();
            connect(&sys_status, SIGNAL(musicPlayerStateChanged(int)), this, SLOT(onMusicPlayerStateChanged(int)));
            connect(&sys_status.connectionManager(),
                    SIGNAL(connectionChanged(WifiProfile, WpaConnection::ConnectionState)),
                    this,
                    SLOT(onWpaConnectionChanged(WifiProfile, WpaConnection::ConnectionState)));
        }
    }

    ObxExplorer::~ObxExplorer()
    {
    }

    void ObxExplorer::onScreenSizeChanged()
    {
        view_.resize(qApp->desktop()->screenGeometry().size());
        if (view_.isActiveWindow())
        {
            onyx::screen::instance().flush(&view_, onyx::screen::ScreenProxy::GC);
        }
    }

    void ObxExplorer::onMusicPlayerStateChanged(int state)
    {
        static int lastState = -1;

        switch (state)
        {
            case START_PLAYER:
                {
                    if (lastState >= 0)
                    {
                        sys::SysStatus::instance().reportMusicPlayerState(lastState);
                    }

                    MpdClientDialog mpdClientDialog(qApp->desktop());
                    lastState = mpdClientDialog.exec();
                    onyx::screen::instance().flush(&mpdClientDialog, onyx::screen::ScreenProxy::GC, false);
                    break;
                }
            case MUSIC_PLAYING:
            case MUSIC_STOPPED:
            case MUSIC_PAUSED:
                lastState = state;
                break;
            case STOP_PLAYER:
                lastState = -1;
                break;
            default:
                break;
        }
    }

    void ObxExplorer::onWpaConnectionChanged(WifiProfile, WpaConnection::ConnectionState state)
    {
        qDebug() << "WpaConnectionChanged" ;
        static bool wpaIdle = true;

        switch (state)
        {
            case WpaConnection::STATE_SCANNING:
                qDebug() << "WpaConnectionChanged : STATE_SCANNING" ;
                if (wpaIdle)
                {
                    qDebug() << "WpaConnectionChanged : wpaIdle" ;
                    wpaIdle = false;
                    sys::SysStatus::instance().enableIdle(false);   // Keep the wifi hardware active
                }
                break;
            case WpaConnection::STATE_COMPLETE:
                qDebug() << "WpaConnectionChanged : STATE_COMPLETE" ;
                system("/etc/rc.d/init.d/ntp start > /dev/null");
                break;
            case WpaConnection::STATE_ACQUIRING_ADDRESS_ERROR:
                qDebug() << "WpaConnectionChanged : STATE_ACQUIRING_ADDRESS_ERROR" ;
            case WpaConnection::STATE_DISCONNECTED:
                qDebug() << "WpaConnectionChanged : STATE_DISCONNECTED" ;
                system("/etc/rc.d/init.d/ntp stop > /dev/null");
                if (!wpaIdle)
                {
                    qDebug() << "WpaConnectionChanged : wpaIdle" ;
                    wpaIdle = true;
                    sys::SysStatus::instance().enableIdle(true);
                }

                // Switch off Wifi
                if (SysStatus::instance().sdioState())
                {
                    qDebug() << "WpaConnectionChanged : switching off wifi" ;
                    SysStatus::instance().enableSdio(false);
                }
                break;
            default:
                break;
        }
    }

    bool initializeDatabase()
    {
        const QStringList queries = QStringList()

            << "INSERT INTO categories VALUES(1,1,1,'Internal Flash',1,'/media/flash','/usr/share/explorer/images/middle/directory.png')"
            << "INSERT INTO categories VALUES(2,2,1,'SD Card',1,'/media/sd','/usr/share/explorer/images/middle/sd.png')"
            << "INSERT INTO categories VALUES(3,3,1,'Books',2,NULL,'/usr/share/explorer/images/middle/library.png')"
            << "INSERT INTO categories VALUES(4,4,1,'Music',5,NULL,'/usr/share/explorer/images/middle/music.png')"
            << "INSERT INTO categories VALUES(5,5,1,'Applications',3,NULL,'/usr/share/explorer/images/middle/applications.png')"
            << "INSERT INTO categories VALUES(6,6,1,'Games',3,NULL,'/usr/share/explorer/images/middle/games.png')"
            << "INSERT INTO categories VALUES(7,7,1,'Websites',4,NULL,'/usr/share/explorer/images/middle/websites.png')"
            << "INSERT INTO categories VALUES(8,8,1,'Scribbles',1,'/root/notes','/usr/share/explorer/images/middle/notes.png')"

            << "INSERT INTO associations VALUES(1,'chm',12,'12;10;11',NULL,'/usr/share/explorer/images/middle/chm.png',2)"
            << "INSERT INTO associations VALUES(2,'djvu',13,NULL,NULL,'/usr/share/explorer/images/middle/djvu.png',2)"
            << "INSERT INTO associations VALUES(3,'epub',9,'9;10;11',NULL,'/usr/share/explorer/images/middle/epub.png',2)"
            << "INSERT INTO associations VALUES(4,'mobi',10,'10;11',NULL,'/usr/share/explorer/images/middle/mobi.png',2)"
            << "INSERT INTO associations VALUES(5,'pdf',9,NULL,NULL,'/usr/share/explorer/images/middle/pdf.png',2)"
            << "INSERT INTO associations VALUES(6,'fb2',10,'10;11',NULL,'/usr/share/explorer/images/middle/fb2.png',2)"
            << "INSERT INTO associations VALUES(7,'fb2.zip',10,'10;11',NULL,'/usr/share/explorer/images/middle/fb2.png',2)"
            << "INSERT INTO associations VALUES(8,'html',14,'14;11',NULL,'/usr/share/explorer/images/middle/html.png',2)"
            << "INSERT INTO associations VALUES(9,'txt',10,'10;11','/opt/onyx/arm/bin/text_editor','/usr/share/explorer/images/middle/txt.png',2)"
            << "INSERT INTO associations VALUES(10,'rtf',10,'10;11',NULL,'/usr/share/explorer/images/middle/rtf.png',2)"
            << "INSERT INTO associations VALUES(11,'doc',15,'15;10;11',NULL,'/usr/share/explorer/images/middle/doc.png',2)"
            << "INSERT INTO associations VALUES(12,'docx',15,NULL,NULL,'/usr/share/explorer/images/middle/doc.png',2)"
            << "INSERT INTO associations VALUES(13,'cbr',16,NULL,NULL,'/usr/share/explorer/images/middle/cbr.png',2)"
            << "INSERT INTO associations VALUES(14,'cbz',16,NULL,NULL,'/usr/share/explorer/images/middle/cbz.png',2)"
            << "INSERT INTO associations VALUES(15,'cb7',16,NULL,NULL,'/usr/share/explorer/images/middle/cb7.png',2)"
            << "INSERT INTO associations VALUES(16,'xls',15,NULL,NULL,'/usr/share/explorer/images/middle/xls.png',NULL)"
            << "INSERT INTO associations VALUES(17,'xlsx',15,NULL,NULL,'/usr/share/explorer/images/middle/xls.png',NULL)"
            << "INSERT INTO associations VALUES(18,'ppt',15,NULL,NULL,'/usr/share/explorer/images/middle/ppt.png',NULL)"
            << "INSERT INTO associations VALUES(19,'pptx',15,NULL,NULL,'/usr/share/explorer/images/middle/ppt.png',NULL)"
            << "INSERT INTO associations VALUES(20,'sketch',3,NULL,NULL,'/usr/share/explorer/images/middle/note.png',NULL)"
            << "INSERT INTO associations VALUES(21,'png',17,NULL,NULL,'/usr/share/explorer/images/middle/png.png',NULL)"
            << "INSERT INTO associations VALUES(22,'jpg',17,NULL,NULL,'/usr/share/explorer/images/middle/jpg.png',NULL)"
            << "INSERT INTO associations VALUES(23,'gif',17,NULL,NULL,'/usr/share/explorer/images/middle/gif.png',NULL)"
            << "INSERT INTO associations VALUES(24,'bmp',17,NULL,NULL,'/usr/share/explorer/images/middle/bmp.png',NULL)"
            << "INSERT INTO associations VALUES(25,'tif',17,NULL,NULL,'/usr/share/explorer/images/middle/tiff.png',NULL)"
            << "INSERT INTO associations VALUES(26,'tiff',17,NULL,NULL,'/usr/share/explorer/images/middle/tiff.png',NULL)"
            << "INSERT INTO associations VALUES(27,'mp3',NULL,NULL,NULL,'/usr/share/explorer/images/middle/mp3.png',5)"
            << "INSERT INTO associations VALUES(28,'flac',NULL,NULL,NULL,'/usr/share/explorer/images/middle/flac.png',5)"

            << "INSERT INTO views VALUES(1,1,1,'Recently Read',2,'SELECT DISTINCT file, title, author, read_count, series_index, series FROM books WHERE read_date > :week_ago ORDER BY read_date DESC, upper(title), title','/usr/share/explorer/images/middle/book.png')"
            << "INSERT INTO views VALUES(2,2,1,'New',2,'SELECT DISTINCT file, title, author, read_count, series_index, series FROM books WHERE add_date > :week_ago ORDER BY add_date DESC, upper(title), title','/usr/share/explorer/images/middle/new.png')"
            << "INSERT INTO views VALUES(3,3,1,'By Title',2,'SELECT DISTINCT file, title, author,read_count, series_index, series  FROM books ORDER BY upper(title), title','/usr/share/explorer/images/middle/dictionary.png')"
            << "INSERT INTO views VALUES(4,4,1,'By Author',2,'SELECT DISTINCT author FROM books ORDER BY upper(author), author;SELECT DISTINCT file, title, author,read_count,series,series_index FROM books WHERE author = :author ORDER BY upper(title), title','/usr/share/explorer/images/middle/author.png')"
            << "INSERT INTO views VALUES(5,5,1,'By Publisher',2,'SELECT DISTINCT publisher FROM books ORDER BY upper(publisher), publisher;SELECT DISTINCT file, title, author, read_count, series_index, series FROM books WHERE publisher = :publisher ORDER BY upper(title), title','/usr/share/explorer/images/middle/publisher.png')"
            << "INSERT INTO views VALUES(6,6,1,'By Year',2,'SELECT DISTINCT year FROM books ORDER BY year;SELECT DISTINCT file, title, author, read_count, series_index, series  FROM books WHERE year = :year ORDER BY upper(title), title','/usr/share/explorer/images/middle/calendar.png')"
            << "INSERT INTO views VALUES(7,7,1,'By Series',2,'SELECT DISTINCT series,max(read_date) as series_read_date,sum(read_count) as series_read_count, count(*) as total_books, (select count(*) from books c where c.series=b.series and read_count = 0) as series_unread, (select count(*) from books c where c.series=b.series and read_count > 0) as series_read FROM books b WHERE series <> '''' group by upper(series) ORDER BY read_date desc, upper(series), series;SELECT DISTINCT file, title, author,read_count, series, series_index FROM books WHERE series = :series ORDER BY series_index','/usr/share/explorer/images/middle/series.png')"

            << "INSERT INTO views VALUES(8,8,1,'Unread',2,'SELECT DISTINCT file, title, author, series_index, series FROM books WHERE read_count = 0 ORDER BY upper(title), title','/usr/share/explorer/images/middle/unread.png')"

            << "INSERT INTO views VALUES(9,1,1,'Favorites',5,'SELECT DISTINCT file, title, artist FROM music WHERE play_count > 0 ORDER BY play_count DESC, upper(title), title','/usr/share/explorer/images/middle/favorites.png')"
            << "INSERT INTO views VALUES(10,2,1,'By Title',5,'SELECT DISTINCT file, title, artist FROM music ORDER BY upper(title), title','/usr/share/explorer/images/middle/title.png')"
            << "INSERT INTO views VALUES(11,3,1,'By Artist',5,'SELECT DISTINCT artist FROM music ORDER BY upper(artist), artist;SELECT DISTINCT file, title, artist FROM music WHERE artist = :artist ORDER BY upper(title), title','/usr/share/explorer/images/middle/artist.png')"
            << "INSERT INTO views VALUES(12,4,1,'By Album',5,'SELECT DISTINCT album FROM music ORDER BY upper(album), album;SELECT DISTINCT file, title, artist FROM music WHERE album = :album ORDER BY track','/usr/share/explorer/images/middle/album.png')"
            << "INSERT INTO views VALUES(13,5,1,'By Year',5,'SELECT DISTINCT year FROM music ORDER BY year;SELECT DISTINCT file, title, artist FROM music WHERE year = :year ORDER BY upper(title), title','/usr/share/explorer/images/middle/calendar.png')"
            << "INSERT INTO views VALUES(14,6,1,'By Genre',5,'SELECT DISTINCT genre FROM music ORDER BY upper(genre), genre;SELECT DISTINCT file, title, artist FROM music WHERE genre = :genre ORDER BY title','/usr/share/explorer/images/middle/genre.png')"
            << "INSERT INTO views VALUES(15,7,1,'Undiscovered',5,'SELECT DISTINCT file, title, artist FROM music WHERE play_count = 0 ORDER BY upper(title), title','/usr/share/explorer/images/middle/undiscovered.png')"

            << "INSERT INTO applications VALUES(1,'Dictionary','/opt/onyx/arm/bin/dict_tool',NULL,'/usr/share/explorer/images/middle/dictionary.png',5)"
            << "INSERT INTO applications VALUES(2,'Text Editor','/opt/onyx/arm/bin/text_editor',NULL,'/usr/share/explorer/images/middle/write_pad.png',5)"
            << "INSERT INTO applications VALUES(3,'Scribbler','/opt/onyx/arm/bin/scribbler',NULL,'/usr/share/explorer/images/middle/note.png',5)"
            << "INSERT INTO applications VALUES(4,'Calendar','/opt/onyx/arm/bin/calendar',NULL,'/usr/share/explorer/images/middle/calendar.png',5)"
            << "INSERT INTO applications VALUES(5,'Calculator','/opt/onyx/arm/bin/calculator',NULL,'/usr/share/explorer/images/middle/calculator.png',5)"
            << "INSERT INTO applications VALUES(6,'Clock','/opt/onyx/arm/bin/clock',NULL,'/usr/share/explorer/images/middle/full_screen_clock.png',5)"
            << "INSERT INTO applications VALUES(7,'Sudoku','/opt/onyx/arm/bin/sudoku',NULL,'/usr/share/explorer/images/middle/suduko.png',6)"
            << "INSERT INTO applications VALUES(8,'Gomoku','/opt/onyx/arm/bin/gomoku',NULL,'/usr/share/explorer/images/middle/gomoku.png',6)"
            << "INSERT INTO applications VALUES(9,'PdfReader','/opt/onyx/arm/bin/naboo_reader',NULL,NULL,NULL)"
            << "INSERT INTO applications VALUES(10,'FBReader','/opt/onyx/arm/bin/onyx_reader',NULL,NULL,NULL)"
            << "INSERT INTO applications VALUES(11,'CoolReader','/opt/onyx/arm/bin/cr3',NULL,NULL,NULL)"
            << "INSERT INTO applications VALUES(12,'HtmlReader','/opt/onyx/arm/bin/html_reader',NULL,NULL,NULL)"
            << "INSERT INTO applications VALUES(13,'DjvuReader','/opt/onyx/arm/bin/djvu_reader',NULL,NULL,NULL)"
            << "INSERT INTO applications VALUES(14,'WebBrowser','/opt/onyx/arm/bin/web_browser',NULL,NULL,NULL)"
            << "INSERT INTO applications VALUES(15,'OfficeViewer','/opt/onyx/arm/bin/office_viewer',NULL,NULL,NULL)"
            << "INSERT INTO applications VALUES(16,'ComicReader','/opt/onyx/arm/bin/comic_reader',NULL,NULL,NULL)"
            << "INSERT INTO applications VALUES(17,'ImageReader','/opt/onyx/arm/bin/image_reader',NULL,NULL,NULL)"
            << "INSERT INTO applications VALUES(17,'RSS Reader','/opt/onyx/arm/bin/rss_reader',NULL,NULL,NULL)"

            << "INSERT INTO websites VALUES(1,'Google','http://www.google.com','/usr/share/explorer/images/middle/google.png')"
            << "INSERT INTO websites VALUES(2,'Wikipedia','http://en.wikipedia.org','/usr/share/explorer/images/middle/wiki.png')"

            << "INSERT INTO settings VALUES('loadlast','N','Y/N - open last read book on startup?')" ;

        return (DatabaseUtils::execQueries(queries) == queries.size());
    }

}
