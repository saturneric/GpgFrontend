//
// Created by eric on 2021/5/30.
//

#ifndef GPGFRONTEND_SUBKEYGENERATETHREAD_H
#define GPGFRONTEND_SUBKEYGENERATETHREAD_H

#include "gpg/GpgContext.h"

class SubkeyGenerateThread : public QThread {
    Q_OBJECT

public:
    SubkeyGenerateThread(GpgKey key, GenKeyInfo *keyGenParams, GpgME::GpgContext *ctx);

signals:

    void signalKeyGenerated(bool success);

private:
    const GpgKey mKey;
    GenKeyInfo *keyGenParams;
    GpgME::GpgContext *mCtx;
    [[maybe_unused]] bool abort;
    QMutex mutex;

protected:

    void run() override;
};


#endif //GPGFRONTEND_SUBKEYGENERATETHREAD_H
