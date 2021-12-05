//
// Created by saturneric on 2021/11/28.
//

#ifndef GPGFRONTEND_SETTINGSSENDMAIL_H
#define GPGFRONTEND_SETTINGSSENDMAIL_H

#include "ui/GpgFrontendUI.h"

namespace GpgFrontend::UI {
class SendMailTab : public QWidget {
  Q_OBJECT

 public:
  explicit SendMailTab(QWidget* parent = nullptr);

  void setSettings();

  void applySettings();

 private slots:

  void slotCheckBoxSetEnableDisable(int state);

 private:
  QString appPath;
  QSettings settings;

  QCheckBox* enableCheckBox;

  QLineEdit* smtpAddress;
  QLineEdit* username;
  QLineEdit* password;
  QSpinBox* portSpin;
  QComboBox* connectionTypeComboBox;
  QLineEdit* defaultSender;

  QPushButton* checkConnectionButton;

 signals:

  void signalRestartNeeded(bool needed);
};
}  // namespace GpgFrontend::UI

#endif  // GPGFRONTEND_SETTINGSSENDMAIL_H
