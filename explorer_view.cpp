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

#include <onyx/sys/sys.h>
#include <onyx/screen/screen_proxy.h>
#include <onyx/ui/status_bar.h>
#include <onyx/ui/menu.h>
#include <onyx/ui/onyx_keyboard_dialog.h>
#include <onyx/ui/time_zone_dialog.h>
#include <onyx/ui/legacy_power_management_dialog.h>
#include <onyx/ui/screen_rotation_dialog.h>
#include <onyx/wireless/wifi_dialog.h>

#include "explorer_view.h"
#include "about_dialog.h"
#include "obx_explorer.h"
#include "file_system_utils.h"
#include "database_utils.h"
#include "book_scanner.h"
#include "mpdclient_dialog.h"
#include "association_dialog.h"

namespace obx
{

enum
{
    FILE_EDIT = 0,
    FILE_RENAME,
    FILE_DELETE,
    FILE_CUT,
    FILE_COPY,
    FILE_PASTE
};

enum
{
    ORG_CATEGORIES = 0,
    ORG_SCANFLASH,
    ORG_SCANSD,
    ORG_SCANPATH,
    ORG_SCANMUSICFLASH,
    ORG_SCANMUSICSD,
    ORG_ADD2APPS,
    ORG_ADD2GAMES,
    ORG_ADD2PLAYLIST,
    ORG_ADDWEBSITE,
    ORG_ADDWEBSITEICON,
    ORG_REMOVEBOOK,
    ORG_REMOVEALLBOOKS,
    ORG_REMOVEAPP,
    ORG_REMOVEWEBSITE,
    ORG_DBCLEANUP
};

enum
{
    SET_TZ = 0,
    SET_PWR_MGMT,
    SET_CALIBRATE,
    SET_DEFAULTS,
    SET_ABOUT, 
    SET_RESUME_LAST
};

ExplorerView::ExplorerView(bool mainUI, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint)
    , waveform_(onyx::screen::ScreenProxy::GC)
    , vbox_(this)
    , model_(0)
    , treeview_(0, 0)
    , status_bar_(this, MENU | PROGRESS | MUSIC_PLAYER | CONNECTION | CLOCK | BATTERY)
    , mainUI_(mainUI)
    , homeSkipped_(false)
    , handler_type_(HDLR_HOME)
    , category_id_(0)
    , level_(0)
    , organize_mode_(false)
    , fileClipboard_()
    , process_(0)
{
    SysStatus &sys_status = SysStatus::instance();
    QMpdClient &mpdClient = QMpdClient::instance();

    resize(qApp->desktop()->screenGeometry().size());

    connect(&treeview_, SIGNAL(activated(const QModelIndex &)),
            this, SLOT(onItemActivated(const QModelIndex &)));
    connect(&treeview_, SIGNAL(positionChanged(int, int)),
            this, SLOT(onPositionChanged(int, int)));
    connect(&status_bar_, SIGNAL(menuClicked()),
            this, SLOT(popupMenu()));
    connect(&status_bar_, SIGNAL(progressClicked(int, int)),
            this, SLOT(onProgressClicked(int, int)));
    connect(&process_, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(onProcessFinished(int, QProcess::ExitStatus)));
    connect(&mpdClient, SIGNAL(databaseUpdated(bool)),
            this, SLOT(onDatabaseUpdated(bool)));
    connect(&mpdClient, SIGNAL(playlistChanged()),
            this, SLOT(onPlaylistChanged()));
    connect(&mpdClient, SIGNAL(stateChanged(QMpdStatus::State)),
            this, SLOT(onStateChanged(QMpdStatus::State)));
    connect(&mpdClient, SIGNAL(songChanged(const QMpdSong &)),
            this, SLOT(onSongChanged(const QMpdSong &)));

    connect(&sys_status, SIGNAL(wakeup()), this, SLOT(onWakeup()));
    connect(&sys_status, SIGNAL(forceQuit()), this, SLOT(onForceQuit()));
    if (mainUI)
    {
        connect(&sys_status, SIGNAL(volumeChanged(int, bool)), this, SLOT(onVolumeChanged(int, bool)));
        connect(&sys_status, SIGNAL(aboutToSuspend()), this, SLOT(onAboutToSuspend()));
        connect(&sys_status, SIGNAL(aboutToShutdown()), this, SLOT(onAboutToShutDown()));

        status_bar_.showItem(CONNECTION, false);

        // Switch off Wifi
        if (sys_status.sdioState())
        {
            sys_status.enableSdio(false);
        }
    }

    mpdClient.connectToServer(QString(), 0, 1000);
    onVolumeChanged(sys_status.volume(), false);

    QSqlQuery query;
    query.exec(QString("SELECT extension FROM associations WHERE handler_id = %1").arg(HDLR_BOOKS));
    while (query.next())
    {
        book_extensions_ << query.value(0).toString();
    }
    query.finish();

    icon_extensions_ << "png" << "jpg";

    treeview_.showHeader(true);
    treeview_.setHovering(true);

    vbox_.setSpacing(0);
    vbox_.setContentsMargins(0, 0, 0, 0);
    vbox_.addWidget(&treeview_);
    vbox_.addWidget(&status_bar_);

    show();
    sys::SysStatus::instance().setSystemBusy(false);
    onyx::screen::instance().enableUpdate(true);

    showHome(0);
}

ExplorerView::~ExplorerView()
{
}

void ExplorerView::showHome(const int &row)
{
	qDebug(" >>> ExplorerView::showHome");
    category_id_ = 0;

    QSqlQuery query("SELECT name, icon, id, handler_id, handler_data FROM categories "
                    "WHERE name <> '' AND visible = 1 ORDER BY position");

    if (query.first() && !query.next())
    {
        homeSkipped_ = true;
        query.first();

        onCategoryActivated(QStringList() << query.value(0).toString() << query.value(2).toString() <<
                            query.value(3).toString() << query.value(4).toString());
    }
    else
    {
        homeSkipped_ = false;
        query.seek(-1);
        showCategories("/", query, row);
    }
    qDebug(" <<< ExplorerView::showHome");
}

void ExplorerView::showCategories(const QString &name, QSqlQuery &query, int row)
{
	qDebug(" <<< ExplorerView::showCategories");
    int i = 0;
    QFont itemFont;
    itemFont.setPointSize(22);

    model_.clear();
    model_.setColumnCount(1);

    while (query.next())
    {
        if (handler_type_ == HDLR_BOOKS || handler_type_ == HDLR_MUSIC)
        {
            QString subQueryString = query.value(4).toString().section(';', 0, 0);
            QSqlQuery subQuery = runDatabaseQuery(subQueryString);
            if (!subQuery.next())
            {
                continue;           // Skip empty view
            }
        }
        QStandardItem *item = new QStandardItem();
        item->setText(query.value(0).toString());
        item->setIcon(QIcon(query.value(1).toString()));
        item->setData(QStringList() << query.value(2).toString() << query.value(3).toString() << query.value(4).toString());
        item->setEditable(false);
        item->setFont(itemFont);
        item->setTextAlignment(Qt::AlignLeft);
        model_.setItem(i++, 0, item);
    }
    model_.setHeaderData(0, Qt::Horizontal, name, Qt::DisplayRole);

    displayView(row);
    
    qDebug(" >>> ExplorerView::showCategories");
}

void ExplorerView::showFiles(int category, const QString &path, int row)
{
	qDebug(" <<< ExplorerView::showFiles");
    QFont itemFont;
    itemFont.setPointSize(20);

    category_id_ = category;

    QString dispPath = path;
    QSqlQuery query;
    query.exec(QString("SELECT name FROM categories WHERE id = %1").arg(category));
    if (query.next())
    {
        dispPath.replace(root_path_, "/" + query.value(0).toString());
    }
    query.finish();

    qDebug() << "current directory:" << path;

    model_.clear();
    model_.setColumnCount(1);

    QDir dir(path);
    dir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    dir.setSorting(QDir::Name | QDir::DirsFirst | QDir::IgnoreCase);

    QFileInfoList list = dir.entryInfoList();
    FileClipboard fileClipboard;
    for (int i = 0; i < list.size(); ++i)
    {
        QFileInfo fileInfo = list.at(i);
        QStandardItem *item = new QStandardItem();
        item->setText(fileInfo.fileName());
        item->setIcon(QIcon(getIconByExtension(fileInfo)));
        QBitArray properties(2);
        properties[0] = fileInfo.isDir();
        properties[1] = FileSystemUtils::isRunnable(fileInfo);
        item->setData(properties);
        item->setSelectable(!(fileClipboard.isClippedToMove() && fileInfo == fileClipboard.fileInfo()));
        item->setEditable(false);
        item->setFont(itemFont);
        item->setTextAlignment(Qt::AlignLeft);
        model_.setItem(i, 0, item);
    }
    model_.setHeaderData(0, Qt::Horizontal, dispPath, Qt::DisplayRole);

    displayView(row);
    qDebug(" >>> ExplorerView::showFiles");
}

void ExplorerView::showDBViews(int category, const QString &path, int row, const QString &pathPrefix)
{
	qDebug(" >>> ExplorerView::showDBViews");
    int level = level_ - int(!homeSkipped_);

    category_id_ = category;

    if (level == 0)
    {
        QSqlQuery query(QString("SELECT name, icon, id, handler_id, handler_data FROM views "
                                "WHERE name <> '' AND visible = 1 AND handler_id = %1 ORDER BY position")
                                .arg(handler_type_));
        showCategories(path, query, row);
        return;
    }

    QString queryString = query_list_.at(level - 1);
    QString bindString = path.section('/', -1);
    QSqlQuery query = runDatabaseQuery(queryString, bindString);
    bool leafReached = (level == query_list_.size());
    int i = 0;
    QFont itemFont;
    itemFont.setPointSize(20);

    model_.clear();
    model_.setColumnCount(1);

    while (query.next())
    {
        if (!leafReached)
        {
            QStandardItem *item = new QStandardItem();
            item->setText(query.value(0).toString());
            item->setIcon(view_icon_);
            item->setEditable(false);
            item->setFont(itemFont);
            item->setTextAlignment(Qt::AlignLeft);
            model_.setItem(i, 0, item);
            i++;
        }
        else
        {
            QString fullFileName = query.value(0).toString();
            QFileInfo fileInfo(pathPrefix + fullFileName);
            QStandardItem *item = new QStandardItem();
            item->setText(query.value(1).toString());
            item->setIcon(QIcon(getIconByExtension(fileInfo)));
            item->setData(fullFileName);

			QString tooltip("");
			QSqlRecord rec = query.record();
			
			//figure out where each column is at; if it exists
			int authorCol = rec.indexOf("author");
			int seriesCol = rec.indexOf("series");
			int seriesIdxCol = rec.indexOf("series_index");
			int readCountCol = rec.indexOf("read_count");
			
			//if we have series info
			if (seriesCol > 0) {
				//if we also have index in the series
				if (seriesIdxCol > 0) {
					tooltip += QString("Book " 
						+ query.value(seriesIdxCol).toString()
						+ " in " 
						+ query.value(seriesCol).toString());
				} else {
					tooltip += QString("Series: "
						+ query.value(seriesCol).toString());
				}
			}
			//if we have author info
			if (authorCol > 0) 
				tooltip += QString(" By: " + query.value(authorCol).toString());
				
			if (readCountCol > 0) 
				tooltip += QString(" " + query.value(readCountCol).toString() + " reads");
			
			item->setToolTip(tooltip);
			
			/*
			//add author if not empty
            if (!query.value(2).toString().isEmpty())
            
   				//SELECT DISTINCT file, title, author, read_count, series_index, series 
				//FROM books 
				//WHERE read_date > :week_ago 
				//ORDER BY read_date DESC
                //author data 
            {
				if (!query.value(3).toString().isEmpty()) {  //add read count, if nonzero
					item->setToolTip(query.value(3).toString() + " reads ; By: " + query.value(2).toString());
				}
				else {
					item->setToolTip("By: " + query.value(2).toString());
				}
            }
            */
            item->setSelectable(fileInfo.exists());
            item->setEditable(false);
            item->setFont(itemFont);
            item->setTextAlignment(Qt::AlignLeft);
            model_.setItem(i, 0, item);
            i++;
        }
    }
    model_.setHeaderData(0, Qt::Horizontal, path, Qt::DisplayRole);

    displayView(row);
    qDebug(" <<< ExplorerView::showDBViews");
}

void ExplorerView::showApps(int category, const QString &name, int row)
{
	qDebug(" <<< ExplorerView::showApps");
    int i = 0;
    QFont itemFont;
    itemFont.setPointSize(22);

    category_id_ = category;

    QSqlQuery query(QString("SELECT DISTINCT name, icon, executable, category_id FROM applications "
                            "WHERE category_id = %1 ORDER BY upper(name), name").arg(category));

    model_.clear();
    model_.setColumnCount(1);

    while (query.next())
    {
        if (QFile::exists(query.value(2).toString()))
        {
            QStandardItem *item = new QStandardItem();
            item->setText(query.value(0).toString());
            if (QFile::exists(query.value(1).toString()))
            {
                item->setIcon(QIcon(query.value(1).toString()));
            }
            else
            {
                if (category == 3)
                {
                    item->setIcon(QIcon("/usr/share/explorer/images/middle/games.png"));
                }
                else
                {
                    item->setIcon(QIcon("/usr/share/explorer/images/middle/runnable_file.png"));
                }
            }
            item->setData(query.value(2).toString());
            item->setEditable(false);
            item->setFont(itemFont);
            item->setTextAlignment(Qt::AlignLeft);
            model_.setItem(i, 0, item);
            i++;
        }
    }
    model_.setHeaderData(0, Qt::Horizontal, name, Qt::DisplayRole);

    displayView(row);
    qDebug(" >>> ExplorerView::showApps");
}

void ExplorerView::showWebsites(int category, const QString &name, int row)
{
	qDebug(" <<< ExplorerView::showWebsites");
    int i = 0;
    QFont itemFont;
    itemFont.setPointSize(22);

    category_id_ = category;

    QSqlQuery query("SELECT DISTINCT name, icon, url FROM websites WHERE name <> '' ORDER BY upper(name), name");

    model_.clear();
    model_.setColumnCount(1);

    while (query.next())
    {
        QStandardItem *item = new QStandardItem();
        item->setText(query.value(0).toString());
        if (QFile::exists(query.value(1).toString()))
        {
            item->setIcon(QIcon(query.value(1).toString()));
        }
        else
        {
            item->setIcon(QIcon("/usr/share/explorer/images/middle/websites.png"));
        }
        item->setData(query.value(2).toStringList());
        item->setEditable(false);
        item->setFont(itemFont);
        item->setTextAlignment(Qt::AlignLeft);
        model_.setItem(i, 0, item);
        i++;
    }
    model_.setHeaderData(0, Qt::Horizontal, name, Qt::DisplayRole);

    displayView(row);
    qDebug(" >>> ExplorerView::showWebsites");
}

void ExplorerView::organizeCategories(int row)
{
	qDebug(" <<< ExplorerView::organizeCategories");
    int i = 0;
    QFont itemFont;
    itemFont.setPointSize(22);

    organize_mode_ = true;

    QSqlQuery query("SELECT name, icon, id, position, visible FROM categories "
                    "WHERE name <> '' ORDER BY position");

    model_.clear();
    model_.setColumnCount(1);

    while (query.next())
    {
        QStandardItem *item = new QStandardItem();
        item->setText(query.value(0).toString());
        item->setIcon(QIcon(query.value(1).toString()));
        item->setData(QStringList() << query.value(2).toString() << query.value(3).toString());
        item->setEditable(false);
        item->setSelectable(query.value(4).toBool());
        item->setFont(itemFont);
        item->setTextAlignment(Qt::AlignLeft);
        model_.setItem(i++, 0, item);
    }
    model_.setHeaderData(0, Qt::Horizontal, "Organize Home Screen", Qt::DisplayRole);

    displayView(row);
    qDebug(" >>> ExplorerView::organizeCategories");
}

void ExplorerView::displayView(int row)
{
	qDebug(" <<< ExplorerView::displayView");
    treeview_.clear();
    if (model_.rowCount())
    {
        treeview_.hide();
    }
    treeview_.update();
    treeview_.setModel(&model_);
    treeview_.show();

    if (row < 0 || row >= model_.rowCount())
    {
        row = 0;
    }
    QModelIndex index = model_.index(row, 0);
    if (index.isValid())
    {
        treeview_.select(index);
    }
    qDebug(" >>> ExplorerView::displayView");
}

QSqlQuery ExplorerView::runDatabaseQuery(const QString &queryString, const QString &bindString)
{
	qDebug(" >>> ExplorerView::runDatabaseQuery");
    QSqlQuery query;

    query.prepare(queryString);
    if (queryString.contains(":week_ago"))
    {
        QDateTime week_ago = QDateTime::currentDateTime().addDays(-7);
        query.bindValue(":week_ago", week_ago.toString(Qt::ISODate));
    }
    else if (!bindString.isEmpty())
    {
        if (queryString.contains(":title"))
        {
            query.bindValue(":title", bindString);
        }
        else if (queryString.contains(":author"))
        {
            query.bindValue(":author", bindString);
        }
        else if (queryString.contains(":publisher"))
        {
            query.bindValue(":publisher", bindString);
        }
        else if (queryString.contains(":year"))
        {
            query.bindValue(":year", bindString);
        }
        else if (queryString.contains(":series"))
        {
            query.bindValue(":series", bindString);
        }
        else if (queryString.contains(":artist"))
        {
            query.bindValue(":artist", bindString);
        }
        else if (queryString.contains(":album"))
        {
            query.bindValue(":album", bindString);
        }
        else if (queryString.contains(":genre"))
        {
            query.bindValue(":genre", bindString);
        }
    }
    query.exec();
	
	qDebug() << "query " << queryString;
	qDebug() << "binds " <<  bindString;
	qDebug(" <<< ExplorerView::runDatabaseQuery");
	
    return query;
}

void ExplorerView::onItemActivated(const QModelIndex &index)
{
	qDebug(" >>> ExplorerView::runDatabaseQuery");
    QStandardItem *item = model_.itemFromIndex(index);
    if (!organize_mode_ && item != 0 && item->isSelectable())
    {
        QString fullFileName;
        int row = 0;

        if (level_ < selected_row_.size() && selected_row_.at(level_) != treeview_.selected())
        {
            if (level_ >= int(!homeSkipped_))
            {
                int oldLevel = (current_path_.count('/') - root_path_.count('/')) + int(!homeSkipped_);
                for(; oldLevel > level_; oldLevel--)
                {
                    current_path_.chop(current_path_.size() - current_path_.lastIndexOf('/'));
                }
            }
            else
            {
                current_path_ = "";
            }
            selected_row_.resize(level_);
        }

        if (level_ == selected_row_.size())
        {
            selected_row_.push_back(treeview_.selected());
        }

        fullFileName = current_path_ + "/" + item->text();

        switch (handler_type_)
        {
        case HDLR_HOME:
        {
            onCategoryActivated(QStringList(item->text()) + item->data().toStringList());
            break;
        }
        case HDLR_FILES:
        {
            QBitArray properties = item->data().toBitArray();
            if (properties[0] == true)
            {
                current_path_ = fullFileName;
                if (++level_ < selected_row_.size())
                {
                    row = selected_row_.at(level_);
                }
                showFiles(category_id_, fullFileName, row);
            }
            else
            {
                if (openDocument(fullFileName) == false && properties[1] == true)
                {
                    qDebug() << "app:" << fullFileName;
                    run(fullFileName, QStringList());
                }
            }
            break;
        }
        case HDLR_BOOKS:
        {
            if (level_ == int(!homeSkipped_))
            {
                query_list_ = item->data().toStringList()[2].split(';');
                view_icon_ = item->icon();
            }

            if (level_ <= query_list_.size() - homeSkipped_)
            {
                current_path_ = fullFileName;
                if (++level_ < selected_row_.size())
                {
                    row = selected_row_.at(level_);
                }
                showDBViews(category_id_, fullFileName, row);
            }
            else
            {
                fullFileName = item->data().toString();
                openDocument(fullFileName);
            }
            break;
        }
        case HDLR_APPS:
        {
            QString app = item->data().toString();
            if (!app.isEmpty())
            {
                qDebug() << "app:" << app;
                run(app, QStringList());
            }
            break;
        }
        case HDLR_WEBSITES:
        {
            QStringList url = item->data().toStringList();
            if (!url.at(0).isEmpty())
            {
                qDebug() << "website:" << url.at(0);
                run("/opt/onyx/arm/bin/web_browser", url);
            }
            break;
        }
        case HDLR_MUSIC:
        {
            if (level_ == int(!homeSkipped_))
            {
                query_list_ = item->data().toStringList()[2].split(';');
                view_icon_ = item->icon();
            }

            if (level_ <= query_list_.size() - homeSkipped_)
            {
                current_path_ = fullFileName;
                if (++level_ < selected_row_.size())
                {
                    row = selected_row_.at(level_);
                }
                showDBViews(category_id_, fullFileName, row, QString(qgetenv("HOME")) + "/.mpd/music/");
            }
            else
            {
                QMpdClient::instance().addToPlaylist(item->data().toString(),
                                                     QMpdClient::instance().status().songPos() + 1);
            }
            break;
        }
        default:
            break;
        }
    }
}

void ExplorerView::onCategoryActivated(const QStringList &stringList)
{
    QString path = "/" + stringList[0];
    int categoryId = stringList[1].toInt();
    handler_type_ = HandlerType(stringList[2].toInt());
    QString handlerData = stringList[3];
    int row = 0;

    if (current_path_.isEmpty())
    {
        if (handler_type_ == HDLR_FILES)
        {
            current_path_ = handlerData;
            root_path_ = handlerData;
        }
        else
        {
            current_path_ = path;
            root_path_ = path;
        }
    }
    level_ = (current_path_.count('/') - root_path_.count('/')) + int(!homeSkipped_);

    if (level_ < selected_row_.size())
    {
        row = selected_row_.at(level_);
    }

    switch (handler_type_)
    {
    case HDLR_FILES:
        showFiles(categoryId, current_path_, row);
        break;
    case HDLR_BOOKS:
        showDBViews(categoryId, current_path_, row);
        break;
    case HDLR_APPS:
        showApps(categoryId, current_path_, row);
        break;
    case HDLR_WEBSITES:
        showWebsites(categoryId, current_path_, row);
        break;
    case HDLR_MUSIC:
        showDBViews(categoryId, current_path_, row, QString(qgetenv("HOME")) + "/.mpd/music/");
        break;
    default:
        break;
    }
}

void ExplorerView::addToPlaylist(QStandardItem *item)
{
    int  level = level_ - int(!homeSkipped_);

    if (level > 0 && level == query_list_.size())
    {
        QString fileName = item->data().toString();
        QMpdClient::instance().addToPlaylist(fileName);
    }
    else
    {
        QString queryString, bindString;

        if (level == 0)
        {
            QStringList booksQueryList = item->data().toStringList()[2].split(';');
            if (booksQueryList.size() != 1)
            {
                return;
            }
            queryString = booksQueryList.at(0);
        }
        else
        {
            queryString = query_list_.at(level);
            bindString = item->text();
        }

        QSqlQuery query = runDatabaseQuery(queryString, bindString);

        while (query.next())
        {
            toBeAdded_ << query.value(0).toString();
        }

        while (!toBeAdded_.isEmpty() &&
               QMpdClient::instance().addToPlaylist(toBeAdded_.takeFirst()) == -1);
    }
}

QString ExplorerView::getByExtension(const QString &field, const QFileInfo &fileInfo)
{
    QStringList extensions = FileSystemUtils::suffixList(fileInfo);
    QString result;
    QSqlQuery query;

    for (int i = 0; i < extensions.size(); i++)
    {
        QString extension = extensions.at(i);
        if (field == "viewer")
        {
            query.exec(QString("SELECT app.executable FROM applications app "
                               "INNER JOIN associations file ON app.id = file.viewer "
                               "WHERE file.extension = '%1'")
                              .arg(extension));
        }
        else
        {
            query.exec(QString("SELECT %1 FROM associations WHERE extension = '%2'")
                              .arg(field)
                              .arg(extension));
        }

        if (query.first())
        {
            result = query.value(0).toString();
        }
        query.finish();

        if (QFile::exists(result))
        {
            return result;
        }
    }

    return QString();
}

QString ExplorerView::getIconByExtension(const QFileInfo &fileInfo)
{
    QString extIcon;

    if (fileInfo.isDir())
    {
        extIcon = "/usr/share/explorer/images/middle/directory.png";
    }
    else
    {
        // Check file system for extension icon
        extIcon = FileSystemUtils::getThumbnail(fileInfo, icon_extensions_);
        if (extIcon.isNull())
        {
            // Check database for extension icon
            extIcon = getByExtension("icon", fileInfo);
            if (extIcon.isNull())
            {
                // Use one of the default icons
                if (FileSystemUtils::isRunnable(fileInfo))
                {
                    extIcon = "/usr/share/explorer/images/middle/runnable_file.png";
                }
                else
                {
                    extIcon = "/usr/share/explorer/images/middle/unknown_document.png";
                }
            }
        }
    }

    return extIcon;
}

QString ExplorerView::getDisplayName(const QFileInfo &fileInfo)
{
    QString displayName;
    bool ok;

    fileInfo.suffix().toInt(&ok);

    if (ok)
    {
        displayName = fileInfo.fileName();
    }
    else
    {
        displayName = fileInfo.completeBaseName();
    }

    displayName[0] = displayName[0].toUpper();

    return displayName;
}

bool ExplorerView::openDocument(const QString &fullFileName)
{
    QString viewer = getByExtension("viewer", QFileInfo(fullFileName));
    if (!viewer.isNull())
    {
        QSqlQuery query;
        query.prepare("SELECT cover, read_count, id FROM books WHERE file = :file");
        query.bindValue(":file", fullFileName);
        query.exec();
        
        if (query.next())
        {
            book_cover_ = query.value(0).toString();

            query.exec(QString("UPDATE books SET read_date = '%1', read_count = %2 WHERE id = %3")
                              .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                              .arg(query.value(1).toInt() + 1)
                              .arg(query.value(2).toInt()));
        } else { 
			//no such book entry in database; let's scan it.
			BookScanner adhoc_scanner(book_extensions_);
			adhoc_scanner.scan(fullFileName);
		}

        qDebug() << "view:" << fullFileName;
        run(viewer, QStringList() << fullFileName);

        return true;
    }

    return false;
}

void ExplorerView::addApplication(const QString &categoryName, const QString &fullFileName)
{
    // Add to database
    QFileInfo fileInfo(fullFileName);
    QString displayName = getDisplayName(fileInfo);
    QString iconFileName = FileSystemUtils::getThumbnail(fileInfo, icon_extensions_);

    QSqlQuery query;
    query.exec(QString("SELECT id FROM categories WHERE name = '%1'").arg(categoryName));
    if (query.next())
    {
        int category = query.value(0).toInt();
        query.exec(QString("INSERT INTO applications (name, executable, icon, category_id) "
                           "VALUES ('%1', '%2', '%3', %4)")
                           .arg(displayName)
                           .arg(fullFileName)
                           .arg(iconFileName)
                           .arg(category));
    }

    if (query.numRowsAffected() > 0)
    {
        qDebug() << "added to applications/games:" << fullFileName;
    }
}

void ExplorerView::run(const QString &command, const QStringList &parameters)
{
    process_.setEnvironment(QProcess::systemEnvironment());
    process_.start(command, parameters);
    sys::SysStatus::instance().setSystemBusy(true);
}

void ExplorerView::onProcessFinished(int, QProcess::ExitStatus)
{
    book_cover_ = QString();
    status_bar_.repaint();
    sys::SysStatus::instance().setSystemBusy(false);
    onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GC);
}

void ExplorerView::onPositionChanged(int currentPage, int pages)
{
    status_bar_.setProgress(currentPage, pages);
    onyx::screen::instance().flush(this, waveform_);
}

void ExplorerView::onDatabaseUpdated(bool)
{
    qDebug() << "updating local music database...";
    QMpdSongList songList = QMpdClient::instance().getSongList();

    QSqlQuery query;
    query.exec("DELETE FROM music");

    for (int i = 0; i < songList.size(); i++)
    {
        QMpdSong song = songList.at(i);

        query.prepare("SELECT id FROM music WHERE file = :file");
        query.bindValue(":file", song.uri());
        query.exec();
        if (!query.next())
        {
            query.prepare("INSERT INTO music (file, title, artist, album, track, "
                                             "genre, year, add_date, play_count) "
                          "VALUES (:file, :title, :artist, :album, :track, "
                                  ":genre, :year, :add_date, :play_count)");
            query.bindValue(":file", song.uri());
            query.bindValue(":title", song.title());
            query.bindValue(":artist", song.artist());
            query.bindValue(":album", song.album());
            query.bindValue(":track", song.track());
            query.bindValue(":genre", song.genre());
            query.bindValue(":year", song.year());
            query.bindValue(":add_date", QDateTime::currentDateTime().toString(Qt::ISODate));
            query.bindValue(":play_count", 0);
            query.exec();
        }
    }

    sys::SysStatus::instance().setSystemBusy(false);

    if (handler_type_ == HDLR_MUSIC)
    {
        if (level_ > int(!homeSkipped_))
        {
            level_ = int(!homeSkipped_);
            current_path_.chop(current_path_.size() - current_path_.indexOf('/', 1));
        }
        selected_row_.resize(level_);
        showDBViews(category_id_, current_path_, 0, QString(qgetenv("HOME")) + "/.mpd/music/");
    }
    else
    {
        onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GC);
    }
}

void ExplorerView::onPlaylistChanged()
{
    while (!toBeAdded_.isEmpty())
    {
        if (QMpdClient::instance().addToPlaylist(toBeAdded_.takeFirst()) >= 0)
        {
            return;
        }
    }

    if (toBeAdded_.isEmpty())
    {
        static bool playListFilled = false;
        QMpdSongList playlist = QMpdClient::instance().syncPlaylist();
        int size = playlist.size();
        int position = QMpdClient::instance().latestSongPosition();

        if (bool(size) ^ playListFilled)
        {
            playListFilled = bool(size);
            if (!playListFilled)
            {
                sys::SysStatus::instance().reportMusicPlayerState(STOP_PLAYER);
            }
        }

        qDebug() << QString("current playlist (%1 entries):").arg(size);
        if (size)
        {
            for (int i = 0; i < size; i++)
            {
                QMpdSong song = playlist.at(i);
                QString  name = song.artist() + " - " + song.title();
                qDebug() << name;
            }

            if (position >= 0)
            {
                QMpdClient::instance().playPosition(position);
            }
            else
            {
                QMpdClient::instance().play();
            }
        }
    }
}

void ExplorerView::onStateChanged(QMpdStatus::State state)
{
    const QStringList stateName = QStringList() << "Unknown" << "Stop" << "Play" << "Pause";
    qDebug() << "state:" << stateName[state];

    switch (state)
    {
    case QMpdStatus::StatePlay:
        sys::SysStatus::instance().enableIdle(false);
        sys::SysStatus::instance().reportMusicPlayerState(MUSIC_PLAYING);
        break;
    case QMpdStatus::StatePause:
        sys::SysStatus::instance().reportMusicPlayerState(MUSIC_PAUSED);
        break;
    case QMpdStatus::StateStop:
    default:
        sys::SysStatus::instance().enableIdle(true);

        if (QMpdClient::instance().playlist().size() > 0)   // No connection required
        {
            sys::SysStatus::instance().reportMusicPlayerState(MUSIC_STOPPED);
        }
        break;
    }
}

void ExplorerView::onSongChanged(const QMpdSong &song)
{
    qDebug() << "song:" << song.uri();

    QSqlQuery query;

    query.prepare("SELECT play_count, id FROM music WHERE file = :file");
    query.bindValue(":file", song.uri());
    query.exec();

    if (query.next())
    {
        query.exec(QString("UPDATE music SET play_date = '%1', play_count = %2 WHERE id = %3")
                          .arg(QDateTime::currentDateTime().toString(Qt::ISODate))
                          .arg(query.value(0).toInt() + 1)
                          .arg(query.value(1).toInt()));
    }
}

void ExplorerView::onWakeup()
{
    QMpdStatus::State state = QMpdClient::instance().status().state();

    if (state == QMpdStatus::StatePlay || state == QMpdStatus::StatePause)
    {
        sys::SysStatus::instance().enableIdle(false);
    }

    if (sys::SysStatus::instance().isWpaSupplicantRunning())
    {
        sys::SysStatus::instance().stopWpaSupplicant();
    }
}

void ExplorerView::onForceQuit()
{
    if (process_.state() == QProcess::Running)
    {
        qDebug() << "killing process:" << process_.pid();
        process_.kill();
    }
}

void ExplorerView::onVolumeChanged(int newVolume, bool isMute)
{
    qDebug() << "onVolumeChanged:" << newVolume << isMute;
    QMpdClient::instance().setVolume(newVolume);
}

void ExplorerView::onAboutToSuspend()
{
    qDebug("System is about to suspend");

    const QString screenSaverImage = "/usr/share/system_manager/images/screen_saver0.png";
    const QStringList screenSaverPools = QStringList()
        << "/media/sd/screen_saver"
        << "/media/flash/screen_saver"
        << "/usr/share/explorer/images/screen_saver";

    QString selectedImage = book_cover_;

    if (selectedImage.isEmpty())
    {
        for (int poolIndex = 0; poolIndex < screenSaverPools.size(); poolIndex++)
        {
            QDir screenSaverDir(screenSaverPools[poolIndex], "*.png",
                                QDir::Name | QDir::IgnoreCase,
                                QDir::Files | QDir::NoDotAndDotDot);
            QFileInfoList fileInfoList = screenSaverDir.entryInfoList();
            if (fileInfoList.size())
            {
                uint seed = qrand();

                selectedImage = fileInfoList[seed % fileInfoList.size()].absoluteFilePath();
                break;
            }
        }
    }

    if (QFile::exists(screenSaverImage))
    {
        QFile::remove(screenSaverImage);
    }

    QPixmap pixmap(selectedImage);
    QSize screenSize(600, 800);
    if (pixmap.size() != screenSize)
    {
        QPixmap newPixmap(screenSize);
        newPixmap.fill();
        QPainter painter(&newPixmap);
        if (pixmap.width() > pixmap.height())
        {
            painter.translate(0, 800);
            painter.rotate(270.0);
            screenSize.transpose();
        }
        QPixmap scaledPixmap = pixmap.scaled(screenSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        painter.drawPixmap(QRect(QPoint(), screenSize - scaledPixmap.size()).center(), scaledPixmap);
        newPixmap.save(screenSaverImage, "PNG");
    }
    else
    {
        QFile::copy(selectedImage, screenSaverImage);
    }

    // Standby splash is displayed by the system manager
    sys::SysStatus::instance().suspend();
}

void ExplorerView::onAboutToShutDown()
{
    qDebug("System is about to shutdown");

    // Save date/time
    system("hwclock -w -u");

    // Save seed
    QFile seedFile(QString(qgetenv("HOME")) + "/seed");
    if (seedFile.open(QIODevice::WriteOnly))
    {
        seedFile.write(QString("%1").arg(qrand()).toLocal8Bit());
        seedFile.close();
    }

    // Shutdown splash has to be provided by the explorer (us)
    ExplorerSplash shutdownSplash(QPixmap("/usr/share/explorer/images/shutdown.png"), this);
    shutdownSplash.show();

    onyx::screen::instance().flush(&shutdownSplash, onyx::screen::ScreenProxy::GC);

    // Switch off Wifi
    if (SysStatus::instance().sdioState())
    {
        SysStatus::instance().enableSdio(false);
    }

    close();
    sys::SysStatus::instance().shutdown();
}

void ExplorerView::popupMenu()
{
    QModelIndex index = model_.index(treeview_.selected(), 0);
    QStandardItem *item = 0;
    QString fullFileName;
    QStringList extensions;
    QString editor;
    QStringList doc;
    bool refresh = false;

    if (index.isValid())
    {
        item = model_.itemFromIndex(index);
        if (item != 0)
        {
            fullFileName = current_path_ + "/" + item->text();
        }
    }

    if (onyx::screen::instance().defaultWaveform() == onyx::screen::ScreenProxy::DW)
    {
        // Stop fastest update mode to get better image quality.
        onyx::screen::instance().setDefaultWaveform(onyx::screen::ScreenProxy::GC);
    }

    PopupMenu menu(this);

    // system actions
    std::vector<int> sys_actions;
    sys_actions.push_back(ROTATE_SCREEN);
    sys_actions.push_back(MUSIC);
    if (sys::SysStatus::instance().isWpaSupplicantRunning())
    {
        sys_actions.push_back(WIFI_OFF);
    }
    else
    {
        sys_actions.push_back(WIFI);
    }
    if (mainUI_)
    {
        if (FileSystemUtils::isSDMounted())
        {
            sys_actions.push_back(REMOVE_SD);
        }
        sys_actions.push_back(STANDBY);
        sys_actions.push_back(SHUTDOWN);
    }
    else
    {
        sys_actions.push_back(RETURN_TO_LIBRARY);
    }
    systemActions_.generateActions(sys_actions);
    menu.setSystemAction(&systemActions_);

    // organize and file actions
    fileActions_.clear();
    organizeActions_.initializeActions(QIcon(":/images/organize.png"), "Organize");
    organizeActions_.addAction(QIcon(":/images/category_organization.png"), tr("Home Screen"), ORG_CATEGORIES);

    switch (handler_type_)
    {
    case HDLR_FILES:
        if (item != 0)
        {
            doc << fullFileName;
            editor = getByExtension("editor", QFileInfo(doc[0]));
            QBitArray properties = item->data().toBitArray();

            if (properties[0] == true)      // Directory
            {
                organizeActions_.addAction(QIcon(":/images/scan.png"), tr("Scan Books From Here"), ORG_SCANPATH);
            }
            else if (properties[1] == true) // Runnable
            {
                organizeActions_.addAction(QIcon(":/images/applications.png"), tr("Add to Applications"), ORG_ADD2APPS);
                organizeActions_.addAction(QIcon(":/images/games.png"), tr("Add to Games"), ORG_ADD2GAMES);
            }
            else                            // Normal file
            {
                QStringList extensions = FileSystemUtils::suffixList(QFileInfo(doc[0]));
                for (int i = 0; i < extensions.size(); i++)
                {
                    QString extension = extensions.at(i);
                    if (book_extensions_.contains(extension))
                    {
                        organizeActions_.addAction(QIcon(":/images/scan.png"), tr("Scan Book"), ORG_SCANPATH);
                        break;
                    }
                    else if (icon_extensions_.contains(extension))
                    {
                        organizeActions_.addAction(QIcon(":/images/website_icon.png"), tr("Set as Website icon"), ORG_ADDWEBSITEICON);
                        break;
                    }
                }
            }

            fileActions_.initializeActions(QIcon(":/images/edit.png"), tr("File"));
            if (!editor.isNull())
            {
                fileActions_.addAction(QIcon(":/images/file_edit.png"), tr("Edit"), FILE_EDIT);
            }
            fileActions_.addAction(QIcon(":/images/rename.png"), tr("Rename"), FILE_RENAME);
            fileActions_.addAction(QIcon(":/images/delete.png"), tr("Delete"), FILE_DELETE);
            fileActions_.addSeparator();
            fileActions_.addAction(QIcon(":/images/cut.png"), tr("Cut"), FILE_CUT);
            fileActions_.addAction(QIcon(":/images/copy.png"), tr("Copy"), FILE_COPY);
            if (!fileClipboard_.isEmpty())
            {
                fileActions_.addAction(QIcon(":/images/paste.png"), tr("Paste"), FILE_PASTE);
            }
        }
        else if (!fileClipboard_.isEmpty())
        {
            fileActions_.initializeActions(QIcon(":/images/edit.png"), tr("File"));
            fileActions_.addAction(QIcon(":/images/paste.png"), tr("Paste"), FILE_PASTE);
        }
        break;
    case HDLR_BOOKS:
        organizeActions_.addAction(QIcon(":/images/scan.png"), tr("Scan Books Int. Flash"), ORG_SCANFLASH);
        organizeActions_.addAction(QIcon(":/images/scan.png"), tr("Scan Books SD Card"), ORG_SCANSD);
        organizeActions_.addSeparator();
        if (item != 0)
        {
            organizeActions_.addAction(QIcon(":/images/delete.png"), tr("Remove Book"), ORG_REMOVEBOOK);

            doc << item->data().toString();
            editor = getByExtension("editor", QFileInfo(doc[0]));

            if (!editor.isNull())
            {
                fileActions_.initializeActions(QIcon(":/images/edit.png"), tr("File"));
                fileActions_.addAction(QIcon(":/images/file_edit.png"), tr("Edit"), FILE_EDIT);
            }
        }
        organizeActions_.addAction(QIcon(":/images/delete.png"), tr("Remove all Books"), ORG_REMOVEALLBOOKS);
        organizeActions_.addAction(QIcon(":/images/delete.png"), tr("Clean up Database"), ORG_DBCLEANUP);
        break;
    case HDLR_APPS:
        if (item != 0)
        {
            organizeActions_.addAction(QIcon(":/images/delete.png"), tr("Remove App/Game"), ORG_REMOVEAPP);
        }
        break;
    case HDLR_WEBSITES:
        organizeActions_.addAction(QIcon(":/images/add.png"), tr("Add Website"), ORG_ADDWEBSITE);
        if (item != 0)
        {
            organizeActions_.addAction(QIcon(":/images/delete.png"), tr("Remove Website"), ORG_REMOVEWEBSITE);
        }
        break;
    case HDLR_MUSIC:
        organizeActions_.addAction(QIcon(":/images/music_scan.png"), tr("Scan Music Int. Flash"), ORG_SCANMUSICFLASH);
        organizeActions_.addAction(QIcon(":/images/music_scan.png"), tr("Scan Music SD Card"), ORG_SCANMUSICSD);
        if (item != 0)
        {
            if (level_ == int(!homeSkipped_))
            {
                QStringList booksQueryList = item->data().toStringList()[2].split(';');
                if (booksQueryList.size() != 1)
                {
                    break;
                }
            }
            organizeActions_.addSeparator();
            organizeActions_.addAction(QIcon(":/images/addto_playlist.png"), tr("Add to Playlist"), ORG_ADD2PLAYLIST);
        }
        break;
    default:
        break;
    }

    menu.addGroup(&organizeActions_);
    if (!fileActions_.isEmpty())
    {
        menu.addGroup(&fileActions_);
    }

    // associate actions
    associateActions_.initializeActions(QIcon(":/images/filetype_settings.png"), tr("Select Reader"));
    QSqlQuery query(QString("SELECT extension FROM associations WHERE viewers <> '' ORDER BY extension"));
    while (query.next())
    {
        QString extension = query.value(0).toString();
        associateActions_.addAction(QIcon("/usr/share/explorer/images/extensions/" + extension + ".png"),
                                    extension, extensions.size());
        extensions.append(extension);
    }
    query.finish();
    menu.addGroup(&associateActions_);

    // settings actions
    settingsActions_.initializeActions(QIcon(":/images/settings.png"), tr("Settings"));
    if (mainUI_)
    {
        settingsActions_.addAction(QIcon(":/images/time_zone.png"), tr("Time Zone"), SET_TZ);
        settingsActions_.addAction(QIcon(":/images/power_management.png"), tr("Power Management"), SET_PWR_MGMT);
        settingsActions_.addAction(QIcon(":/images/screen_calibration.png"), tr("Screen Calibration"), SET_CALIBRATE);
        settingsActions_.addAction(QIcon(":/images/startup.png"),tr("Open last read book by default"), SET_RESUME_LAST);
    }
    settingsActions_.addAction(QIcon(":/images/restore.png"), tr("Default Categories"), SET_DEFAULTS);
    settingsActions_.addAction(QIcon(":/images/about.png"), tr("About"), SET_ABOUT);
    menu.addGroup(&settingsActions_);

    if (menu.popup() == QDialog::Accepted)
    {
        onyx::screen::instance().enableUpdate(false);
        QApplication::processEvents();
        onyx::screen::instance().enableUpdate(true);
        onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GU, false);

        QAction *group = menu.selectedCategory();
        if (group == systemActions_.category())
        {
            switch (systemActions_.selected())
            {
            case RETURN_TO_LIBRARY:
                qApp->exit();
                return;
            case ROTATE_SCREEN:
            {
                ScreenRotationDialog dialog;
                dialog.popup();
                return;
            }
            case MUSIC:
                if (mainUI_)
                {
                    SysStatus::instance().requestMusicPlayer(START_PLAYER);
                    return;
                }
                else
                {
                    MpdClientDialog mpdClientDialog(this);
                    mpdClientDialog.exec();
                    break;
                }
            case WIFI:
            {
                WifiDialog dialog(this, SysStatus::instance());
                dialog.popup();
                break;
            }
            case WIFI_OFF:
                sys::SysStatus::instance().stopWpaSupplicant();
                break;
            case REMOVE_SD:
                if (SysStatus::instance().umountSD() == false)
                {
                    qDebug("SD removal failed.");
                }
                break;
            case STANDBY:
                onAboutToSuspend();
                return;
            case SHUTDOWN:
                onAboutToShutDown();
                return;
            default:
                break;
            }
        }
        else if (group == fileActions_.category())
        {
            switch (fileActions_.selected())
            {
            case FILE_EDIT:
                qDebug() << "edit:" << doc[0];
                run(editor, doc);
                return;
            case FILE_RENAME:
            {
                treeview_.setHovering(false);
                onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GU);

                OnyxKeyboardDialog dialog(this, tr("Enter new file name"));
                QString newFileName = dialog.popup(item->text());
                treeview_.setHovering(true);

                if (!newFileName.isEmpty())
                {
                    newFileName.prepend(current_path_ + "/");
                    if (QFile::rename(fullFileName, newFileName))
                    {
                        qDebug() << "renamed:" << fullFileName << "to" << newFileName;
                        showFiles(category_id_, current_path_, treeview_.selected());
                        return;
                    }
                    else
                    {
                        QString message;
                        if (QFile::exists(newFileName))
                        {
                            if (QFileInfo(newFileName).isDir())
                            {
                                message = tr("Directory name already exists");
                            }
                            else
                            {
                                message = tr("File name already exists");
                            }
                        }
                        else
                        {
                            message = tr("Rename failed");
                        }

                        MessageDialog error(QMessageBox::Icon(QMessageBox::Warning) , tr("Rename failed"),
                                            message, QMessageBox::Ok);
                        error.exec();
                    }
                }
                break;
            }
            case FILE_DELETE:
            {
                // Confirmation dialog
                MessageDialog del(QMessageBox::Icon(QMessageBox::Warning) , tr("Delete"),
                                  tr("Do you want to delete %1?").arg(item->text()),
                                  QMessageBox::Yes | QMessageBox::No);

                if (del.exec() == QMessageBox::Yes)
                {
                    bool removed = false;
                    QFileInfo fileInfo(fullFileName);
                    if (fileInfo.isDir())
                    {
                        removed = FileSystemUtils::removeDir(fullFileName);
                    }
                    else if (fileInfo.isFile())
                    {
                        removed = QFile::remove(fullFileName);
                    }

                    if (removed == true)
                    {
                        qDebug() << "deleted:" << fullFileName;
                        showFiles(category_id_, current_path_, 0);
                        return;
                    }
                    else
                    {
                        qDebug() << "deleting failed!";
                    }
                }
                break;
            }
            case FILE_CUT:
                fileClipboard_.cut(fullFileName);
                item->setSelectable(false);
                treeview_.repaint();
                repaint();
                break;
            case FILE_COPY:
                fileClipboard_.copy(fullFileName);
                item->setSelectable(true);
                treeview_.repaint();
                repaint();
                break;
            case FILE_PASTE:
                if (fileClipboard_.paste(current_path_))
                {
                    qDebug() << "pasted:" << (current_path_ + "/" + fileClipboard_.fileName());
                    showFiles(category_id_, current_path_, treeview_.selected());
                    return;
                }
                else
                {
                    QString message;
                    if (QFile::exists(current_path_ + "/" + fileClipboard_.fileName()))
                    {
                        if (fileClipboard_.holdsDir())
                        {
                            message = tr("Directory already exists");
                        }
                        else
                        {
                            message = tr("File already exists");
                        }
                    }
                    else
                    {
                        message = tr("Paste failed");
                    }

                    MessageDialog error(QMessageBox::Icon(QMessageBox::Warning) , tr("Paste failed"),
                                        message, QMessageBox::Ok);
                    error.exec();
                }
                break;
            default:
                break;
            }
        }
        else if (group == organizeActions_.category())
        {
            BookScanner scanner(book_extensions_);
            QString dir = "sd";

            switch (organizeActions_.selected())
            {
            case ORG_CATEGORIES:
                organizeCategories(0);
                return;
            case ORG_SCANFLASH:
                dir = "flash";
                // fall through
            case ORG_SCANSD:
                sys::SysStatus::instance().setSystemBusy(true);
                scanner.scan("/media/" + dir);
                sys::SysStatus::instance().setSystemBusy(false);
                if (handler_type_ == HDLR_BOOKS)
                {
                    if (level_ > int(!homeSkipped_))
                    {
                        level_ = int(!homeSkipped_);
                        current_path_.chop(current_path_.size() - current_path_.indexOf('/', 1));
                    }
                    selected_row_.resize(level_);
                    showDBViews(category_id_, current_path_, 0);
                }
                return;
            case ORG_SCANPATH:
                sys::SysStatus::instance().setSystemBusy(true);
                scanner.scan(fullFileName);
                sys::SysStatus::instance().setSystemBusy(false);
                break;
            case ORG_SCANMUSICFLASH:
                dir = "flash";
                // fall through
            case ORG_SCANMUSICSD:
                sys::SysStatus::instance().setSystemBusy(true);
                qDebug() << "updating mpd database...";
                if (!QMpdClient::instance().updateDB(dir))
                {
                    sys::SysStatus::instance().setSystemBusy(false);
                    break;
                }
                return;
            case ORG_ADD2APPS:
                addApplication("Applications", fullFileName);
                break;
            case ORG_ADD2GAMES:
                addApplication("Games", fullFileName);
                break;
            case ORG_ADD2PLAYLIST:
                addToPlaylist(item);
                break;
            case ORG_ADDWEBSITE:
            {
                treeview_.setHovering(false);
                onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GU);

                OnyxKeyboardDialog dialog(this, tr("Enter web address"));
                QString url = dialog.popup("");
                treeview_.setHovering(true);

                if (!url.isEmpty())
                {
                    QString name = url;
                    if (name.startsWith("http://"))
                        name.remove("http://");
                    if (name.startsWith("www."))
                        name.remove("www.");
                    name.truncate(name.lastIndexOf('.'));
                    name[0] = name[0].toUpper();

                    if (!url.startsWith("http://"))
                        url.prepend("http://");

                    QString icon = "/usr/share/explorer/images/middle/websites.png";

                    QSqlQuery query(QString("INSERT INTO websites (name, url, icon) "
                                            "VALUES ('%1', '%2', '%3')").arg(name).arg(url).arg(icon));

                    if (query.numRowsAffected() > 0)
                    {
                        qDebug() << "added to websites:" << url;
                        showWebsites(category_id_, current_path_, treeview_.selected());
                        return;
                    }
                }
                break;
            }
            case ORG_ADDWEBSITEICON:
            {
                QSqlQuery query(QString("UPDATE websites SET icon = '%1' WHERE url LIKE '%%2%'")
                                       .arg(fullFileName)
                                       .arg(QFileInfo(fullFileName).baseName()));

                if (query.numRowsAffected() > 0)
                {
                    qDebug() << "Set as website icon:" << fullFileName;
                }
                break;
            }
            case ORG_REMOVEBOOK:
            {
                // Confirmation dialog
                MessageDialog remove(QMessageBox::Icon(QMessageBox::Warning) , tr("Remove Book"),
                                     tr("Do you want to remove %1?").arg(item->text()),
                                     QMessageBox::Yes | QMessageBox::No);

                if (remove.exec() == QMessageBox::Yes)
                {
                    QSqlQuery query(QString("DELETE FROM books WHERE file = '%1'")
                                            .arg(item->data().toString()));

                    if (query.numRowsAffected() > 0)
                    {
                        qDebug() << "removed:" << item->data().toString();
                        showDBViews(category_id_, current_path_, 0);
                        return;
                    }
                }
                break;
            }
	    
		case ORG_DBCLEANUP:
		{
			// Confirmation dialog
			qDebug() << "starting cleanup" ;
			MessageDialog cleanup(QMessageBox::Icon(QMessageBox::Warning) , tr("Clean up database"),
						tr("Do you want to clean up obsolete entries? "),
						QMessageBox::Yes | QMessageBox::No);
			if (cleanup.exec() == QMessageBox::Yes)
			{
				QSqlQuery loopquery(QString("SELECT file FROM books"));
				while (loopquery.next())
				{
					//qDebug() << " checking : " << loopquery.value(0).toString();
					QFile bookfile(loopquery.value(0).toString());
					if (!bookfile.exists()) {
						//qDebug() << "will delete entry for " << loopquery.value(0).toString();
						QSqlQuery delquery(QString("DELETE FROM books where file='%1'").arg(loopquery.value(0).toString()));
						//qDebug() << delquery.lastError();
					}
				}
				//vacuum db; provides significant size reduction when many books have been deleted
				QSqlQuery cleanupquery(QString("vacuum"));
			}
			break;
		}

            case ORG_REMOVEALLBOOKS:
            {
                // Confirmation dialog
                MessageDialog remove(QMessageBox::Icon(QMessageBox::Warning) , tr("Remove all Books"),
                                     tr("Do you want to remove all books?"),
                                     QMessageBox::Yes | QMessageBox::No);

                if (remove.exec() == QMessageBox::Yes)
                {
                    QSqlQuery query("DELETE FROM books");

                    qDebug() << "removed all books";
                    if (handler_type_ == HDLR_BOOKS)
                    {
                        if (level_ > int(!homeSkipped_))
                        {
                            level_ = int(!homeSkipped_);
                            current_path_.chop(current_path_.size() - current_path_.indexOf('/', 1));
                        }
                        selected_row_.resize(level_);
                        showDBViews(category_id_, current_path_, 0);
                        return;
                    }
                }
                break;
            }
            case ORG_REMOVEAPP:
            {
                // Confirmation dialog
                MessageDialog remove(QMessageBox::Icon(QMessageBox::Warning) , tr("Remove"),
                                     tr("Do you want to remove %1?").arg(item->text()),
                                     QMessageBox::Yes | QMessageBox::No);

                if (remove.exec() == QMessageBox::Yes)
                {
                    QSqlQuery query(QString("DELETE FROM applications WHERE executable = '%1' "
                                            "AND category_id = '%2'").arg(item->data().toString())
                                            .arg(category_id_));

                    if (query.numRowsAffected() > 0)
                    {
                        qDebug() << "removed:" << item->data().toString();
                        showApps(category_id_, current_path_, 0);
                        return;
                    }
                }
                break;
            }
            case ORG_REMOVEWEBSITE:
            {
                // Confirmation dialog
                MessageDialog remove(QMessageBox::Icon(QMessageBox::Warning) , tr("Remove"),
                                     tr("Do you want to remove %1?").arg(item->text()),
                                     QMessageBox::Yes | QMessageBox::No);

                if (remove.exec() == QMessageBox::Yes)
                {
                    QSqlQuery query(QString("DELETE FROM websites WHERE url = '%1'")
                                            .arg(item->data().toString()));

                    if (query.numRowsAffected() > 0)
                    {
                        qDebug() << "removed:" << item->data().toString();
                        showWebsites(category_id_, current_path_, 0);
                        return;
                    }
                }
                break;
            }
            default:
                break;
            }
        }
        else if (group == associateActions_.category())
        {
			//reader selection
            QString extension = extensions.at(associateActions_.selected());
            QStringList viewerIds, viewers;
            QSqlQuery query;
            int viewerId = 0, index = 0;

            query.exec(QString("SELECT viewers, viewer FROM associations WHERE extension = '%1'").arg(extension));
            if (query.next())
            {
                viewerIds = query.value(0).toString().split(';');
                viewerId = query.value(1).toInt();
            }

            for (int i = 0; i < viewerIds.count(); i++)
            {
                int numId = viewerIds.at(i).toInt();
                query.exec(QString("SELECT name FROM applications WHERE id = %1").arg(numId));
                if (query.next())
                {
                    viewers.append(query.value(0).toString());
                    if (numId == viewerId)
                    {
                        index = i;
                    }
                }
            }

            AssociationDialog associationDialog(extension, viewers, index, this);
            int selected = associationDialog.exec();
            if (selected >= 0)
            {
                query.exec(QString("UPDATE associations SET viewer = %1 WHERE extension = '%2'")
                                  .arg(viewerIds.at(selected))
                                  .arg(extension));

                if (query.numRowsAffected() > 0)
                {
                    qDebug() << "Associated" << extension << "with" << viewers.at(selected);
                }
            }
        }
        else if (group == settingsActions_.category())
        {
            switch (settingsActions_.selected())
            {
            case SET_TZ:
            {
                TimeZoneDialog dialog(this);
                if (dialog.popup("Select Time Zone") == QDialog::Accepted)
                {
                    if (SystemConfig::setTimezone(dialog.selectedTimeZone()))
                    {
                        qDebug() << "set time zone:" << dialog.selectedTimeZone();
                    }
                }
                break;
            }
            case SET_PWR_MGMT:
            {
                LegacyPowerManagementDialog dialog(this, SysStatus::instance());
                dialog.exec();
                break;
            }
            case SET_CALIBRATE:
                run("/opt/onyx/arm/bin/mouse_calibration", QStringList());
                return;
            case SET_DEFAULTS:
            {
                // Confirmation dialog
                MessageDialog defaults(QMessageBox::Icon(QMessageBox::Warning) , tr("Default Categories"),
                                       tr("Do you want to restore the default categories and their content?"),
                                       QMessageBox::Yes | QMessageBox::No);

                if (defaults.exec() == QMessageBox::Yes)
                {
                    DatabaseUtils::clearDatabase(QStringList() << "revision" << "books" << "music");
                    obx::initializeDatabase();
                    qDebug() << "default database restored";
                    selected_row_.clear();
                    level_ = 0;
                    handler_type_ = HDLR_HOME;
                    current_path_ = QString();
                    showHome(0);
                    return;
                }
                break;
            }
            case SET_ABOUT:
            {
                AboutDialog aboutDialog(mainUI_, this);
                aboutDialog.exec();
                break;
            }
//            case SET_RESUME_LAST:
//            {
//
// 			  }
            default:
                break;
            }
        }

        refresh = true;
    }

    QApplication::processEvents();
    status_bar_.repaint();
    onyx::screen::instance().updateWidget(&status_bar_, onyx::screen::ScreenProxy::GU,
                                          false, onyx::screen::ScreenCommand::WAIT_ALL);
    if (refresh)
    {
        onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GC);
    }
}

void ExplorerView::onProgressClicked(int, int page)
{
    treeview_.jumpToPage(page);
}

void ExplorerView::keyPressEvent(QKeyEvent *ke)
{
    ke->accept();
}

void ExplorerView::keyReleaseEvent(QKeyEvent *ke)
{
    QModelIndex index = model_.index(treeview_.selected(), 0);
    QStandardItem *item = 0;
    if (index.isValid())
    {
        item = model_.itemFromIndex(index);
    }

    if (organize_mode_)
    {
        if (item)
        {
            switch(ke->key())
            {
            case Qt::Key_Left:
            case Qt::Key_Right:
            {
                bool visible = (ke->key() == Qt::Key_Left ? 1 : 0);

                if (visible != item->isSelectable())
                {
                    QSqlQuery query(QString("UPDATE categories SET visible = %1 WHERE id = %2")
                                           .arg(visible)
                                           .arg(item->data().toStringList()[0].toInt()));

                    if (query.numRowsAffected() > 0)
                    {
                        waveform_ = onyx::screen::ScreenProxy::GU;
                        organizeCategories(treeview_.selected());
                    }
                }
                break;
            }
            case Qt::Key_PageUp:
            case Qt::Key_PageDown:
            case Qt::Key_Up:
            case Qt::Key_Down:
                treeview_.keyReleaseEvent(ke);
                break;
            case Qt::Key_Return:
                if (treeview_.selected())
                {
                    int id;
                    int position = item->data().toStringList()[1].toInt();

                    QSqlQuery query;
                    query.exec(QString("SELECT id FROM categories WHERE position = %1").arg(position - 1));
                    if (query.first())
                    {
                        id = query.value(0).toInt();
                        query.exec(QString("UPDATE categories SET position = %1 WHERE id = %2")
                                           .arg(position - 1)
                                           .arg(item->data().toStringList()[0].toInt()));
                        query.exec(QString("UPDATE categories SET position = %1 WHERE id = %2")
                                           .arg(position)
                                           .arg(id));
                    }

                    if (query.numRowsAffected() > 0)
                    {
                        waveform_ = onyx::screen::ScreenProxy::GU;
                        organizeCategories(treeview_.selected() - 1);
                    }
                }
                break;
            case Qt::Key_Escape:
                organize_mode_ = false;
                waveform_ = onyx::screen::ScreenProxy::GC;
                selected_row_.clear();
                level_ = 0;
                handler_type_ = HDLR_HOME;
                current_path_ = QString();
                showHome(0);
                break;
            case Qt::Key_Menu:
            default:
                break;
            }
        }
    }
    else
    {
        switch(ke->key())
        {
        case Qt::Key_Right:
        {
            if (handler_type_ == HDLR_MUSIC)
            {
                addToPlaylist(item);
            }
            break;
        }
        case Qt::Key_PageUp:
        case Qt::Key_PageDown:
        case Qt::Key_Up:
        case Qt::Key_Down:
        case Qt::Key_Return:
            treeview_.keyReleaseEvent(ke);
            break;
        case Qt::Key_Left:
        {
            if (level_ > int(!homeSkipped_))
            {
                int row = 0;
                if (--level_ < selected_row_.size())
                {
                    row = selected_row_.at(level_);
                }

                current_path_.chop(current_path_.size() - current_path_.lastIndexOf('/'));

                if (handler_type_ == HDLR_FILES)
                {
                    showFiles(category_id_, current_path_, row);
                    break;
                }
                else if (handler_type_ == HDLR_BOOKS)
                {
                    showDBViews(category_id_, current_path_, row);
                    break;
                }
                else if (handler_type_ == HDLR_MUSIC)
                {
                    showDBViews(category_id_, current_path_, row, QString(qgetenv("HOME")) + "/.mpd/music/");
                    break;
                }
            }
            // fall through
        }
        case Qt::Key_Escape:
        {
            if (level_ == 0)
            {
                if (ke->key() == Qt::Key_Escape && !mainUI_)
                {
                    qApp->exit();
                }
                break;
            }

            int row = selected_row_.front();
            level_ = 0;
            handler_type_ = HDLR_HOME;
            if (homeSkipped_)
            {
                current_path_ = root_path_;
            }
            showHome(row);
            break;
        }
        case Qt::Key_Menu:
            popupMenu();
            break;
        default:
            break;
        }
    }
}

ExplorerSplash::ExplorerSplash(QPixmap pixmap, QWidget *parent)
    : QWidget(parent, Qt::FramelessWindowHint)
{
    pixmap_ = pixmap;
    resize(qApp->desktop()->screenGeometry().size());
}

ExplorerSplash::~ExplorerSplash()
{
}

void ExplorerSplash::paintEvent(QPaintEvent *)
{
    QPainter image(this);

    // Adjust to currently used rotation
    switch (SysStatus::instance().screenTransformation())
    {
    case 90:
        image.translate(800, 0);
        break;
    case 180:
        image.translate(800, 600);
        break;
    case 270:
        image.translate(0, 600);
        break;
    case 0:
    default:
        break;
    }
    image.rotate(qreal(SysStatus::instance().screenTransformation()));
    image.drawPixmap(0, 0, pixmap_);
}

}
