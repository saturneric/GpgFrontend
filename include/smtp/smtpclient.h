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

#ifndef SMTPCLIENT_H
#define SMTPCLIENT_H

#include <QObject>
#include <QtNetwork/QSslSocket>

#include "mimemessage.h"
#include "smtpexports.h"

class SMTP_EXPORT SmtpClient : public QObject {
Q_OBJECT
public:

    /* [0] Enumerations */

    enum AuthMethod {
        AuthPlain,
        AuthLogin
    };

    enum SmtpError {
        ConnectionTimeoutError,
        ResponseTimeoutError,
        SendDataTimeoutError,
        AuthenticationFailedError,
        ServerError,    // 4xx smtp error
        ClientError     // 5xx smtp error
    };

    enum ConnectionType {
        TcpConnection,
        SslConnection,
        TlsConnection       // STARTTLS
    };

    /* [0] --- */


    /* [1] Constructors and Destructors */

    explicit SmtpClient(const QString &host = "localhost", int port = 25, ConnectionType ct = TcpConnection);

    ~SmtpClient() override;

    /* [1] --- */


    /* [2] Getters and Setters */

    [[nodiscard]] const QString &getHost() const;

    void setHost(const QString &host);

    [[nodiscard]] int getPort() const;

    void setPort(int port);

    [[nodiscard]] const QString &getName() const;

    void setName(const QString &name);

    [[nodiscard]] ConnectionType getConnectionType() const;

    void setConnectionType(ConnectionType ct);

    [[nodiscard]] const QString &getUser() const;

    void setUser(const QString &user);

    [[nodiscard]] const QString &getPassword() const;

    void setPassword(const QString &password);

    [[nodiscard]] SmtpClient::AuthMethod getAuthMethod() const;

    void setAuthMethod(AuthMethod method);

    [[nodiscard]] const QString &getResponseText() const;

    [[nodiscard]] int getResponseCode() const;

    [[nodiscard]] int getConnectionTimeout() const;

    void setConnectionTimeout(int msec);

    [[nodiscard]] int getResponseTimeout() const;

    void setResponseTimeout(int msec);

    [[nodiscard]] int getSendMessageTimeout() const;

    void setSendMessageTimeout(int msec);

    QTcpSocket *getSocket();


    /* [2] --- */


    /* [3] Public methods */

    bool connectToHost();

    bool login();

    bool login(const QString &user, const QString &password, AuthMethod method = AuthLogin);

    bool sendMail(MimeMessage &email);

    void quit();


    /* [3] --- */

protected:

    /* [4] Protected members */

    QTcpSocket *socket;

    QString host;
    int port;
    ConnectionType connectionType;
    QString name;

    QString user;
    QString password;
    AuthMethod authMethod;

    int connectionTimeout;
    int responseTimeout;
    int sendMessageTimeout;


    QString responseText;
    int responseCode;


    class ResponseTimeoutException : public std::exception {
    };

    class SendMessageTimeoutException : public std::exception {
    };

    /* [4] --- */


    /* [5] Protected methods */

    void waitForResponse();

    void sendMessage(const QString &text);

    /* [5] --- */

protected slots:

    /* [6] Protected slots */

    void socketStateChanged(QAbstractSocket::SocketState state);

    void socketError(QAbstractSocket::SocketError error);

    void socketReadyRead();

    /* [6] --- */


signals:

    /* [7] Signals */

    void smtpError(SmtpClient::SmtpError e);

    /* [7] --- */

};

#endif // SMTPCLIENT_H
