
#ifndef FINDWIDGET_H
#define FINDWIDGET_H

#include <include/GPG4USB.h>

#include "editorpage.h"

#include <QWidget>

/**
 * @brief Class for handling the find widget shown at buttom of a textedit-page
 */
class FindWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief
     *
     * @param parent The parent widget
     */
    explicit FindWidget(QWidget *parent, QTextEdit *edit);

private:
    void keyPressEvent( QKeyEvent* e );
    /**
     * @details Set background of findEdit to red, if no match is found (Documents textcursor position equals -1),
     *          otherwise set it to white.
     */
    void setBackground();

    QTextEdit *mTextpage; /** Textedit associated to the notification */
    QLineEdit *findEdit; /** Label holding the text shown in verifyNotification */
    QTextCharFormat cursorFormat;

private slots:
    void slotFindNext();
    void slotFindPrevious();
    void slotFind();
    void slotClose();
};
#endif // FINDWIDGET_H
