/*
  Copyright (c) 2011-2012 - Tőkés Attila

  This file is part of SmtpClient for Qt.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  See the LICENSE file for more details.
*/

#ifndef MIMEMESSAGE_H
#define MIMEMESSAGE_H

#include "mimepart.h"
#include "mimemultipart.h"
#include "emailaddress.h"
#include <QList>

#include "smtpexports.h"

class SMTP_EXPORT MimeMessage : public QObject {
public:

    enum RecipientType {
        To,                 // primary
        Cc,                 // carbon copy
        Bcc                 // blind carbon copy
    };

    /* [1] Constructors and Destructors */

    explicit MimeMessage(bool createAutoMimeConent = true);

    ~MimeMessage() override;

    /* [1] --- */


    /* [2] Getters and Setters */

    void setSender(EmailAddress *e);

    void addRecipient(EmailAddress *rcpt, RecipientType type = To);

    void addTo(EmailAddress *rcpt);

    void addCc(EmailAddress *rcpt);

    void addBcc(EmailAddress *rcpt);

    void setSubject(const QString &subject);

    void addPart(MimePart *part);

    void setReplyTo(EmailAddress *rto);

    void setInReplyTo(const QString &inReplyTo);

    void setHeaderEncoding(MimePart::Encoding);

    [[nodiscard]] const EmailAddress &getSender() const;

    [[nodiscard]] const QList<EmailAddress *> &getRecipients(RecipientType type = To) const;

    [[nodiscard]] const QString &getSubject() const;

    [[nodiscard]] const QList<MimePart *> &getParts() const;

    [[nodiscard]] const EmailAddress *getReplyTo() const;

    MimePart &getContent();

    void setContent(MimePart *content);
    /* [2] --- */


    /* [3] Public methods */

    virtual QString toString();

    /* [3] --- */

protected:

    /* [4] Protected members */

    EmailAddress *sender{};
    EmailAddress *replyTo;
    QList<EmailAddress *> recipientsTo, recipientsCc, recipientsBcc;
    QString subject;
    QString mInReplyTo;
    MimePart *content;
    bool autoMimeContentCreated;

    MimePart::Encoding hEncoding;

    /* [4] --- */


};

#endif // MIMEMESSAGE_H
