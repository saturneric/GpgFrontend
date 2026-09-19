#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>

#include "core/function/ArchiveFileOperator.h"
#include "core/module/ModulePackageVerifier.h"

namespace GpgFrontend::Test {

TEST(ZZTiming, WhereTheVerifyTimeGoes) {
  const QDir packages(QCoreApplication::applicationDirPath() +
                      "/module-packages");
  const auto built = packages.entryInfoList(QStringList{"*.gfmodule"},
                                            QDir::Files, QDir::Size);
  if (built.isEmpty()) GTEST_SKIP();
  const auto path = built.first().absoluteFilePath();
  const auto size = built.first().size();
  printf("package %s (%.1f MiB)\n", qPrintable(built.first().fileName()),
         size / 1048576.0);

  QElapsedTimer t;

  t.start();
  {
    QFile f(path);
    f.open(QIODevice::ReadOnly);
    QCryptographicHash h(QCryptographicHash::Sha256);
    h.addData(&f);
    (void)h.result();
  }
  printf("  plain read+sha256          : %6lld ms\n", (long long)t.elapsed());

  t.restart();
  {
    QString reason;
    ArchiveFileOperator::ExtractArchiveFromFileSync(
        path, QDir::tempPath() + "/zz-nowhere-does-not-exist",
        ArchiveExtractPolicy::Permissive(), [](const QString&) { return true; },
        {}, [](const QString&, const QByteArray&) { return true; }, &reason);
  }
  printf("  archive walk, raw sink     : %6lld ms\n", (long long)t.elapsed());

  t.restart();
  const auto v = Module::VerifyModulePackage(path);
  printf("  VerifyModulePackage        : %6lld ms  (ok=%d)\n",
         (long long)t.elapsed(), (int)v.ok);

  t.restart();
  const auto r = Module::ReadVerifiedModuleImage(path);
  printf("  ReadVerifiedModuleImage    : %6lld ms  (ok=%d)\n",
         (long long)t.elapsed(), (int)r.ok);
}

}  // namespace GpgFrontend::Test
