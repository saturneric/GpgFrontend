/*
 *      mime.cpp
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

/***
 * quotedPrintableDecode copied from KCodecs, where it is stated:

   The quoted-printable codec as described in RFC 2045, section 6.7. is by
   Rik Hemsley (C) 2001.

 */

/*    TODO: proper import / copyright statement
 *
 */

#include "mime.h"

Mime::Mime(QByteArray *message)
{
    splitParts(message);
    /*
    mMessage = message;
    int bStart = mMessage->indexOf("boundary=\"") + 10 ;
    int bEnd = mMessage->indexOf("\"\n", bStart );

    qDebug() << "bStart: " << bStart << " bEnd: " << bEnd;
    mBoundary = new QByteArray(mMessage->mid(bStart, bEnd - bStart));
    qDebug() << "boundary: " << *mBoundary;

    Part *p1 = new Part();

    int nb = mMessage->indexOf(*mBoundary, bEnd) + mBoundary->length() +1 ;
    qDebug() << "nb: " << nb;
    int eh = mMessage->indexOf("\n\n", nb);
    qDebug() << "eh: " << eh;
    QByteArray *header = new QByteArray(mMessage->mid(nb , eh - nb));
    qDebug() << "header:" << header;

    // split header at newlines
    foreach(QByteArray tmp , header->split(* "\n")) {
    // split lines at :
    QList<QByteArray> tmp2 = tmp.split(* ":");
    p1->header.insert(QString(tmp2[0].trimmed()), QString(tmp2[1].trimmed()));
    }

    QHashIterator<QString, QString> i(p1->header);
    while (i.hasNext()) {
       i.next();
       qDebug() << "found: " << i.key() << ":" << i.value() << endl;
    }

    int nb2 = mMessage->indexOf(*mBoundary, eh);

    p1->body = mMessage->mid(eh , nb2 - eh);

    QTextCodec *codec = QTextCodec::codecForName("ISO-8859-15");
    QString qs = codec->toUnicode(p1->body);
    qDebug() << "body: " << qs;
    */
}

Mime::~Mime()
{

}

void Mime::splitParts(QByteArray *message)
{
    int pos1, pos2, headEnd;
    MimePart p_tmp;

    // find the boundary
    pos1 = message->indexOf("boundary=\"") + 10 ;
    pos2 = message->indexOf("\"\n", pos1);
    QByteArray boundary = message->mid(pos1, pos2 - pos1);
    //qDebug() << "boundary: " << boundary;

    while (pos2 > pos1) {

        pos1 = message->indexOf(boundary, pos2) + boundary.length() + 1 ;
        headEnd = message->indexOf("\n\n", pos1);
        if (headEnd < 0)
            break;
        QByteArray header = message->mid(pos1 , headEnd - pos1);

        p_tmp.header = parseHeader(&header);

        pos2 = message->indexOf(boundary, headEnd);
        p_tmp.body = message->mid(headEnd , pos2 - headEnd);

        mPartList.append(p_tmp);
    }
}

Header Mime::parseHeader(QByteArray *header)
{

    QList<HeadElem> ret;

    /** http://www.aspnetmime.com/help/welcome/overviewmimeii.html :
     * If a line starts with any white space, that line is said to be 'folded' and is actually
     * part of the header above it.
     */
    header->replace("\n ", " ");

    //split header at newlines
    foreach(QByteArray line , header->split(* "\n")) {
        HeadElem elem;
        //split lines at :
        QList<QByteArray> tmp2 = line.split(* ":");
        elem.name = tmp2[0].trimmed();
        if (tmp2[1].contains(';')) {
            // split lines at ;
            // TODO: what if ; is inside ""
            QList<QByteArray> tmp3 = tmp2[1].split(* ";");
            elem.value = QString(tmp3.takeFirst().trimmed());
            foreach(QByteArray tmp4, tmp3) {
                QList<QByteArray> tmp5 = tmp4.split(* "=");
                elem.params.insert(QString(tmp5[0].trimmed()), QString(tmp5[1].trimmed()));
            }
        } else {
            elem.value = tmp2[1].trimmed();
        }
        ret.append(elem);
    }
    return Header(ret);
}

Header Mime::getHeader(const QByteArray *message) {
    int headEnd = message->indexOf("\n\n");
    QByteArray header = message->mid(0, headEnd);
    return parseHeader(&header);
}

bool Mime::isMultipart(QByteArray *message)
{
    return message->startsWith("Content-Type: multipart/mixed;");
}

/**
 * if Content-Type is specified, it should be mime
 *
 */
bool Mime::isMime(const QByteArray *message)
{
    return message->startsWith("Content-Type:");
}

/***
 * quotedPrintableDecode copied from KCodecs, where it is stated:

   The quoted-printable codec as described in RFC 2045, section 6.7. is by
   Rik Hemsley (C) 2001.

 */

static const char hexChars[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

/******************************** KCodecs ********************************/
// strchr(3) for broken systems.
static int rikFindChar(register const char * _s, const char c)
{
    register const char * s = _s;

    while (true) {
        if ((0 == *s) || (c == *s)) break; ++s;
        if ((0 == *s) || (c == *s)) break; ++s;
        if ((0 == *s) || (c == *s)) break; ++s;
        if ((0 == *s) || (c == *s)) break; ++s;
    }

    return s - _s;
}

void Mime::quotedPrintableDecode(const QByteArray& in, QByteArray& out)
{
    // clear out the output buffer
    out.resize(0);
    if (in.isEmpty())
        return;

    char *cursor;
    const char *data;
    const unsigned int length = in.size();

    data = in.data();
    out.resize(length);
    cursor = out.data();

    for (unsigned int i = 0; i < length; i++) {
        char c(in[i]);

        if ('=' == c) {
            if (i < length - 2) {
                char c1 = in[i + 1];
                char c2 = in[i + 2];

                if (('\n' == c1) || ('\r' == c1 && '\n' == c2)) {
                    // Soft line break. No output.
                    if ('\r' == c1)
                        i += 2;        // CRLF line breaks
                    else
                        i += 1;
                } else {
                    // =XX encoded byte.

                    int hexChar0 = rikFindChar(hexChars, c1);
                    int hexChar1 = rikFindChar(hexChars, c2);

                    if (hexChar0 < 16 && hexChar1 < 16) {
                        *cursor++ = char((hexChar0 * 16) | hexChar1);
                        i += 2;
                    }
                }
            }
        } else {
            *cursor++ = c;
        }
    }

    out.truncate(cursor - out.data());
}
