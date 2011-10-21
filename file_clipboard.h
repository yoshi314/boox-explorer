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

#ifndef FILE_CLIPBOARD_H_
#define FILE_CLIPBOARD_H_

class QString;
class QFileInfo;

namespace obx
{

class FileClipboard
{
public:
    FileClipboard();
    ~FileClipboard();

public:
    void cut(const QString &absoluteFilePath);
    void copy(const QString &absoluteFilePath);
    bool paste(const QString &absolutePath);
    void clear();
    bool isEmpty();
    bool isClippedToMove();
    bool holdsDir();
    QString fileName();
    QFileInfo fileInfo();

private:
    typedef enum { ACT_NONE = -1, ACT_COPY, ACT_MOVE } ActionType;
    typedef bool (*ActionFunction)( const QString &, const QString &);

    static ActionType action_;
    static QFileInfo fileInfo_;
};

}

#endif // FILE_CLIPBOARD_H_
