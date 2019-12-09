/*****************************************************************************
 Copyright (c) 2009-2019, Intel Corporation

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
 FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/


/*-----------------------------------------------------------------------
* Zuc functional test
*-----------------------------------------------------------------------
*
* A simple functional test for ZUC
*
*-----------------------------------------------------------------------*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <intel-ipsec-mb.h>

#include "zuc_test_vectors.h"
#include "gcm_ctr_vectors_test.h"

#define MAXBUFS 9
#define PASS_STATUS 0
#define FAIL_STATUS -1

int zuc_test(const enum arch_type arch, struct MB_MGR *mb_mgr);

int validate_zuc_algorithm(struct MB_MGR *mb_mgr, uint8_t *pSrcData,
                           uint8_t *pDstData, uint8_t *pKeys, uint8_t *pIV);
int validate_zuc_EEA_1_block(struct MB_MGR *mb_mgr, uint8_t *pSrcData,
                             uint8_t *pDstData, uint8_t *pKeys, uint8_t *pIV,
                             unsigned int job_api);
int validate_zuc_EEA_4_block(struct MB_MGR *mb_mgr, uint8_t **pSrcData,
                             uint8_t **pDstData, uint8_t **pKeys,
                             uint8_t **pIV, unsigned int job_api);
int validate_zuc_EEA_n_block(struct MB_MGR *mb_mgr, uint8_t **pSrcData,
                             uint8_t **pDstData, uint8_t **pKeys, uint8_t **pIV,
                             uint32_t numBuffs, unsigned int job_api);
int validate_zuc_EIA_1_block(struct MB_MGR *mb_mgr, uint8_t *pSrcData,
                             uint8_t *pDstData, uint8_t *pKeys, uint8_t *pIV,
                             unsigned int job_api);
static void byte_hexdump(const char *message, const uint8_t *ptr, int len);

/******************************************************************************
 * @ingroup zuc_functionalTest_app
 *
 * @description
 * This function allocates memory for buffers and set random data in each buffer
 *
 * pSrcData = pointers to the new source buffers
 * numOfBuffs = number of buffers
 * ************************************************/
static uint32_t createData(uint8_t *pSrcData[MAXBUFS],
                                 uint32_t numOfBuffs)
{
        uint32_t i = 0, j = 0;

        for (i = 0; i < numOfBuffs; i++) {
                pSrcData[i] = (uint8_t *)malloc(MAX_BUFFER_LENGTH_IN_BYTES);

                if (!pSrcData[i]) {
                        printf("malloc(pSrcData[i]): failed!\n");

                        for (j = 0; j < i; j++) {
                                free(pSrcData[j]);
                                pSrcData[j] = NULL;
                        }

                        return FAIL_STATUS;
                }
        }
        return PASS_STATUS;
}

/******************************************************************************
 * @ingroup zuc_functionalTest_app
 *
 * @description
 * This function creates source data and vector buffers.
 *
 * keyLen = key length
 * pKeys = array of pointers to the new key buffers
 * ivLen = vector length
 * pIV = array of pointers to the new vector buffers
 * numOfBuffs = number of buffers
************************************************/
static uint32_t createKeyVecData(uint32_t keyLen, uint8_t *pKeys[MAXBUFS],
                                 uint32_t ivLen, uint8_t *pIV[MAXBUFS],
                                 uint32_t numOfBuffs)
{
        uint32_t i = 0, j = 0;

        for (i = 0; i < numOfBuffs; i++) {
                pIV[i] = (uint8_t *)malloc(ivLen);

                if (!pIV[i]) {
                        printf("malloc(pIV[i]): failed!\n");

                        for (j = 0; j < i; j++) {
                                free(pIV[j]);
                                free(pKeys[j]);
                        }

                        return FAIL_STATUS;
                }

                pKeys[i] = malloc(keyLen);

                if (!pKeys[i]) {
                        printf("malloc(pKeys[i]): failed!\n");

                        for (j = 0; j <= i; j++) {
                                free(pIV[j]);

                                if (j < i)
                                        free(pKeys[j]);
                        }
                        return FAIL_STATUS;
                }
        }

        return PASS_STATUS;
}

/******************************************************************************
 * @ingroup zuc_benchmark_app
 *
 * @description
 * This function free memory pointed to by an array of pointers
 *
 * arr = array of memory pointers
 * length = length of pointer array (or number of pointers whose buffers
 * should be freed)
 * ************************************************/
static void freePtrArray(uint8_t *pArr[MAXBUFS], uint32_t arrayLength)
{
        uint32_t i = 0;

        for (i = 0; i < arrayLength; i++)
                free(pArr[i]);
}

static uint32_t bswap4(const uint32_t val)
{
        return ((val >> 24) |             /**< A*/
                ((val & 0xff0000) >> 8) | /**< B*/
                ((val & 0xff00) << 8) |   /**< C*/
                (val << 24));             /**< D*/
}

int zuc_test(const enum arch_type arch, struct MB_MGR *mb_mgr)
{

        uint32_t numBuffs, a;
        uint32_t status = PASS_STATUS;
        uint8_t *pKeys[MAXBUFS];
        uint8_t *pIV[MAXBUFS];
        uint8_t *pSrcData[MAXBUFS];
        uint8_t *pDstData[MAXBUFS];

        /* Do not run the tests for aesni emulation */
        if (arch == ARCH_NO_AESNI)
                return 0;

        printf("Running Functional Tests\n");
        fflush(stdout);

        /*Create test data buffers + populate with random data*/
        if (createData(pSrcData, MAXBUFS)) {
                printf("createData() error\n");
                return FAIL_STATUS;
        }
        if (createData(pDstData, MAXBUFS)) {
                printf("createData() error\n");
                return FAIL_STATUS;
        }

        /*Create random keys and vectors*/
        if (createKeyVecData(ZUC_KEY_LEN_IN_BYTES, pKeys, ZUC_IV_LEN_IN_BYTES,
                             pIV, MAXBUFS)) {
                printf("createKeyVecData() error\n");
                freePtrArray(pSrcData, MAXBUFS);
                freePtrArray(pDstData, MAXBUFS);
                return FAIL_STATUS;
        }

        if (validate_zuc_algorithm(mb_mgr, pSrcData[0], pSrcData[0], pKeys[0],
                                   pIV[0]))
                status = 1;
        else
                printf("validate ZUC algorithm: PASS\n");

        /* Direct API tests */
        if (validate_zuc_EEA_1_block(mb_mgr, pSrcData[0], pSrcData[0], pKeys[0],
                                     pIV[0], 0))
                status = 1;
        else
                printf("validate ZUC 1 block (direct API): PASS\n");

        if (validate_zuc_EEA_4_block(mb_mgr, pSrcData, pSrcData, pKeys, pIV, 0))
                status = 1;
        else
                printf("validate ZUC 4 block (direct API): PASS\n");

        for (a = 0; a < 3; a++) {
                switch (a) {
                case 0:
                        numBuffs = 4;
                        break;
                case 1:
                        numBuffs = 8;
                        break;
                default:
                        numBuffs = 9;
                        break;
                }
                if (validate_zuc_EEA_n_block(mb_mgr, pSrcData, pDstData, pKeys,
                                             pIV, numBuffs, 0))
                        status = 1;
                else
                        printf("validate ZUC n block buffers %d (direct API): "
                               "PASS\n", a);
        }

        if (validate_zuc_EIA_1_block(mb_mgr, pSrcData[0], pDstData[0], pKeys[0],
                                     pIV[0], 0))
                status = 1;
        else
                printf("validate ZUC Integrity 1 block (direct API): PASS\n");

        /* Job API tests */
        if (validate_zuc_EEA_1_block(mb_mgr, pSrcData[0], pSrcData[0], pKeys[0],
                                     pIV[0], 1))
                status = 1;
        else
                printf("validate ZUC 1 block (job API): PASS\n");

        if (validate_zuc_EEA_4_block(mb_mgr, pSrcData, pSrcData, pKeys, pIV, 1))
                status = 1;
        else
                printf("validate ZUC 4 block (job API): PASS\n");

        for (a = 0; a < 3; a++) {
                switch (a) {
                case 0:
                        numBuffs = 4;
                        break;
                case 1:
                        numBuffs = 8;
                        break;
                default:
                        numBuffs = 9;
                        break;
                }
                if (validate_zuc_EEA_n_block(mb_mgr, pSrcData, pDstData, pKeys,
                                             pIV, numBuffs, 1))
                        status = 1;
                else
                        printf("validate ZUC n block buffers %d (job API): "
                               "PASS\n", a);
        }

        if (validate_zuc_EIA_1_block(mb_mgr, pSrcData[0], pDstData[0], pKeys[0],
                                     pIV[0], 1))
                status = 1;
        else
                printf("validate ZUC Integrity 1 block (job API): PASS\n");

        freePtrArray(pKeys, MAXBUFS);    /*Free the key buffers*/
        freePtrArray(pIV, MAXBUFS);      /*Free the vector buffers*/
        freePtrArray(pSrcData, MAXBUFS); /*Free the source buffers*/
        freePtrArray(pDstData, MAXBUFS); /*Free the destination buffers*/
        if (status)
                return status;

        printf("The Functional Test application completed\n");
        return 0;
}

static inline int
submit_eea3_jobs(struct MB_MGR *mb_mgr, uint8_t **keys, uint8_t **ivs,
                 uint8_t **src, uint8_t **dst, const uint32_t *lens,
                 int dir, const unsigned int num_jobs)
{
        JOB_AES_HMAC *job;
        unsigned int i;
        unsigned int jobs_rx = 0;

        for (i = 0; i < num_jobs; i++) {
                job = IMB_GET_NEXT_JOB(mb_mgr);
                job->cipher_direction = dir;
                job->chain_order = CIPHER_HASH;
                job->cipher_mode = ZUC_EEA3;
                job->src = src[i];
                job->dst = dst[i];
                job->iv = ivs[i];
                job->iv_len_in_bytes = 16;
                job->aes_enc_key_expanded = keys[i];
                job->aes_key_len_in_bytes = 16;

                job->cipher_start_src_offset_in_bytes = 0;
                job->msg_len_to_cipher_in_bytes = lens[i];
                job->hash_alg = NULL_HASH;

                job = IMB_SUBMIT_JOB(mb_mgr);
                if (job != NULL) {
                        jobs_rx++;
                        if (job->status != STS_COMPLETED) {
                                printf("%d error status:%d, job %d",
                                       __LINE__, job->status, i);
                                return -1;
                        }
                }
        }

        while ((job = IMB_FLUSH_JOB(mb_mgr)) != NULL) {
                jobs_rx++;
                if (job->status != STS_COMPLETED) {
                        printf("%d error status:%d, job %d",
                               __LINE__, job->status, i);
                        return -1;
                }
        }

        if (jobs_rx != num_jobs) {
                printf("Expected %d jobs, received %d\n", num_jobs, jobs_rx);
                return -1;
        }

        return 0;
}

static inline int
submit_eia3_job(struct MB_MGR *mb_mgr, uint8_t *key, uint8_t *iv,
                 uint8_t *src, uint8_t *tag, const uint32_t len)
{
        JOB_AES_HMAC *job;

        job = IMB_GET_NEXT_JOB(mb_mgr);
        job->chain_order = CIPHER_HASH;
        job->cipher_mode = NULL_CIPHER;
        job->src = src;
        job->u.ZUC_EIA3._iv = iv;
        job->u.ZUC_EIA3._key = key;

        job->hash_start_src_offset_in_bytes = 0;
        job->msg_len_to_hash_in_bits = len;
        job->hash_alg = ZUC_EIA3_BITLEN;
        job->auth_tag_output = tag;
        job->auth_tag_output_len_in_bytes = 4;

        job = IMB_SUBMIT_JOB(mb_mgr);
        if (job != NULL) {
                if (job->status != STS_COMPLETED) {
                        printf("%d error status:%d",
                               __LINE__, job->status);
                        return -1;
                }
        } else {
                printf("Expected returned job, but got nothing\n");
                return -1;
        }

        return 0;
}

static int
test_output(const uint8_t *out, const uint8_t *ref, const uint32_t bytelen,
            const uint32_t bitlen, const char *err_msg)
{
        int ret = 0;
        uint32_t byteResidue;
        uint32_t bitResidue;

        ret = memcmp(out, ref, bytelen - 1);
        if (ret) {
                printf("%s : FAIL\n", err_msg);
                byte_hexdump("Expected", ref, bytelen);
                byte_hexdump("Found", out, bytelen);
                ret = 1;
        } else {
                bitResidue = (0xFF00 >> (bitlen % 8)) & 0x00FF;
                byteResidue = (ref[bitlen / 8] ^ out[bitlen / 8]) & bitResidue;
                if (byteResidue) {
                        printf("%s : FAIL\n", err_msg);
                        printf("Expected: 0x%02X (last byte)\n",
                               0xFF & ref[bitlen / 8]);
                        printf("Found: 0x%02X (last byte)\n",
                               0xFF & out[bitlen / 8]);
                        ret = -1;
                }
#ifdef DEBUG
                else
                        printf("%s : PASS\n", err_msg);
#endif
        }
        fflush(stdout);

        return 0;
}

int validate_zuc_EEA_1_block(struct MB_MGR *mb_mgr, uint8_t *pSrcData,
                             uint8_t *pDstData, uint8_t *pKeys, uint8_t *pIV,
                             unsigned int job_api)
{
        uint32_t i;
        int ret = 0;

        for (i = 0; i < NUM_ZUC_EEA3_TESTS; i++) {
                char msg[50];
                int retTmp;
                uint32_t byteLength;

                memcpy(pKeys, testEEA3_vectors[i].CK, ZUC_KEY_LEN_IN_BYTES);
                zuc_eea3_iv_gen(testEEA3_vectors[i].count,
                                testEEA3_vectors[i].Bearer,
                                testEEA3_vectors[i].Direction,
                                pIV);
                byteLength = (testEEA3_vectors[i].length_in_bits + 7) / 8;
                memcpy(pSrcData, testEEA3_vectors[i].plaintext, byteLength);
                if (job_api)
                        submit_eea3_jobs(mb_mgr, &pKeys, &pIV, &pSrcData,
                                         &pDstData, &byteLength, ENCRYPT, 1);
                else
                        IMB_ZUC_EEA3_1_BUFFER(mb_mgr, pKeys, pIV, pSrcData,
                                              pDstData, byteLength);

                snprintf(msg, sizeof(msg),
                         "Validate ZUC 1 block test %u (Enc):", i + 1);
                retTmp = test_output(pDstData, testEEA3_vectors[i].ciphertext,
                                     byteLength,
                                     testEEA3_vectors[i].length_in_bits, msg);
                if (retTmp < 0)
                        ret = retTmp;
        }
        return ret;
};

int validate_zuc_EEA_4_block(struct MB_MGR *mb_mgr, uint8_t **pSrcData,
                             uint8_t **pDstData, uint8_t **pKeys, uint8_t **pIV,
                             unsigned int job_api)
{
        uint32_t i, j;
        int ret = 0;

        for (i = 0; i < NUM_ZUC_EEA3_TESTS; i++) {
                uint32_t packetLen[4];

                for (j = 0; j < 4; j++) {
                        packetLen[j] =
                            (testEEA3_vectors[i].length_in_bits + 7) / 8;
                        memcpy(pKeys[j], testEEA3_vectors[i].CK,
                               ZUC_KEY_LEN_IN_BYTES);
                        zuc_eea3_iv_gen(testEEA3_vectors[i].count,
                                        testEEA3_vectors[i].Bearer,
                                        testEEA3_vectors[i].Direction,
                                        pIV[j]);
                        memcpy(pSrcData[j], testEEA3_vectors[i].plaintext,
                               packetLen[j]);
                }
                if (job_api)
                        submit_eea3_jobs(mb_mgr, pKeys, pIV, pSrcData,
                                         pDstData, packetLen, ENCRYPT, 4);
                else
                        IMB_ZUC_EEA3_4_BUFFER(mb_mgr,
                                              (const void * const *)pKeys,
                                              (const void * const *)pIV,
                                              (const void * const *)pSrcData,
                                              (void **)pDstData, packetLen);
                for (j = 0; j < 4; j++) {
                        uint8_t *pDst8 = (uint8_t *)pDstData[j];
                        int retTmp;
                        char msg[50];

                        snprintf(msg, sizeof(msg),
                                "Validate ZUC 4 block test %u, index %u (Enc):",
                                i + 1, j);
                        retTmp = test_output(pDst8, testEEA3_vectors[i].ciphertext,
                                     packetLen[j],
                                     testEEA3_vectors[i].length_in_bits, msg);
                        if (retTmp < 0)
                                ret = retTmp;
                }

                for (j = 0; j < 4; j++)
                        memcpy(pSrcData[j], testEEA3_vectors[i].ciphertext,
                               (testEEA3_vectors[i].length_in_bits + 7) / 8);

                if (job_api)
                        submit_eea3_jobs(mb_mgr, pKeys, pIV, pSrcData,
                                         pDstData, packetLen, DECRYPT, 4);
                else
                        IMB_ZUC_EEA3_4_BUFFER(mb_mgr,
                                              (const void * const *)pKeys,
                                              (const void * const *)pIV,
                                              (const void * const *)pSrcData,
                                              (void **)pDstData, packetLen);
                for (j = 0; j < 4; j++) {
                        uint8_t *pDst8 = (uint8_t *)pDstData[j];
                        int retTmp;
                        char msg[50];

                        snprintf(msg, sizeof(msg),
                                "Validate ZUC 4 block test %u, index %u (Dec):",
                                i + 1, j);
                        retTmp = test_output(pDst8, testEEA3_vectors[i].plaintext,
                                     packetLen[j],
                                     testEEA3_vectors[i].length_in_bits, msg);
                        if (retTmp < 0)
                                ret = retTmp;
                }
        }
        return ret;
};

int validate_zuc_EEA_n_block(struct MB_MGR *mb_mgr, uint8_t **pSrcData,
                             uint8_t **pDstData, uint8_t **pKeys, uint8_t **pIV,
                             uint32_t numBuffs, unsigned int job_api)
{
        uint32_t i, j;
        int ret = 0;

        assert(numBuffs > 0);
        for (i = 0; i < NUM_ZUC_EEA3_TESTS; i++) {
                uint32_t packetLen[MAXBUFS];

                for (j = 0; j <= (numBuffs - 1); j++) {
                        memcpy(pKeys[j], testEEA3_vectors[i].CK,
                               ZUC_KEY_LEN_IN_BYTES);
                        zuc_eea3_iv_gen(testEEA3_vectors[i].count,
                                        testEEA3_vectors[i].Bearer,
                                        testEEA3_vectors[i].Direction,
                                        pIV[j]);
                        memcpy(pSrcData[j], testEEA3_vectors[i].plaintext,
                               (testEEA3_vectors[i].length_in_bits + 7) / 8);
                        packetLen[j] =
                            (testEEA3_vectors[i].length_in_bits + 7) / 8;
                }
                if (job_api)
                        submit_eea3_jobs(mb_mgr, pKeys, pIV, pSrcData,
                                         pDstData, packetLen, ENCRYPT,
                                         numBuffs);
                else
                        IMB_ZUC_EEA3_N_BUFFER(mb_mgr,
                                              (const void * const *)pKeys,
                                              (const void * const *)pIV,
                                              (const void * const *)pSrcData,
                                              (void **)pDstData, packetLen,
                                              numBuffs);

                for (j = 0; j <= (numBuffs - 1); j++) {
                        uint8_t *pDst8 = (uint8_t *)pDstData[j];
                        int retTmp;
                        char msg[50];

                        snprintf(msg, sizeof(msg),
                                "Validate ZUC n block test %u, index %u (Enc):",
                                i + 1, j);
                        retTmp = test_output(pDst8, testEEA3_vectors[i].ciphertext,
                                     packetLen[j],
                                     testEEA3_vectors[i].length_in_bits, msg);
                        if (retTmp < 0)
                                ret = retTmp;
                }
                for (j = 0; j <= (numBuffs - 1); j++) {
                        memcpy(pSrcData[j], testEEA3_vectors[i].ciphertext,
                               (testEEA3_vectors[i].length_in_bits + 7) / 8);
                }
                if (job_api)
                        submit_eea3_jobs(mb_mgr, pKeys, pIV, pSrcData,
                                         pDstData, packetLen, DECRYPT,
                                         numBuffs);
                else
                        IMB_ZUC_EEA3_N_BUFFER(mb_mgr,
                                              (const void * const *)pKeys,
                                              (const void * const *)pIV,
                                              (const void * const *)pSrcData,
                                              (void **)pDstData, packetLen,
                                              numBuffs);
                for (j = 0; j <= (numBuffs - 1); j++) {
                        uint8_t *pDst8 = (uint8_t *)pDstData[j];
                        int retTmp;
                        char msg[50];

                        snprintf(msg, sizeof(msg),
                                "Validate ZUC n block test %u, index %u (Dec):",
                                i + 1, j);
                        retTmp = test_output(pDst8, testEEA3_vectors[i].plaintext,
                                     packetLen[j],
                                     testEEA3_vectors[i].length_in_bits, msg);
                        if (retTmp < 0)
                                ret = retTmp;
                }

        }
        return ret;
};

int validate_zuc_EIA_1_block(struct MB_MGR *mb_mgr, uint8_t *pSrcData,
                             uint8_t *pDstData, uint8_t *pKeys, uint8_t *pIV,
                             unsigned int job_api)
{
        uint32_t i;
        int retTmp, ret = 0;
        uint32_t byteLength;

        for (i = 0; i < NUM_ZUC_EIA3_TESTS; i++) {
                memcpy(pKeys, testEIA3_vectors[i].CK, ZUC_KEY_LEN_IN_BYTES);

                zuc_eia3_iv_gen(testEIA3_vectors[i].count,
                                testEIA3_vectors[i].Bearer,
                                testEIA3_vectors[i].Direction,
                                pIV);
                byteLength = (testEIA3_vectors[i].length_in_bits + 7) / 8;
                memcpy(pSrcData, testEIA3_vectors[i].message, byteLength);
                if (job_api)
                        submit_eia3_job(mb_mgr, pKeys, pIV,
                                        pSrcData, pDstData,
                                        testEIA3_vectors[i].length_in_bits);
                else
                        IMB_ZUC_EIA3_1_BUFFER(mb_mgr, pKeys, pIV, pSrcData,
                                            testEIA3_vectors[i].length_in_bits,
                                            (uint32_t *)pDstData);
                retTmp =
                    memcmp(pDstData, &testEIA3_vectors[i].mac,
                           sizeof(((struct test128EIA3_vectors_t *)0)->mac));
                if (retTmp) {
                        printf("Validate ZUC 1 block  test %d (Int): FAIL\n",
                               i + 1);
                        byte_hexdump("Expected",
                                     (const uint8_t *)&testEIA3_vectors[i].mac,
                                     ZUC_DIGEST_LEN);
                        byte_hexdump("Found", pDstData, ZUC_DIGEST_LEN);
                        ret = retTmp;
                }
#ifdef DEBUG
                else
                        printf("Validate ZUC 1 block  test %d (Int): PASS\n",
                               i + 1);
#endif
                fflush(stdout);
        }
        return ret;
};

int validate_zuc_algorithm(struct MB_MGR *mb_mgr, uint8_t *pSrcData,
                           uint8_t *pDstData, uint8_t *pKeys, uint8_t *pIV)
{
        uint32_t i;
        int ret = 0;
        union SwapBytes {
                uint8_t sbb[8];
                uint32_t sbw[2];
        } swapBytes;

        for (i = 0; i < NUM_ZUC_ALG_TESTS; i++) {
                memcpy(pKeys, testZUC_vectors[i].CK, ZUC_KEY_LEN_IN_BYTES);
                memcpy(pIV, testZUC_vectors[i].IV, ZUC_IV_LEN_IN_BYTES);
                memset(pSrcData, 0, 8);
                IMB_ZUC_EEA3_1_BUFFER(mb_mgr, pKeys, pIV, pSrcData, pDstData,
                                      8);
                swapBytes.sbw[0] = bswap4(testZUC_vectors[i].Z[0]);
                swapBytes.sbw[1] = bswap4(testZUC_vectors[i].Z[1]);
                ret = memcmp(pDstData, swapBytes.sbb, 8);
                if (ret)
                        printf("ZUC 1 algorithm test %d: FAIL\n", i);
#ifdef DEBUG
                else
                        printf("ZUC 1 algorithm test %d: PASS\n", i);
#endif
        }
        return ret;
};
/*****************************************************************************
 ** @description - utility function to dump test buffers$
 ** $
 ** @param message [IN] - debug message to print$
 ** @param ptr [IN] - pointer to beginning of buffer.$
 ** @param len [IN] - length of buffer.$
 *****************************************************************************/
static void byte_hexdump(const char *message, const uint8_t *ptr, int len)
{
        int ctr;

        printf("%s:\n", message);
        for (ctr = 0; ctr < len; ctr++) {
                printf("0x%02X ", ptr[ctr] & 0xff);
                if (!((ctr + 1) % 16))
                        printf("\n");
        }
        printf("\n");
        printf("\n");
};
