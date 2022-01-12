#pragma once
#ifndef TEXT_ENCODING_DETECT_H_
#define TEXT_ENCODING_DETECT_H_

//
// Copyright 2015 Jonathan Bennett <jon@autoitscript.com>
//
// https://www.autoitscript.com
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

// Includes
#include <stddef.h>

namespace AutoIt::Common {
class TextEncodingDetect {
 public:
  enum Encoding {
    None,            // Unknown or binary
    ANSI,            // 0-255
    ASCII,           // 0-127
    UTF8_BOM,        // UTF8 with BOM
    UTF8_NOBOM,      // UTF8 without BOM
    UTF16_LE_BOM,    // UTF16 LE with BOM
    UTF16_LE_NOBOM,  // UTF16 LE without BOM
    UTF16_BE_BOM,    // UTF16-BE with BOM
    UTF16_BE_NOBOM,  // UTF16-BE without BOM
  };

  TextEncodingDetect();
  ~TextEncodingDetect() = default;

  static Encoding CheckBOM(
      const unsigned char *pBuffer,
      size_t size);  // Just check if there is a BOM and return
  Encoding DetectEncoding(const unsigned char *pBuffer, size_t size)
      const;  // Check BOM and also guess if there is no BOM
  static int GetBOMLengthFromEncodingMode(
      Encoding encoding);  // Just return the BOM length of a given mode

  void SetNullSuggestsBinary(bool null_suggests_binary) {
    null_suggests_binary_ = null_suggests_binary;
  }
  void SetUtf16UnexpectedNullPercent(int percent);
  void SetUtf16ExpectedNullPercent(int percent);

 private:
  TextEncodingDetect(const TextEncodingDetect &);
  const TextEncodingDetect &operator=(const TextEncodingDetect &);

  static const unsigned char *utf16_bom_le_;
  static const unsigned char *utf16_bom_be_;
  static const unsigned char *utf8_bom_;

  bool null_suggests_binary_;
  int utf16_expected_null_percent_;
  int utf16_unexpected_null_percent_;

  Encoding CheckUTF8(const unsigned char *pBuffer,
                     size_t size) const;  // Check for valid UTF8 with no BOM
  static Encoding CheckUTF16NewlineChars(
      const unsigned char *pBuffer,
      size_t size);  // Check for valid UTF16 with no BOM via control chars
  Encoding CheckUTF16ASCII(const unsigned char *pBuffer, size_t size)
      const;  // Check for valid UTF16 with no BOM via null distribution
  static bool DoesContainNulls(const unsigned char *pBuffer,
                               size_t size);  // Check for nulls
};

}  // namespace AutoIt::Common

//////////////////////////////////////////////////////////////////////

#endif
