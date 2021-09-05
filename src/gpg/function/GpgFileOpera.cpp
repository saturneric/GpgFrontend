/**
 * This file is part of GPGFrontend.
 *
 * GPGFrontend is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Foobar.  If not, see <https://www.gnu.org/licenses/>.
 *
 * The initial version of the source code is inherited from gpg4usb-team.
 * Their source code version also complies with GNU General Public License.
 *
 * The source code version of this software was modified and released
 * by Saturneric<eric@bktus.com> starting on May 12, 2021.
 *
 */
#include "gpg/function/GpgFileOpera.h"
#include "gpg/function/BasicOperator.h"

#include <filesystem>
#include <memory>

GpgFrontend::GpgError GpgFrontend::GpgFileOpera::EncryptFile(
    KeyArgsList &keys, const std::string &path, GpgEncrResult &result) {

  using namespace std::filesystem;
  class path file_info(path.c_str());

  if (!exists(file_info) || !is_regular_file(path))
    throw std::runtime_error("no permission");

  QFile in_file(path.c_str());
  if (!in_file.open(QIODevice::ReadOnly))
    throw std::runtime_error("cannot open file");

  QByteArray in_buffer = in_file.readAll();
  auto out_buffer = std::make_unique<QByteArray>();
  in_file.close();

  auto err =
      BasicOperator::GetInstance().Encrypt(keys, in_buffer, out_buffer, result);

  if (gpg_err_code(err) != GPG_ERR_NO_ERROR)
    return err;

  QFile out_file((path + ".asc").c_str());

  if (!out_file.open(QFile::WriteOnly))
    throw std::runtime_error("cannot open file");

  QDataStream out(&out_file);
  out.writeRawData(out_buffer->data(), out_buffer->length());
  out_file.close();
  return err;
}

GpgFrontend::GpgError
GpgFrontend::GpgFileOpera::DecryptFile(const std::string &path,
                                       GpgDecrResult &result) {

  QFileInfo file_info(path);

  if (!file_info.isFile() || !file_info.isReadable())
    throw std::runtime_error("no permission");

  QFile in_file(path);
  if (!in_file.open(QIODevice::ReadOnly))
    throw std::runtime_error("cannot open file");

  QByteArray in_buffer = in_file.readAll();
  auto out_buffer = std::make_unique<QByteArray>();
  in_file.close();

  auto err =
      BasicOperator::GetInstance().Decrypt(in_buffer, out_buffer, result);

  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  QString out_file_name, file_extension = file_info.suffix();

  if (file_extension == "asc" || file_extension == "gpg") {
    int pos = path.lastIndexOf('.');
    out_file_name = path.left(pos);
  } else
    out_file_name = path + ".out";

  QFile out_file(out_file_name);

  if (!out_file.open(QFile::WriteOnly))
    throw std::runtime_error("cannot open file");

  QDataStream out(&out_file);
  out.writeRawData(out_buffer->data(), out_buffer->length());
  out_file.close();

  return err;
}

gpgme_error_t GpgFrontend::GpgFileOpera::SignFile(KeyArgsList &keys,
                                                  const std::string &path,
                                                  GpgSignResult &result) {

  QFileInfo file_info(path.c_str());

  if (!file_info.isFile() || !file_info.isReadable())
    throw std::runtime_error("no permission");

  QFile in_file(path.c_str());
  if (!in_file.open(QIODevice::ReadOnly))
    throw std::runtime_error("cannot open file");

  QByteArray in_buffer = in_file.readAll();
  auto out_buffer = std::make_unique<QByteArray>();
  in_file.close();

  auto err = BasicOperator::GetInstance().Sign(keys, in_buffer, out_buffer,
                                               GPGME_SIG_MODE_DETACH, result);
  assert(gpgme_err_code(err) == GPG_ERR_NO_ERROR);

  QFile out_file((path + ".sig").c_str());

  if (!out_file.open(QFile::WriteOnly))
    throw std::runtime_error("cannot open file");

  QDataStream out(&out_file);
  out.writeRawData(out_buffer->data(), out_buffer->length());
  out_file.close();

  return err;
}

gpgme_error_t GpgFrontend::GpgFileOpera::VerifyFile(const std::string &path,
                                                    GpgVerifyResult &result) {

  qDebug() << "Verify File Path" << path.c_str();

  QFileInfo file_info(path.c_str());

  if (!file_info.isFile() || !file_info.isReadable())
    throw std::runtime_error("no permission");

  QFile in_file(path.c_str());
  if (!in_file.open(QIODevice::ReadOnly))
    throw std::runtime_error("cannot open file");

  QByteArray in_buffer = in_file.readAll();
  std::unique_ptr<QByteArray> sign_buffer = nullptr;

  if (file_info.suffix() == "gpg") {
    auto err =
        BasicOperator::GetInstance().Verify(in_buffer, sign_buffer, result);
    return err;
  } else {
    QFile sign_file;
    sign_file.setFileName((path + ".sig").c_str());
    if (!sign_file.open(QIODevice::ReadOnly)) {
      throw std::runtime_error("cannot open file");
    }

    auto sign_buffer = std::make_unique<QByteArray>(sign_file.readAll());
    in_file.close();

    auto err =
        BasicOperator::GetInstance().Verify(in_buffer, sign_buffer, result);
    return err;
  }
}

// TODO

gpg_error_t GpgFrontend::GpgFileOpera::EncryptSignFile(
    KeyArgsList &keys, const std::string &path, GpgEncrResult &encr_res,
    GpgSignResult &sign_res) {

  qDebug() << "Encrypt Sign File Path" << path.c_str();

  QFileInfo file_info(path.c_str());

  if (!file_info.isFile() || !file_info.isReadable())
    throw std::runtime_error("no permission");

  QFile in_file;
  in_file.setFileName(path.c_str());
  if (!in_file.open(QIODevice::ReadOnly))
    throw std::runtime_error("cannot open file");

  QByteArray in_buffer = in_file.readAll();
  in_file.close();
  std::unique_ptr<QByteArray> out_buffer = nullptr;

  // TODO Fill the vector
  std::vector<GpgKey> signerKeys;

  // TODO dealing with signer keys
  auto err = BasicOperator::GetInstance().EncryptSign(
      keys, signerKeys, in_buffer, out_buffer, encr_res, sign_res);

  if (gpg_err_code(err) != GPG_ERR_NO_ERROR)
    return err;

  QFile out_file((path + ".gpg").c_str());

  if (!out_file.open(QFile::WriteOnly))
    throw std::runtime_error("cannot open file");

  QDataStream out(&out_file);
  out.writeRawData(out_buffer->data(), out_buffer->length());
  out_file.close();

  return err;
}

gpg_error_t
GpgFrontend::GpgFileOpera::DecryptVerifyFile(const std::string &path,
                                             GpgDecrResult &decr_res,
                                             GpgVerifyResult &verify_res) {

  qDebug() << "Decrypt Verify File Path" << path.c_str();

  QFileInfo file_info(path.c_str());

  if (!file_info.isFile() || !file_info.isReadable())
    throw std::runtime_error("no permission");

  QFile in_file;
  in_file.setFileName(path.c_str());
  if (!in_file.open(QIODevice::ReadOnly))
    throw std::runtime_error("cannot open file");

  QByteArray in_buffer = in_file.readAll();
  in_file.close();
  std::unique_ptr<QByteArray> out_buffer = nullptr;

  auto err = BasicOperator::GetInstance().DecryptVerify(in_buffer, out_buffer,
                                                        decr_res, verify_res);
  if (gpg_err_code(err) != GPG_ERR_NO_ERROR)
    return err;

  std::string out_file_name,
      file_extension = file_info.suffix().toUtf8().constData();

  if (file_extension == "asc" || file_extension == "gpg") {
    int pos = path.find_last_of('.');
    out_file_name = path.substr(0, pos);
  } else
    out_file_name = (path + ".out").c_str();

  QFile out_file(out_file_name.c_str());

  if (!out_file.open(QFile::WriteOnly))
    throw std::runtime_error("cannot open file");

  QDataStream out(&out_file);
  out.writeRawData(out_buffer->data(), out_buffer->length());
  out_file.close();

  return err;
}
