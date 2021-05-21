/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */

#ifndef __MIME_H__
#define __MIME_H__

#include <GpgFrontend.h>
#include <utility>

class HeadElem {
public:
    QString name;
    QString value;
    QHash<QString, QString> params;

    /*    QDataStream & operator<<(QDataStream& Stream, const HeadElem& H)
        {
            return Stream << H.name << " : " << H.value;
        }*/

};

class Header {
public:
    QList<HeadElem> headElems;

    Header() = default;

    explicit Header(QList<HeadElem> heads) {
        headElems = std::move(heads);
    }

    [[maybe_unused]] void setHeader(QList<HeadElem> heads) {
        headElems = std::move(heads);
    }

    QString getValue(const QString &key) {
                foreach(HeadElem tmp, headElems) {
                //qDebug() << "gv: " << tmp.name << ":" << tmp.value;
                if (tmp.name == key)
                    return tmp.value;
            }
        return "";
    }

    [[maybe_unused]] QHash<QString, QString> getParams(const QString &key) {
                foreach(HeadElem tmp, headElems) {
                //qDebug() << "gv: " << tmp.name << ":" << tmp.value;
                if (tmp.name == key)
                    //return tmp.value;
                    return tmp.params;
            }
        return *(new QHash<QString, QString>());
    }

    QString getParam(const QString &key, const QString &pKey) {
                foreach(HeadElem tmp, headElems) {
                //qDebug() << "gv: " << tmp.name << ":" << tmp.value;
                if (tmp.name == key)
                    return tmp.params.value(pKey);
            }
        return "";
    }


};

class MimePart {
public:
    Header header;
    QByteArray body;



    /*    QDataStream & operator<<(QDataStream& Stream, const Part& P)
        {
            foreach(HeadElem tmp, header) {
                Stream << tmp << "\n";
            }
            return Stream;
        }*/
};

class Mime {

public:
    explicit Mime(QByteArray *message); // Constructor
    ~Mime(); // Destructor
    [[maybe_unused]] static bool isMultipart(QByteArray *message);

    static bool isMime(const QByteArray *message);

    QList<MimePart> parts() {
        return mPartList;
    }

    void splitParts(QByteArray *message);

    static Header getHeader(const QByteArray *message);

    static Header parseHeader(QByteArray *header);

    static void quotedPrintableDecode(const QByteArray &in, QByteArray &out);

private:
    [[maybe_unused]] QByteArray *mMessage;
    [[maybe_unused]] QByteArray *mBoundary;
    QList<MimePart> mPartList;

};

#endif  // __MIME_H__
