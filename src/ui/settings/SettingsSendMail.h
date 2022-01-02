//
// Created by saturneric on 2021/11/28.
//

#ifndef GPGFRONTEND_SETTINGSSENDMAIL_H
#define GPGFRONTEND_SETTINGSSENDMAIL_H

#include "smtp/SmtpMime"
#include "ui/GpgFrontendUI.h"

class Ui_SendMailSettings;

namespace GpgFrontend::UI {
class SendMailTab : public QWidget {
  Q_OBJECT

 public:
  explicit SendMailTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private slots:

  void slotTestSMTPConnectionResult(const QString& result);

#ifdef SMTP_SUPPORT
  void slotCheckConnection();

  void slotSendTestMail();
#endif

 private:
  std::shared_ptr<Ui_SendMailSettings> ui;
  QRegularExpression re_email{
      R"((?:[a-z0-9!#$%&'*+/=?^_`{|}~-]+(?:\.[a-z0-9!#$%&'*+/=?^_`{|}~-]+)*|"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])*")@(?:(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\.)+[a-z0-9](?:[a-z0-9-]*[a-z0-9])?|\[(?:(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9]))\.){3}(?:(2(5[0-5]|[0-4][0-9])|1[0-9][0-9]|[1-9]?[0-9])|[a-z0-9-]*[a-z0-9]:(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\[\x01-\x09\x0b\x0c\x0e-\x7f])+)\]))"};
  SmtpClient::ConnectionType connection_type_ =
      SmtpClient::ConnectionType::TcpConnection;
 signals:

  void signalRestartNeeded(bool needed);
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSSENDMAIL_H
