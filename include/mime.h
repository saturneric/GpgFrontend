/*
 *      mime.h
 *
 *      Copyright 2008 gpg4usb-team <gpg4usb@cpunk.de>
 *
 *      This file is part of gpg4usb.
 *
 *      Gpg4usb is free software: you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation, either version 3 of the License, or
 *      (at your option) any later version.
 *
 *      Gpg4usb is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with gpg4usb.  If not, see <http://www.gnu.org/licenses/>
 */

#ifndef __MIME_H__
#define __MIME_H__

#include <GPG4USB.h>

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
