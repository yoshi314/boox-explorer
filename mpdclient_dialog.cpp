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
#include <onyx/ui/keyboard_navigator.h>

#include "mpdclient_dialog.h"

namespace obx
{

static const QString PAGE_BUTTON_STYLE =   "\
QPushButton                             \
{                                       \
    background: transparent;            \
    font-size: 14px;                    \
    border-width: 1px;                  \
    border-color: transparent;          \
    border-style: solid;                \
    color: black;                       \
    padding: 0px;                       \
}                                       \
QPushButton:pressed                     \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    border-color: black;                \
    background-color: black;            \
}                                       \
QPushButton:focus {                     \
    border-width: 2px;                  \
    border-color: black;                \
    border-style: solid;                \
    border-radius: 3;                   \
}                                       \
QPushButton:checked                     \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    color: white;                       \
    border-color: black;                \
    background-color: black;            \
}                                       \
QPushButton:disabled                    \
{                                       \
    padding-left: 0px;                  \
    padding-top: 0px;                   \
    border-color: dark;                 \
    color: dark;                        \
    background-color: white;            \
}";

static QIcon loadIcon(const QString & path, QSize & size)
{
    QPixmap pixmap(path);
    size = pixmap.size();
    return QIcon(pixmap);
}

MpdClientDialog::MpdClientDialog(QWidget *parent)
    : QDialog(parent, Qt::FramelessWindowHint)
    , vbox_(this)
    , titlebar_widget_(this)
    , titlebar_layout_(&titlebar_widget_)
    , titlebar_icon_(0)
    , titlebar_label_(0)
    , close_button_("", 0)
    , title_layout_(0)
    , title_icon_(0)
    , title_label_(0)
    , artist_layout_(0)
    , artist_icon_(0)
    , artist_label_(0)
    , hor_separator_(this)
    , button_widget_(this)
    , button_layout_(&button_widget_)
    , prev_button_("", 0)
    , play_button_("", 0)
    , stop_button_("", 0)
    , next_button_("", 0)
    , mode_button_("", 0)
{
    QIcon icon;
    QSize icon_size;

    setModal(true);

    setAutoFillBackground(true);
    setBackgroundRole(QPalette::Base);

    titlebar_icon_.setPixmap(QPixmap(":/images/music_player_small.png"));
    QFont titlebar_font(titlebar_label_.font());
    titlebar_font.setPointSize(25);
    titlebar_label_.setFont(titlebar_font);
    titlebar_label_.setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    titlebar_label_.setIndent(10);
    titlebar_label_.setText(QCoreApplication::tr("Music Player"));

    icon = loadIcon(":/images/close.png", icon_size);
    close_button_.setStyleSheet(PAGE_BUTTON_STYLE);
    close_button_.setIconSize(icon_size);
    close_button_.setIcon(icon);
    close_button_.setFocusPolicy(Qt::NoFocus);

    titlebar_layout_.setSpacing(2);
    titlebar_layout_.setContentsMargins(5, 0, 5, 0);
    titlebar_layout_.addWidget(&titlebar_icon_, 10);
    titlebar_layout_.addWidget(&titlebar_label_, 300);
    titlebar_layout_.addWidget(&close_button_);

    titlebar_widget_.setAutoFillBackground(true);
    titlebar_widget_.setBackgroundRole(QPalette::Dark);
    titlebar_widget_.setContentsMargins(0, 0, 0, 0);
    titlebar_widget_.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    title_icon_.setPixmap(QPixmap(":/images/title.png"));
    QFont title_font(title_label_.font());
    title_font.setPointSize(28);
    title_label_.setFont(title_font);
    title_label_.setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    title_label_.setContentsMargins(10, 0, 5, 0);
    title_label_.setMaximumWidth(400);

    title_layout_.setSpacing(2);
    title_layout_.setContentsMargins(5, 5, 5, 0);
    title_layout_.addWidget(&title_icon_, 10);
    title_layout_.addWidget(&title_label_, 200);

    artist_icon_.setPixmap(QPixmap(":/images/artist.png"));
    QFont artist_font(artist_label_.font());
    artist_font.setPointSize(20);
    artist_label_.setFont(artist_font);
    artist_label_.setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    artist_label_.setContentsMargins(10, 0, 5, 0);
    artist_label_.setMaximumWidth(400);

    artist_layout_.setSpacing(2);
    artist_layout_.setContentsMargins(5, 0, 5, 0);
    artist_layout_.addWidget(&artist_icon_, 10);
    artist_layout_.addWidget(&artist_label_, 200);

    album_icon_.setPixmap(QPixmap(":/images/album.png"));
    QFont album_font(album_label_.font());
    album_font.setPointSize(20);
    album_label_.setFont(album_font);
    album_label_.setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    album_label_.setContentsMargins(10, 0, 5, 0);
    album_label_.setMaximumWidth(400);

    album_layout_.setSpacing(2);
    album_layout_.setContentsMargins(5, 0, 5, 5);
    album_layout_.addWidget(&album_icon_, 10);
    album_layout_.addWidget(&album_label_, 200);

    progress_bar_.setMinimum(0);
    progress_bar_.setMaximum(0);
    progress_bar_.setContentsMargins(2, 1, 2, 1);

    QFont progress_font(progress_label_.font());
    progress_font.setPointSize(12);
    progress_font.setBold(true);
    progress_label_.setFont(progress_font);
    progress_label_.setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
    progress_label_.setIndent(10);

    progress_layout_.setSpacing(2);
    progress_layout_.setContentsMargins(5, 0, 5, 5);
    progress_layout_.addWidget(&progress_bar_);
    progress_layout_.addWidget(&progress_label_);

    hor_separator_.setFixedHeight(1);
    hor_separator_.setFocusPolicy(Qt::NoFocus);
    hor_separator_.setFrameShape(QFrame::HLine);
    hor_separator_.setAutoFillBackground(true);
    hor_separator_.setBackgroundRole(QPalette::Light);

    icon = loadIcon(":/images/play_prev.png", icon_size);
    prev_button_.setStyleSheet(PAGE_BUTTON_STYLE);
    prev_button_.setIconSize(icon_size);
    prev_button_.setIcon(icon);
    prev_button_.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    prev_button_.setFocusPolicy(Qt::TabFocus);

    play_button_.setStyleSheet(PAGE_BUTTON_STYLE);
    play_button_.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    play_button_.setFocusPolicy(Qt::TabFocus);

    stop_button_.setStyleSheet(PAGE_BUTTON_STYLE);
    stop_button_.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    stop_button_.setFocusPolicy(Qt::TabFocus);

    icon = loadIcon(":/images/play_next.png", icon_size);
    next_button_.setStyleSheet(PAGE_BUTTON_STYLE);
    next_button_.setIconSize(icon_size);
    next_button_.setIcon(icon);
    next_button_.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    next_button_.setFocusPolicy(Qt::TabFocus);

    mode_button_.setStyleSheet(PAGE_BUTTON_STYLE);
    mode_button_.setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    mode_button_.setFocusPolicy(Qt::TabFocus);

    button_layout_.setSpacing(2);
    button_layout_.setContentsMargins(5, 5, 5, 5);
    button_layout_.addStretch();
    button_layout_.addWidget(&prev_button_);
    button_layout_.addWidget(&play_button_);
    button_layout_.addWidget(&stop_button_);
    button_layout_.addWidget(&next_button_);
    button_layout_.addWidget(&mode_button_);
    button_layout_.addStretch();

    button_widget_.setContentsMargins(0, 0, 0, 0);
    button_widget_.setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    vbox_.setSpacing(0);
    vbox_.setContentsMargins(0, 0, 0, 0);
    vbox_.addWidget(&titlebar_widget_);
    vbox_.addLayout(&title_layout_);
    vbox_.addLayout(&artist_layout_);
    vbox_.addLayout(&album_layout_);
    vbox_.addLayout(&progress_layout_);
    vbox_.addWidget(&hor_separator_);
    vbox_.addWidget(&button_widget_);

    connect(&close_button_, SIGNAL(clicked()), this, SLOT(done()));
    connect(&prev_button_, SIGNAL(clicked()), this, SLOT(onPrevClicked()));
    connect(&play_button_, SIGNAL(clicked()), this, SLOT(onPlayClicked()));
    connect(&stop_button_, SIGNAL(clicked()), this, SLOT(onStopClicked()));
    connect(&next_button_, SIGNAL(clicked()), this, SLOT(onNextClicked()));
    connect(&mode_button_, SIGNAL(clicked()), this, SLOT(onModeClicked()));

    timer_ = new QTimer(this);
    timer_->setSingleShot(false);
    timer_->setInterval(2 * 1000);

    QMpdClient &mpdClient = QMpdClient::instance();
    connect(&mpdClient, SIGNAL(stateChanged(QMpdStatus::State)), this, SLOT(onStateChanged(QMpdStatus::State)));
    connect(&mpdClient, SIGNAL(modeChanged(QMpdStatus::Mode)), this, SLOT(onModeChanged(QMpdStatus::Mode)));
    connect(&mpdClient, SIGNAL(songChanged(const QMpdSong &)), this, SLOT(onSongChanged(const QMpdSong &)));
    connect(&mpdClient, SIGNAL(elapsedSecondsAtStatusChange(int)), this, SLOT(onElapsedSecondsAtStatusChange(int)));
    connect(timer_, SIGNAL(timeout()), this, SLOT(onTimeout()));

    onStateChanged(mpdClient.status().state());
    onModeChanged(mpdClient.status().mode());
    onSongChanged(mpdClient.song());
    onElapsedSecondsAtStatusChange(mpdClient.status().elapsedSeconds());

    show();
    sys::SysStatus::instance().setSystemBusy(false);
    onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GC, false);
}

MpdClientDialog::~MpdClientDialog()
{
}

void MpdClientDialog::onPrevClicked()
{
    QMpdClient::instance().previous();
}

void MpdClientDialog::onPlayClicked()
{
    if (state_ == QMpdStatus::StatePlay)
    {
        QMpdClient::instance().pause();
    }
    else
    {
        QMpdClient::instance().play();
    }
}

void MpdClientDialog::onStopClicked()
{
    if (state_ == QMpdStatus::StateStop)
    {
        QMpdClient::instance().clearPlaylist();
    }
    else
    {
        QMpdClient::instance().stop();
    }
}

void MpdClientDialog::onNextClicked()
{
    QMpdClient::instance().next();
}

void MpdClientDialog::onModeClicked()
{
    switch (mode_)
    {
    case QMpdStatus::ModeNormal:
        QMpdClient::instance().setMode(QMpdStatus::ModeRepeat);
        break;
    case QMpdStatus::ModeRepeat:
        QMpdClient::instance().setMode(QMpdStatus::ModeRandomRepeat);
        break;
    case QMpdStatus::ModeRandom:
    case QMpdStatus::ModeRandomRepeat:
        QMpdClient::instance().setMode(QMpdStatus::ModeNormal);
        break;
    default:
        break;
    }
}

void MpdClientDialog::onStateChanged(QMpdStatus::State state)
{
    QIcon icon;
    QSize icon_size;

    state_ = state;

    if (state == QMpdStatus::StatePlay)
    {
        icon = loadIcon(":/images/pause.png", icon_size);
        timer_->start();
    }
    else
    {
        icon = loadIcon(":/images/play.png", icon_size);
        timer_->stop();
    }

    updateButtonIcon(play_button_, icon, icon_size);

    if (state == QMpdStatus::StateStop)
    {
        icon = loadIcon(":/images/clear_playlist.png", icon_size);
    }
    else
    {
        icon = loadIcon(":/images/stop.png", icon_size);
    }

    updateButtonIcon(stop_button_, icon, icon_size);

    onyx::screen::instance().flush(&play_button_, onyx::screen::ScreenProxy::GU);
    onyx::screen::instance().flush(&stop_button_, onyx::screen::ScreenProxy::GU);
}

void MpdClientDialog::onModeChanged(QMpdStatus::Mode mode)
{
    QIcon icon;
    QSize icon_size;

    const QStringList modeName = QStringList() << "Unknown" << "Normal" << "Repeat" << "Random" << "RandomRepeat";
    qDebug() << "mode:" << modeName[mode];

    mode_ = mode;

    switch (mode)
    {
    case QMpdStatus::ModeNormal:
        icon = loadIcon(":/images/repeat.png", icon_size);
        break;
    case QMpdStatus::ModeRepeat:
        icon = loadIcon(":/images/random.png", icon_size);
        break;
    case QMpdStatus::ModeRandom:
    case QMpdStatus::ModeRandomRepeat:
        icon = loadIcon(":/images/normal.png", icon_size);
        break;
    default:
        break;
    }

    bool visable = mode_button_.isVisible();
    bool focus = mode_button_.hasFocus();
    if (visable)
    {
        mode_button_.hide();
    }
    mode_button_.setIconSize(icon_size);
    mode_button_.setIcon(icon);
    if (visable)
    {
        mode_button_.show();
        if (focus)
        {
            mode_button_.setFocus();
        }

        onyx::screen::instance().flush(&mode_button_, onyx::screen::ScreenProxy::GU);
    }
}

void MpdClientDialog::onSongChanged(const QMpdSong &song)
{
    title_label_.setText(song.title());
    artist_label_.setText(song.artist());
    album_label_.setText(song.album());

    progress_bar_.setMaximum(song.duration());
    progress_label_.setText(song.totalTime().toString("m:ss"));

    onyx::screen::instance().flush(this, onyx::screen::ScreenProxy::GU);
}

void MpdClientDialog::onElapsedSecondsAtStatusChange(int elapsedSeconds)
{
    qDebug() << "elapsedSeconds:" << elapsedSeconds;
    elapsedSeconds_ = elapsedSeconds;
    timedSeconds_ = 0;

    updateElapsedTime();
}

void MpdClientDialog::onTimeout()
{
    timedSeconds_ += 2;
    updateElapsedTime();
}

void MpdClientDialog::done()
{
    int state;

    if (title_label_.text().isEmpty())
    {
        state = STOP_PLAYER;
    }
    else
    {
        const int musicPlayerState[] = { STOP_PLAYER, MUSIC_STOPPED, MUSIC_PLAYING, MUSIC_PAUSED };
        state = musicPlayerState[state_];
    }

    QDialog::done(state);
}

void MpdClientDialog::updateButtonIcon(OnyxPushButton &button, const QIcon &icon, const QSize &icon_size)
{
    bool visable = button.isVisible();
    bool focus = button.hasFocus();
    if (visable)
    {
        button.hide();
    }
    button.setIconSize(icon_size);
    button.setIcon(icon);
    if (visable)
    {
        button.show();
        if (focus)
        {
            button.setFocus();
        }
    }
}

void MpdClientDialog::updateElapsedTime()
{
    int elapsedSeconds = elapsedSeconds_ + timedSeconds_;
    QTime elapsedTime = QTime(0, 0).addSecs(elapsedSeconds);

    progress_bar_.setValue(elapsedSeconds);
    progress_bar_.setFormat(elapsedTime.toString("m:ss"));

    onyx::screen::instance().updateWidget(&progress_bar_, onyx::screen::ScreenProxy::GU,
                                          false, onyx::screen::ScreenCommand::WAIT_NONE);
}

void MpdClientDialog::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    QPen pen(Qt::SolidLine);
    pen.setColor(Qt::black);
    p.setPen(pen);
    p.drawRect(QRect(QPoint(0, 0), size()).adjusted(0, 0, -1, -1));

    QRect size1 = title_layout_.contentsRect();
    QRect size2 = album_layout_.contentsRect();

    QRect border = size1.united(size2);
    border.adjust(0, 0, 0, 0);
    p.drawRoundedRect(border, 3, 3);
}

void MpdClientDialog::resizeEvent(QResizeEvent*)
{
    QSize screenSize = qApp->desktop()->screenGeometry().size();
    QSize windowSize = size();
    QSize offset = (screenSize - windowSize) / 2;
    setGeometry(offset.width(), offset.height(), windowSize.width(), windowSize.height());
}

void MpdClientDialog::keyPressEvent(QKeyEvent *ke)
{
    ke->accept();
}

void MpdClientDialog::keyReleaseEvent(QKeyEvent *ke)
{
    switch(ke->key())
    {
    case Qt::Key_Left:
    case Qt::Key_Right:
    {
        QWidget *wnd = ui::moveFocus(&button_widget_, ke->key());
        if (wnd)
        {
            wnd->setFocus();
            onyx::screen::instance().flush();
            onyx::screen::instance().updateWidget(&button_widget_, onyx::screen::ScreenProxy::DW, false);
        }
        break;
    }
    case Qt::Key_Escape:
        done();
        break;
    case Qt::Key_Return:
    case Qt::Key_PageDown:
    case Qt::Key_PageUp:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Menu:
    default:
        break;
    }
}

}
