//
// Created by eric on 2021/5/30.
//

#include "ui/keygen/SubkeyGenerateThread.h"

#include <utility>

SubkeyGenerateThread::SubkeyGenerateThread(GpgKey key, GenKeyInfo *keyGenParams, GpgME::GpgContext *ctx)
        : mKey(std::move(key)), keyGenParams(keyGenParams) , mCtx(ctx), abort(
        false) {
    connect(this, &SubkeyGenerateThread::finished, this, &SubkeyGenerateThread::deleteLater);
}

void SubkeyGenerateThread::run() {
    bool success = mCtx->generateSubkey(mKey, keyGenParams);
    emit signalKeyGenerated(success);
    emit finished({});
}
