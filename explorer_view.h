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

#ifndef EXPLORER_VIEW_H_
#define EXPLORER_VIEW_H_

#include <QtGui/QWidget>

#include "tree_view.h"
#include "file_clipboard.h"
#include "boox_action.h"

#include "onyx/screen/screen_proxy.h"
#include "onyx/ui/system_actions.h"
#include "onyx/ui/status_bar.h"

using namespace ui;

namespace obx
{

typedef enum
{
    HDLR_HOME = 0,
    HDLR_FILES,
    HDLR_BOOKS,
    HDLR_APPS,
    HDLR_WEBSITES
} HandlerType;

class ExplorerView : public QWidget
{
    Q_OBJECT

public:
    ExplorerView(bool mainUI, QWidget *parent = 0);
    ~ExplorerView();

private Q_SLOTS:
    void onItemActivated(const QModelIndex &);
    void onPositionChanged(int currentPage, int pages);
    void onAboutToSuspend();
    void onAboutToShutDown();
    void popupMenu();

private:
    void showHome(const int&);
    void showCategories(const QString &name, QSqlQuery &query, int row);
    void showFiles(int, const QString&, int);
    void showBooks(int, const QString&, int);
    void showApps(int, const QString&, int);
    void showWebsites(int, const QString&, int);
    void organizeCategories(int row);
    void onCategoryActivated(const QStringList &stringList);
    QString getByExtension(const QString &field, const QString &extension);
    QString getIconByExtension(const QFileInfo &fileInfo);
    QString getDisplayName(const QFileInfo &fileInfo);
    bool openDocument(QString fullFileName);
    void addApplication(int category, QString fullFileName);
    int  run(const QString &command, const QStringList & parameters);
    void keyPressEvent(QKeyEvent *ke);
    void keyReleaseEvent(QKeyEvent *ke);

private:
    onyx::screen::ScreenProxy::Waveform waveform_;

    QVBoxLayout        vbox_;
    QStandardItemModel model_;
    ObxTreeView        treeview_;
    StatusBar          status_bar_;
    SystemActions      systemActions_;
    BooxActions        fileActions_;
    BooxActions        organizeActions_;
    BooxActions        settingsActions_;

    bool               mainUI_;
    bool               homeSkipped_;
    HandlerType        handler_type_;
    int                category_id_;
    int                level_;
    QVector<int>       selected_row_;
    bool               organize_mode_;
    QString            root_path_;
    QString            current_path_;
    QStringList        books_query_list_;
    QIcon              books_view_icon_;
    QString            book_cover_;
    QStringList        book_extensions_;
    QStringList        icon_extensions_;

    FileClipboard      fileClipboard_;
};

class ExplorerSplash : public QWidget
{
public:
    ExplorerSplash(QPixmap pixmap, QWidget *parent = 0);
    ~ExplorerSplash();

private:
    void paintEvent(QPaintEvent *);

private:
    QPixmap pixmap_;
};

}

#endif // EXPLORER_VIEW_H_
