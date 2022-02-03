//
// Copyright 2015-2016 Jonathan Bennett <jon@autoitscript.com>
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
#include "TextEncodingDetect.h"

using namespace AutoIt::Common;

static const unsigned char TextEncodingDetect_UTF16_BOM_LE[] = {
    (unsigned char)(0xFF), (unsigned char)(0xFE)};
static const unsigned char TextEncodingDetect_UTF16_BOM_BE[] = {
    (unsigned char)(0xFE), (unsigned char)(0xFF)};
static const unsigned char TextEncodingDetect_UTF8_BOM[] = {
    (unsigned char)(0xEF), (unsigned char)(0xBB), (unsigned char)(0xBF)};

const unsigned char *TextEncodingDetect::utf16_bom_le_ =
    TextEncodingDetect_UTF16_BOM_LE;
const unsigned char *TextEncodingDetect::utf16_bom_be_ =
    TextEncodingDetect_UTF16_BOM_BE;
const unsigned char *TextEncodingDetect::utf8_bom_ =
    TextEncodingDetect_UTF8_BOM;

///////////////////////////////////////////////////////////////////////////////
// Constructor()
// Default constructor
///////////////////////////////////////////////////////////////////////////////

TextEncodingDetect::TextEncodingDetect() {
  // By default, assume nulls can't appear in ANSI/ASCII/UTF8 text files
  null_suggests_binary_ = true;

  // Set defaults for utf16 detection based the use of odd/even nulls
  utf16_expected_null_percent_ = 70;
  utf16_unexpected_null_percent_ = 10;
}

///////////////////////////////////////////////////////////////////////////////
// Set the percentages used in utf16 detection using nulls.
///////////////////////////////////////////////////////////////////////////////

void TextEncodingDetect::SetUtf16UnexpectedNullPercent(int percent) {
  if (percent > 0 && percent < 100) utf16_expected_null_percent_ = percent;
}

void TextEncodingDetect::SetUtf16ExpectedNullPercent(int percent) {
  if (percent > 0 && percent < 100) utf16_unexpected_null_percent_ = percent;
}

///////////////////////////////////////////////////////////////////////////////
// Simple function to return the length of the BOM for a particular encoding
// mode.
///////////////////////////////////////////////////////////////////////////////

int TextEncodingDetect::GetBOMLengthFromEncodingMode(Encoding encoding) {
  int length = 0;

  if (encoding == UTF16_BE_BOM || encoding == UTF16_LE_BOM)
    length = 2;
  else if (encoding == UTF8_BOM)
    length = 3;

  return length;
}

///////////////////////////////////////////////////////////////////////////////
// Checks if a buffer contains a valid BOM and returns the encoding based on it.
// Returns encoding "None" if there is no BOM.
///////////////////////////////////////////////////////////////////////////////

TextEncodingDetect::Encoding TextEncodingDetect::CheckBOM(
    const unsigned char *pBuffer, size_t size) {
  // Check for BOM
  if (size >= 2 && pBuffer[0] == utf16_bom_le_[0] &&
      pBuffer[1] == utf16_bom_le_[1]) {
    return UTF16_LE_BOM;
  } else if (size >= 2 && pBuffer[0] == utf16_bom_be_[0] &&
             pBuffer[1] == utf16_bom_be_[1]) {
    return UTF16_BE_BOM;
  } else if (size >= 3 && pBuffer[0] == utf8_bom_[0] &&
             pBuffer[1] == utf8_bom_[1] && pBuffer[2] == utf8_bom_[2]) {
    return UTF8_BOM;
  } else {
    return None;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Checks if a buffer contains a valid BOM and returns the encoding based on it.
// If it doesn't contain a BOM it tries to guess what the encoding is or
// "None" if it just looks like binary data.
///////////////////////////////////////////////////////////////////////////////

TextEncodingDetect::Encoding TextEncodingDetect::DetectEncoding(
    const unsigned char *pBuffer, size_t size) const {
  // First check if we have a BOM and return that if so
  Encoding encoding = CheckBOM(pBuffer, size);
  if (encoding != None) return encoding;

  // Now check for valid UTF8
  encoding = CheckUTF8(pBuffer, size);
  if (encoding != None) return encoding;

  // Now try UTF16
  encoding = CheckUTF16NewlineChars(pBuffer, size);
  if (encoding != None) return encoding;

  encoding = CheckUTF16ASCII(pBuffer, size);
  if (encoding != None) return encoding;

  // ANSI or None (binary) then
  if (!DoesContainNulls(pBuffer, size))
    return ANSI;
  else {
    // Found a null, return based on the preference in null_suggests_binary_
    if (null_suggests_binary_)
      return None;
    else
      return ANSI;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Checks if a buffer contains valid utf8. Returns:
// None - not valid utf8
// UTF8_NOBOM - valid utf8 encodings and multibyte sequences
// ASCII - Only data in the 0-127 range.
///////////////////////////////////////////////////////////////////////////////

TextEncodingDetect::Encoding TextEncodingDetect::CheckUTF8(
    const unsigned char *pBuffer, size_t size) const {
  // UTF8 Valid sequences
  // 0xxxxxxx  ASCII
  // 110xxxxx 10xxxxxx  2-byte
  // 1110xxxx 10xxxxxx 10xxxxxx  3-byte
  // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  4-byte
  //
  // Width in UTF8
  // Decimal		Width
  // 0-127		1 byte
  // 194-223		2 bytes
  // 224-239		3 bytes
  // 240-244		4 bytes
  //
  // Subsequent chars are in the range 128-191

  bool only_saw_ascii_range = true;
  size_t pos = 0;
  int more_chars;

  while (pos < size) {
    unsigned char ch = pBuffer[pos++];

    if (ch == 0 && null_suggests_binary_) {
      return None;
    } else if (ch <= 127) {
      // 1 byte
      more_chars = 0;
    } else if (ch >= 194 && ch <= 223) {
      // 2 Byte
      more_chars = 1;
    } else if (ch >= 224 && ch <= 239) {
      // 3 Byte
      more_chars = 2;
    } else if (ch >= 240 && ch <= 244) {
      // 4 Byte
      more_chars = 3;
    } else {
      return None;  // Not utf8
    }

    // Check secondary chars are in range if we are expecting any
    while (more_chars && pos < size) {
      only_saw_ascii_range = false;  // Seen non-ascii chars now

      ch = pBuffer[pos++];
      if (ch < 128 || ch > 191) return None;  // Not utf8

      --more_chars;
    }
  }

  // If we get to here then only valid UTF-8 sequences have been processed

  // If we only saw chars in the range 0-127 then we can't assume UTF8 (the
  // caller will need to decide)
  if (only_saw_ascii_range)
    return ASCII;
  else
    return UTF8_NOBOM;
}

///////////////////////////////////////////////////////////////////////////////
// Checks if a buffer contains text that looks like utf16 by scanning for
// newline chars that would be present even in non-english text.
// Returns:
// None - not valid utf16
// UTF16_LE_NOBOM - looks like utf16 le
// UTF16_BE_NOBOM - looks like utf16 be
///////////////////////////////////////////////////////////////////////////////

TextEncodingDetect::Encoding TextEncodingDetect::CheckUTF16NewlineChars(
    const unsigned char *pBuffer, size_t size) {
  if (size < 2) return None;

  // Reduce size by 1 so we don't need to worry about bounds checking for pairs
  // of bytes
  size--;

  int le_control_chars = 0;
  int be_control_chars = 0;
  unsigned char ch1, ch2;

  size_t pos = 0;
  while (pos < size) {
    ch1 = pBuffer[pos++];
    ch2 = pBuffer[pos++];

    if (ch1 == 0) {
      if (ch2 == 0x0a || ch2 == 0x0d) ++be_control_chars;
    } else if (ch2 == 0) {
      if (ch1 == 0x0a || ch1 == 0x0d) ++le_control_chars;
    }

    // If we are getting both LE and BE control chars then this file is not
    // utf16
    if (le_control_chars && be_control_chars) return None;
  }

  if (le_control_chars)
    return UTF16_LE_NOBOM;
  else if (be_control_chars)
    return UTF16_BE_NOBOM;
  else
    return None;
}

///////////////////////////////////////////////////////////////////////////////
// Checks if a buffer contains text that looks like utf16. This is done based
// the use of nulls which in ASCII/script like text can be useful to identify.
// Returns:
// None - not valid utf16
// UTF16_LE_NOBOM - looks like utf16 le
// UTF16_BE_NOBOM - looks like utf16 be
///////////////////////////////////////////////////////////////////////////////

TextEncodingDetect::Encoding TextEncodingDetect::CheckUTF16ASCII(
    const unsigned char *pBuffer, size_t size) const {
  int num_odd_nulls = 0;
  int num_even_nulls = 0;

  // Get even nulls
  size_t pos = 0;
  while (pos < size) {
    if (pBuffer[pos] == 0) num_even_nulls++;

    pos += 2;
  }

  // Get odd nulls
  pos = 1;
  while (pos < size) {
    if (pBuffer[pos] == 0) num_odd_nulls++;

    pos += 2;
  }

  double even_null_threshold = (num_even_nulls * 2.0) / size;
  double odd_null_threshold = (num_odd_nulls * 2.0) / size;
  double expected_null_threshold = utf16_expected_null_percent_ / 100.0;
  double unexpected_null_threshold = utf16_unexpected_null_percent_ / 100.0;

  // Lots of odd nulls, low number of even nulls
  if (even_null_threshold < unexpected_null_threshold &&
      odd_null_threshold > expected_null_threshold)
    return UTF16_LE_NOBOM;

  // Lots of even nulls, low number of odd nulls
  if (odd_null_threshold < unexpected_null_threshold &&
      even_null_threshold > expected_null_threshold)
    return UTF16_BE_NOBOM;

  // Don't know
  return None;
}

///////////////////////////////////////////////////////////////////////////////
// Checks if a buffer contains any nulls. Used to check for binary vs text data.
///////////////////////////////////////////////////////////////////////////////

bool TextEncodingDetect::DoesContainNulls(const unsigned char *pBuffer,
                                          size_t size) {
  size_t pos = 0;
  while (pos < size) {
    if (pBuffer[pos++] == 0) return true;
  }

  return false;
}
