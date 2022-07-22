/**
 * AES encryption/decryption demo program using OpenSSL EVP apis
 * gcc -Wall openssl_aes.c -lcrypto
 * this is public domain code.
 * Saju Pillai (saju.pillai@gmail.com)
 **/

#include "aes_ssl.h"

namespace GpgFrontend::RawAPI {

/**
 * @brief Create a 256 bit key and IV using the supplied key_data. salt can be
 * added for taste. Fills in the encryption and decryption ctx objects and
 * returns 0 on success
 *
 * @param key_data
 * @param key_data_len
 * @param salt
 * @param e_ctx
 * @param d_ctx
 * @return int
 */
int aes_256_cbc_init(uint8_t *key_data, int key_data_len, uint8_t *salt,
                     EVP_CIPHER_CTX *e_ctx, EVP_CIPHER_CTX *d_ctx) {
  int i, nrounds = 5;
  uint8_t key[32], iv[32];

  /*
   * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the
   * supplied key material. nrounds is the number of times the we hash the
   * material. More rounds are more secure but slower.
   */
  i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data,
                     key_data_len, nrounds, key, iv);
  if (i != 32) {
    printf("Key size is %d bits - should be 256 bits\n", i);
    return -1;
  }

  EVP_CIPHER_CTX_init(e_ctx);
  EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
  EVP_CIPHER_CTX_init(d_ctx);
  EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);

  return 0;
}

/**
 * @brief  Encrypt *len bytes of data All data going in & out is considered
 * binary (uint8_t[])
 *
 * @param e
 * @param plaintext
 * @param len
 * @return uint8_t*
 */
uint8_t *aes_256_cbc_encrypt(EVP_CIPHER_CTX *e, uint8_t *plaintext, int *len) {
  /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1
   * bytes */
  int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
  auto *ciphertext = (uint8_t *)malloc(c_len);

  /* allows reusing of 'e' for multiple encryption cycles */
  EVP_EncryptInit_ex(e, nullptr, nullptr, nullptr, nullptr);

  /* update ciphertext, c_len is filled with the length of ciphertext generated,
   *len is the size of plaintext in bytes */
  EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len);

  /* update ciphertext with the final remaining bytes */
  EVP_EncryptFinal_ex(e, ciphertext + c_len, &f_len);

  *len = c_len + f_len;
  return ciphertext;
}

/**
 * @brief Decrypt *len bytes of ciphertext
 *
 * @param e
 * @param ciphertext
 * @param len
 * @return uint8_t*
 */
uint8_t *aes_256_cbc_decrypt(EVP_CIPHER_CTX *e, uint8_t *ciphertext, int *len) {
  /* plaintext will always be equal to or lesser than length of ciphertext*/
  int p_len = *len, f_len = 0;
  auto *plaintext = (uint8_t *)malloc(p_len);

  EVP_DecryptInit_ex(e, nullptr, nullptr, nullptr, nullptr);
  EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len);
  EVP_DecryptFinal_ex(e, plaintext + p_len, &f_len);

  *len = p_len + f_len;
  return plaintext;
}

}  // namespace GpgFrontend::RawAPI