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

#ifndef MPDCLIENT_DIALOG_H_
#define MPDCLIENT_DIALOG_H_

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QtMpdClient/QMpdClient>

#include <onyx/ui/buttons.h>

using namespace ui;

namespace obx
{

class MpdClientDialog : public QDialog
{
    Q_OBJECT

public:
    MpdClientDialog(QWidget *parent = 0);
    virtual ~MpdClientDialog();

private Q_SLOTS:
    void onPrevClicked();
    void onPlayClicked();
    void onStopClicked();
    void onNextClicked();
    void onModeClicked();

    void onStateChanged(QMpdStatus::State state);
    void onModeChanged(QMpdStatus::Mode mode);
    void onSongChanged(const QMpdSong &song);
    void onElapsedSecondsAtStatusChange(int elapsedSeconds);
    void onTimeout();
    void done();

private:
    void updateButtonIcon(OnyxPushButton &button, const QIcon &icon, const QSize &icon_size);
    void updateElapsedTime();
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);

private:
    QVBoxLayout    vbox_;
    QWidget        titlebar_widget_;
    QHBoxLayout    titlebar_layout_;
    QLabel         titlebar_icon_;
    QLabel         titlebar_label_;
    OnyxPushButton close_button_;

    QHBoxLayout    title_layout_;
    QLabel         title_icon_;
    QLabel         title_label_;

    QHBoxLayout    artist_layout_;
    QLabel         artist_icon_;
    QLabel         artist_label_;

    QHBoxLayout    album_layout_;
    QLabel         album_icon_;
    QLabel         album_label_;

    QHBoxLayout    progress_layout_;
    QProgressBar   progress_bar_;
    QLabel         progress_label_;

    QLabel         hor_separator_;

    QWidget        button_widget_;
    QHBoxLayout    button_layout_;
    OnyxPushButton prev_button_;
    OnyxPushButton play_button_;
    OnyxPushButton stop_button_;
    OnyxPushButton next_button_;
    OnyxPushButton mode_button_;

    QTimer            *timer_;
    QMpdStatus::State state_;
    QMpdStatus::Mode  mode_;
    int               elapsedSeconds_;
    int               timedSeconds_;
};

}

#endif // MPDCLIENT_DIALOG_H_
