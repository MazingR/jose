/* vim: set tabstop=8 shiftwidth=4 softtabstop=4 expandtab smarttab colorcolumn=80: */
/*
 * Copyright 2016 Red Hat, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <jose/buf.h>
#include <jansson.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct jose_jwe_crypter {
    struct jose_jwe_crypter *next;
    const char **encs;

    const char *
    (*suggest)(const json_t *jwk);

    bool
    (*encrypt)(json_t *jwe, const json_t *cek, const uint8_t pt[], size_t ptl,
               const char *enc, const char *prot, const char *aad);

    jose_buf_t *
    (*decrypt)(const json_t *jwe, const json_t *cek, const char *enc,
               const char *prot, const char *aad);
} jose_jwe_crypter_t;

typedef struct jose_jwe_wrapper {
    struct jose_jwe_wrapper *next;
    const char **algs;

    const char *
    (*suggest)(const json_t *jwk);

    bool
    (*wrap)(json_t *jwe, json_t *cek, const json_t *jwk, json_t *rcp,
            const char *alg);
    bool
    (*unwrap)(const json_t *jwe, const json_t *jwk, const json_t *rcp,
              const char *alg, json_t *cek);
} jose_jwe_wrapper_t;

typedef struct jose_jwe_zipper {
    struct jose_jwe_zipper *next;
    const char *zip;

    jose_buf_t *
    (*deflate)(const uint8_t val[], size_t len);

    jose_buf_t *
    (*inflate)(const uint8_t val[], size_t len);
} jose_jwe_zipper_t;

void
jose_jwe_register_crypter(jose_jwe_crypter_t *crypter);

void
jose_jwe_register_wrapper(jose_jwe_wrapper_t *wrapper);

void
jose_jwe_register_zipper(jose_jwe_zipper_t *zipper);

/**
 * Converts a JWE from compact format into JSON format.
 */
json_t *
jose_jwe_from_compact(const char *jwe);

/**
 * Converts a JWS from JSON format into compact format.
 *
 * If more than one recipient exists or if an unprotected header is found,
 * this operation will fail.
 *
 * Free with free().
 */
char *
jose_jwe_to_compact(const json_t *jwe);

/**
 * Encrypts the specified plaintext bytes into the JWE using the specified CEK.
 *
 * Please note that this DOES NOT wrap the CEK. You need to also call
 * jose_jwe_wrap() in order to perform that operation.
 */
bool
jose_jwe_encrypt(json_t *jwe, const json_t *cek,
                 const uint8_t pt[], size_t ptl);

/**
 * Encrypts the specified plaintext JSON into the JWE using the specified CEK.
 *
 * Please note that this DOES NOT wrap the CEK. You need to also call
 * jose_jwe_wrap() in order to perform that operation.
 */
bool
jose_jwe_encrypt_json(json_t *jwe, const json_t *cek, json_t *pt);

/**
 * Wraps a CEK using the specified JWK.
 *
 * This function has sophisticated behavior, so please read this carefully.
 *
 * This function only performs wrapping. To encrypt data using the CEk, you
 * need to call jose_jwe_encrypt().
 *
 * The input CEK may be a JWK template (as used by jose_jwk_generate()). In
 * this case, a CEK will be generated for you on the first call to
 * jose_jwe_wrap(). This is particularly useful for some encryption modes where
 * the CEK must be generated, such as ECDH-ES.
 *
 * This function may be called multiple times to wrap the same CEK with
 * multiple JWKs. In such cases, if you wish to mix with wrapping algorithms
 * such as ECDH-ES (which calculate the CEK directly), you must specify them
 * first.
 *
 * The rcp parameter is not required. You may pass NULL to omit it. If used,
 * the value must be a template recipient object.
 */
bool
jose_jwe_wrap(json_t *jwe, json_t *cek, const json_t *jwk, json_t *rcp);

/**
 * Unwraps the CEK using the specified JWK.
 *
 * Where the PBES2 family of algorithms are used, the input jwk parameter
 * should actually be a JSON string containing the password.
 *
 * Where the dir algorithm is used, the output CEK will simply be a deep copy
 * of the input jwk parameter. This enables your code to avoid special-casing
 * the dir algorithm.
 *
 * The rcp parameter is not required. You may pass NULL to omit it. If
 * specified, it contains the recipient object you wish to unwrap. Otherwise,
 * this function will return the CEK from the first recipient object to succeed
 * the unwrapping process.
 */
json_t *
jose_jwe_unwrap(const json_t *jwe, const json_t *jwk, const json_t *rcp);

/**
 * Decrypts the ciphertext bytes in the JWE using the specified CEK.
 *
 * Implicitly, this validates the protected header.
 */
jose_buf_t *
jose_jwe_decrypt(const json_t *jwe, const json_t *cek);

/**
 * Decrypts the ciphertext JSON in the JWE using the specified CEK.
 *
 * Implicitly, this validates the protected header.
 */
json_t *
jose_jwe_decrypt_json(const json_t *jwe, const json_t *cek);

/**
 * Merges the protected, shared and unprotected headers into the JOSE header.
 *
 * WARNING: This function does not verify the protected header. You MUST call
 * jose_jwe_decrypt() or jose_jwe_decrypt_json() to ensure that the protected
 * header has not been modified.
 */
json_t *
jose_jwe_merge_header(const json_t *jwe, const json_t *rcp);
