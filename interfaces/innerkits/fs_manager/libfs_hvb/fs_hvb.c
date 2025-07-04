/*
 * Copyright (c) 2022-2023 Huawei Device Co., Ltd.
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

#include "fs_hvb.h"
#include "fs_dm.h"
#include "securec.h"
#include "fs_manager/ext4_super_block.h"
#include "fs_manager/erofs_super_block.h"
#include <stdint.h>
#include "beget_ext.h"
#include "init_utils.h"
#include <libhvb.h>
#include <libgen.h>
#include "hvb_sm2.h"
#include "hvb_sm3.h"
#include "hookmgr.h"
#include "bootstage.h"
#include "fs_manager.h"
#include "list.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define FS_HVB_VERITY_TARGET_MAX 512
#define FS_HVB_BLANK_SPACE_ASCII 32
#define FS_HVB_SECTOR_BYTES 512
#define FS_HVB_UINT64_MAX_LENGTH 64
#define FS_HVB_HASH_ALG_STR_MAX 32

#define FS_HVB_DIGEST_SHA256_BYTES 32
#define FS_HVB_DIGEST_SHA512_BYTES 64
#define FS_HVB_DIGEST_SM3_BYTES 32
#define FS_HVB_DIGEST_MAX_BYTES FS_HVB_DIGEST_SHA512_BYTES
#define FS_HVB_DIGEST_STR_MAX_BYTES 128
#define FS_HVB_DEVPATH_MAX_LEN 128
#define FS_HVB_RVT_PARTITION_NAME "rvt"
#define FS_HVB_EXT_RVT_PARTITION_NAME "ext_rvt"
#define FS_HVB_CMDLINE_PATH "/proc/cmdline"
#define FS_HVB_PARTITION_PREFIX "/dev/block/by-name/"
#define HVB_CMDLINE_EXT_CERT_DIGEST "ohos.boot.hvb.ext_rvt"
#define FS_HVB_SNAPSHOT_PREFIX "/dev/block/"

#define HASH_ALGO_SHA256 0
#define HASH_ALGO_SHA256_STR "sha256"
#define HASH_ALGO_SHA512 2
#define HASH_ALGO_SHA512_STR "sha512"
#define HASH_ALGO_SM3 3
#define HASH_ALGO_SM3_STR "sm3"

#define FS_HVB_RETURN_ERR_IF_NULL(__ptr)             \
    do {                                                \
        if ((__ptr) == NULL) {                          \
            BEGET_LOGE("error, %s is NULL\n", #__ptr); \
            return -1;                                  \
        }                                               \
    } while (0)

static struct hvb_verified_data *g_vd = NULL;
static struct hvb_verified_data *g_extVd = NULL;

static enum hvb_errno GetCurrVerifiedData(InitHvbType hvbType, struct hvb_verified_data ***currVd)
{
    if (currVd == NULL) {
        BEGET_LOGE("error, invalid input");
        return HVB_ERROR_INVALID_ARGUMENT;
    }
    if (hvbType == MAIN_HVB) {
        *currVd = &g_vd;
    } else if (hvbType == EXT_HVB) {
        *currVd = &g_extVd;
    } else {
        BEGET_LOGE("error, invalid hvb type");
        return HVB_ERROR_INVALID_ARGUMENT;
    }

    return HVB_OK;
}

static char FsHvbBinToHexChar(char octet)
{
    return octet < 10 ? '0' + octet : 'a' + (octet - 10);
}

static char FsHvbHexCharToBin(char hex)
{
    return '0' <= hex && hex <= '9' ? hex - '0' : 10 + (hex - 'a');
}

static int FsHvbGetHashStr(char *str, size_t size, InitHvbType hvbType)
{
    if (hvbType == MAIN_HVB) {
        return FsHvbGetValueFromCmdLine(str, size, HVB_CMDLINE_HASH_ALG);
    }

    if (hvbType == EXT_HVB) {
        if (memcpy_s(str, size, HASH_ALGO_SHA256_STR, strlen(HASH_ALGO_SHA256_STR)) != 0) {
            BEGET_LOGE("fail to copy hvb hash str");
            return -1;
        }
        return 0;
    }

    BEGET_LOGE("error, invalid hvbType");
    return -1;
}

static int FsHvbGetCertDigstStr(char *str, size_t size, InitHvbType hvbType)
{
    if (hvbType == MAIN_HVB) {
        return FsHvbGetValueFromCmdLine(str, size, HVB_CMDLINE_CERT_DIGEST);
    }

    if (hvbType == EXT_HVB) {
        return FsHvbGetValueFromCmdLine(str, size, HVB_CMDLINE_EXT_CERT_DIGEST);
    }

    BEGET_LOGE("error, invalid hvbType");
    return -1;
}

static int FsHvbComputeSha256(char *digest, uint32_t size, struct hvb_cert_data *certs, uint64_t num_certs)
{
    int ret;
    uint64_t n;
    struct hash_ctx_t ctx = {0};

    if (size < FS_HVB_DIGEST_SHA256_BYTES) {
        BEGET_LOGE("error, size=%zu", size);
        return -1;
    }

    ret = hash_ctx_init(&ctx, HASH_ALG_SHA256);
    if (ret != HASH_OK) {
        BEGET_LOGE("error 0x%x, sha256 init", ret);
        return -1;
    }

    for (n = 0; n < num_certs; n++) {
        ret = hash_calc_update(&ctx, certs[n].data.addr, certs[n].data.size);
        if (ret != HASH_OK) {
            BEGET_LOGE("error 0x%x, sha256 update", ret);
            return -1;
        }
    }

    ret = hash_calc_do_final(&ctx, NULL, 0, (uint8_t *)digest, size);
    if (ret != HASH_OK) {
        BEGET_LOGE("error 0x%x, sha256 final", ret);
        return -1;
    }

    return 0;
}

static int FsHvbComputeSM3Hash(char *digest, uint32_t *size, struct hvb_cert_data *certs, uint64_t num_certs)
{
    BEGET_CHECK(digest != NULL && certs != NULL && size != NULL, return -1);
    int ret;
    uint64_t n;
    struct sm3_ctx_t ctx = {0};

    if (*size < FS_HVB_DIGEST_SM3_BYTES) {
        BEGET_LOGE("error, size=%zu", *size);
        return -1;
    }

    ret = hvb_sm3_init(&ctx);
    if (ret != SM3_OK) {
        BEGET_LOGE("error 0x%x, sm3 init", ret);
        return -1;
    }

    for (n = 0; n < num_certs; n++) {
        ret = hvb_sm3_update(&ctx, certs[n].data.addr, certs[n].data.size);
        if (ret != SM3_OK) {
            BEGET_LOGE("error 0x%x, sm3 update", ret);
            return -1;
        }
    }

    ret = hvb_sm3_final(&ctx, (uint8_t *)digest, size);
    if (ret != SM3_OK) {
        BEGET_LOGE("error 0x%x, sha256 final", ret);
        return -1;
    }

    return 0;
}

static int FsHvbHexStrToBin(char *bin, size_t bin_size, char *str, size_t str_size)
{
    size_t i;

    if ((str_size & 0x1) != 0 || bin_size < (str_size >> 1)) {
        BEGET_LOGE("error, bin_size %zu str_size %zu", bin_size, str_size);
        return -1;
    }

    for (i = 0; i < (str_size >> 1); i++) {
        bin[i] = (FsHvbHexCharToBin(str[2 * i]) << 4) | FsHvbHexCharToBin(str[2 * i + 1]);
    }

    return 0;
}

static int FsHvbCheckCertChainDigest(struct hvb_cert_data *certs, uint64_t num_certs, InitHvbType hvbType)
{
    int ret;
    size_t digest_len = 0;
    char hashAlg[FS_HVB_HASH_ALG_STR_MAX] = {0};
    char certDigest[FS_HVB_DIGEST_MAX_BYTES] = {0};
    char computeDigest[FS_HVB_DIGEST_MAX_BYTES] = {0};
    char certDigestStr[FS_HVB_DIGEST_STR_MAX_BYTES + 1] = {0};
    uint32_t computeDigestLen = FS_HVB_DIGEST_MAX_BYTES;

    ret = FsHvbGetHashStr(&hashAlg[0], sizeof(hashAlg), hvbType);
    if (ret != 0) {
        BEGET_LOGE("error 0x%x, get hash val from cmdline", ret);
        return ret;
    }

    ret = FsHvbGetCertDigstStr(&certDigestStr[0], sizeof(certDigestStr), hvbType);
    if (ret != 0) {
        BEGET_LOGE("error 0x%x, get digest val from cmdline", ret);
        return ret;
    }

    ret = FsHvbHexStrToBin(&certDigest[0], sizeof(certDigest), &certDigestStr[0], strlen(certDigestStr));
    if (ret != 0) {
        return ret;
    }

    if (strcmp(&hashAlg[0], "sha256") == 0) {
        digest_len = FS_HVB_DIGEST_SHA256_BYTES;
        ret = FsHvbComputeSha256(computeDigest, computeDigestLen, certs, num_certs);
    } else if (strcmp(&hashAlg[0], "sm3") == 0) {
        digest_len = FS_HVB_DIGEST_SM3_BYTES;
        ret = FsHvbComputeSM3Hash(computeDigest, &computeDigestLen, certs, num_certs);
    } else {
        BEGET_LOGE("error, not support alg %s", &hashAlg[0]);
        return -1;
    }

    if (ret != 0) {
        BEGET_LOGE("error 0x%x, compute hash", ret);
        return -1;
    }

    ret = memcmp(&certDigest[0], &computeDigest[0], digest_len);
    if (ret != 0) {
        BEGET_LOGE("error, cert digest not match with cmdline");
        return -1;
    }

    return 0;
}

int FsHvbInit(InitHvbType hvbType)
{
    enum hvb_errno rc;
    struct hvb_verified_data **currVd = NULL;
    char *rootPartitionName = NULL;
    rc = GetCurrVerifiedData(hvbType, &currVd);
    if (rc != HVB_OK) {
        BEGET_LOGE("error 0x%x, GetCurrVerifiedData", rc);
        return rc;
    }

    // return ok if the hvb is already initialized
    if (*currVd != NULL) {
        BEGET_LOGI("Hvb has already been inited");
        return 0;
    }

    if (hvbType == MAIN_HVB) {
        rootPartitionName = FS_HVB_RVT_PARTITION_NAME;
    } else {
        rootPartitionName = FS_HVB_EXT_RVT_PARTITION_NAME;
    }

    rc = hvb_chain_verify(FsHvbGetOps(), rootPartitionName, NULL, currVd);
    if (*currVd == NULL) {
        BEGET_LOGE("error 0x%x, hvb chain verify, vd is NULL", rc);
        return rc;
    }

    if (rc != HVB_OK) {
        BEGET_LOGE("error 0x%x, hvb chain verify", rc);
        hvb_chain_verify_data_free(*currVd);
        *currVd = NULL;
        return rc;
    }

    rc = FsHvbCheckCertChainDigest((*currVd)->certs, (*currVd)->num_loaded_certs, hvbType);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, cert chain hash", rc);
        hvb_chain_verify_data_free(*currVd);
        *currVd = NULL;
        return rc;
    }

    BEGET_LOGI("hvb type %d verify succ in init", hvbType);

    return 0;
}

static int FsHvbFindVabPartition(const char *devName, HvbDeviceParam *devPara)
{
    int rc;
    HOOK_EXEC_OPTIONS options;
    if (devPara == NULL) {
        BEGET_LOGE("error, get devPara");
        return -1;
    }
    if (memcpy_s(devPara->partName, sizeof(devPara->partName), devName, strlen(devName)) != 0) {
        BEGET_LOGE("error, fail to copy shapshot name");
        return -1;
    }
    (void)memset_s(devPara->value, sizeof(devPara->value), 0, sizeof(devPara->value));
    devPara->reverse = 1;
    devPara->len = (int)sizeof(devPara->value);
    options.flags = TRAVERSE_STOP_WHEN_ERROR;
    options.preHook = NULL;
    options.postHook = NULL;
 
    rc = HookMgrExecute(GetBootStageHookMgr(), INIT_VAB_HVBCHECK, (void*)devPara, &options);
    BEGET_LOGW("try find partition from snapshot path %s, ret = %d", devName, rc);
    if (rc == 0) {
        BEGET_LOGW("found partition %s, len=%d", devPara->value, devPara->len);
    }
 
    return rc;
}

static int FsHvbGetCert(struct hvb_cert *cert, const char *devName, struct hvb_verified_data *vd)
{
    enum hvb_errno hr;
    size_t devNameLen = strlen(devName);
    struct hvb_cert_data *p = vd->certs;
    struct hvb_cert_data *end = p + vd->num_loaded_certs;

    // for virtual ab boot, find partition name with snapshot name
    HvbDeviceParam devPara = {};
    if (FsHvbFindVabPartition(devName, &devPara) == 0) {
        devName = devPara.value;
        devNameLen = strlen(devName);
    }

    int bootSlots = GetBootSlots();
    if (bootSlots > 1) {
        if (devNameLen <= FS_HVB_AB_SUFFIX_LEN) {
            BEGET_LOGE("error, devname (%s) is invlaid, devnamelen = %u", devName, devNameLen);
            return -1;
        }
        if (memcmp(devName + devNameLen - FS_HVB_AB_SUFFIX_LEN, "_a", FS_HVB_AB_SUFFIX_LEN) == 0 ||
            memcmp(devName + devNameLen - FS_HVB_AB_SUFFIX_LEN, "_b", FS_HVB_AB_SUFFIX_LEN) == 0) {
            BEGET_LOGI("remove ab suffix in %s to match in hvb certs", devName);
            devNameLen -= FS_HVB_AB_SUFFIX_LEN;
        }
    }

    for (; p < end; p++) {
        if (devNameLen != strlen(p->partition_name)) {
            continue;
        }

        if (memcmp(p->partition_name, devName, devNameLen) == 0) {
            break;
        }
    }

    if (p == end) {
        BEGET_LOGE("error, can't found %s partition", devName);
        return -1;
    }

    hr = hvb_cert_parser(cert, &p->data);
    if (hr != HVB_OK) {
        BEGET_LOGE("error 0x%x, parser hvb cert", hr);
        return -1;
    }

    return 0;
}

static int FsHvbVerityTargetAppendString(char **p, char *end, char *str, size_t len)
{
    errno_t err;

    // check range for append string
    if (*p + len >= end || *p + len < *p) {
        BEGET_LOGE("error, append string overflow");
        return -1;
    }
    // append string
    err = memcpy_s(*p, end - *p, str, len);
    if (err != EOK) {
        BEGET_LOGE("error 0x%x, cp string fail", err);
        return -1;
    }
    *p += len;

    // check range for append blank space
    if (*p + 1 >= end || *p + 1 < *p) {
        BEGET_LOGE("error, append blank space overflow");
        return -1;
    }
    // append blank space
    **p = FS_HVB_BLANK_SPACE_ASCII;
    *p += 1;

    BEGET_LOGI("append string %s", str);

    return 0;
}

static int FsHvbVerityTargetAppendOctets(char **p, char *end, char *octs, size_t octs_len)
{
    size_t i;
    int rc;
    char *str = NULL;
    size_t str_len = octs_len * 2;

    str = calloc(1, str_len + 1);
    if (str == NULL) {
        BEGET_LOGE("error, calloc str fail");
        return -1;
    }

    for (i = 0; i < octs_len; i++) {
        str[2 * i] = FsHvbBinToHexChar((octs[i] >> 4) & 0xf);
        str[2 * i + 1] = FsHvbBinToHexChar(octs[i] & 0xf);
    }

    rc = FsHvbVerityTargetAppendString(p, end, str, str_len);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, append str fail", rc);
    }
    free(str);

    return rc;
}

static int FsHvbVerityTargetAppendNum(char **p, char *end, uint64_t num)
{
    int rc;
    char num_str[FS_HVB_UINT64_MAX_LENGTH] = {0};

    rc = sprintf_s(&num_str[0], sizeof(num_str), "%llu", num);
    if (rc < 0) {
        BEGET_LOGE("error 0x%x, calloc num_str", rc);
        return rc;
    }

    rc = FsHvbVerityTargetAppendString(p, end, num_str, strlen(&num_str[0]));
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, append num_str fail", rc);
    }

    return rc;
}

#define RETURN_ERR_IF_APPEND_STRING_ERR(p, end, str, str_len)                     \
    do {                                                                          \
        int __ret = FsHvbVerityTargetAppendString(p, end, str, str_len);          \
        if (__ret != 0)                                                           \
            return __ret;                                                         \
    } while (0)

#define RETURN_ERR_IF_APPEND_OCTETS_ERR(p, end, octs, octs_len)                    \
    do {                                                                          \
        int __ret = FsHvbVerityTargetAppendOctets(p, end, octs, octs_len);          \
        if (__ret != 0)                                                           \
            return __ret;                                                         \
    } while (0)

#define RETURN_ERR_IF_APPEND_DIGIT_ERR(p, end, num)                               \
    do {                                                                          \
        int __ret = FsHvbVerityTargetAppendNum(p, end, num);                      \
        if (__ret != 0)                                                           \
            return __ret;                                                         \
    } while (0)

static char *FsHvbGetHashAlgStr(unsigned int hash_algo)
{
    char *alg = NULL;

    switch (hash_algo) {
        case HASH_ALGO_SHA256:
            alg = HASH_ALGO_SHA256_STR;
            break;

        case HASH_ALGO_SHA512:
            alg = HASH_ALGO_SHA512_STR;
            break;
        
        case HASH_ALGO_SM3:
            alg = HASH_ALGO_SM3_STR;
            break;

        default:
            alg = NULL;
            break;
    }

    return alg;
}

/*
 * add optional fec data if supported, format as below;
 * <use_fec_from_device> <fec_roots><fec_blocks><fec_start>
 */
static int FsHvbVerityTargetAddFecArgs(struct hvb_cert *cert, char *devPath, char **str, char *end)
{
    FS_HVB_RETURN_ERR_IF_NULL(cert);
    FS_HVB_RETURN_ERR_IF_NULL(devPath);
    FS_HVB_RETURN_ERR_IF_NULL(str);
    FS_HVB_RETURN_ERR_IF_NULL(end);

    // append <device>
    RETURN_ERR_IF_APPEND_STRING_ERR(str, end, USE_FEC_FROM_DEVICE, strlen(USE_FEC_FROM_DEVICE));
    RETURN_ERR_IF_APPEND_STRING_ERR(str, end, &devPath[0], strlen(devPath));

    // append <fec_roots>
    RETURN_ERR_IF_APPEND_STRING_ERR(str, end, FEC_ROOTS, strlen(FEC_ROOTS));
    RETURN_ERR_IF_APPEND_DIGIT_ERR(str, end, cert->fec_num_roots);

    if (cert->data_block_size == 0 || cert->hash_block_size == 0) {
        BEGET_LOGE("error, block size is zero");
        return -1;
    }
    // append <fec_blocks>
    RETURN_ERR_IF_APPEND_STRING_ERR(str, end, FEC_BLOCKS, strlen(FEC_BLOCKS));
    RETURN_ERR_IF_APPEND_DIGIT_ERR(str, end, cert->fec_offset / cert->hash_block_size);

    // append <fec_start>
    RETURN_ERR_IF_APPEND_STRING_ERR(str, end, FEC_START, strlen(FEC_START));
    RETURN_ERR_IF_APPEND_DIGIT_ERR(str, end, cert->fec_offset / cert->data_block_size);

    return 0;
}

static const char *FsHvbGetFsPrefix(const char *devName)
{
    HvbDeviceParam devPara = {};
    if (FsHvbFindVabPartition(devName, &devPara) == 0) {
        return FS_HVB_SNAPSHOT_PREFIX;
    }
    return FS_HVB_PARTITION_PREFIX;
}

/*
 * target->paras is verity table target, format as below;
 * <version> <dev><hash_dev><data_block_size><hash_block_size>
 * <num_data_blocks><hash_start_block><algorithm><digest><salt>
 * [<#opt_params><opt_params>]
 * exp: 1 /dev/sda1 /dev/sda1 4096 4096 262144 262144 sha256 \
       xxxxx
       xxxxx
 */

int FsHvbConstructVerityTarget(DmVerityTarget *target, const char *devName, struct hvb_cert *cert)
{
    char *p = NULL;
    char *end = NULL;
    char *hashALgo = NULL;
    char devPath[FS_HVB_DEVPATH_MAX_LEN] = {0};

    FS_HVB_RETURN_ERR_IF_NULL(target);
    FS_HVB_RETURN_ERR_IF_NULL(devName);
    FS_HVB_RETURN_ERR_IF_NULL(cert);

    target->start = 0;
    target->length = cert->image_len / FS_HVB_SECTOR_BYTES;
    target->paras = calloc(1, FS_HVB_VERITY_TARGET_MAX); // simple it, just calloc a big mem
    if (target->paras == NULL) {
        BEGET_LOGE("error, alloc target paras");
        return -1;
    }

    if (snprintf_s(&devPath[0], sizeof(devPath), sizeof(devPath) - 1, "%s%s",
        ((strchr(devName, '/') == NULL) ? FsHvbGetFsPrefix(devName) : ""), devName) == -1) {
        BEGET_LOGE("error, snprintf_s devPath");
        return -1;
    }

    p = target->paras;
    end = p + FS_HVB_VERITY_TARGET_MAX;

    // append <version>
    RETURN_ERR_IF_APPEND_DIGIT_ERR(&p, end, 1);
    // append <dev>
    RETURN_ERR_IF_APPEND_STRING_ERR(&p, end, &devPath[0], strlen(devPath));
    // append <hash_dev>
    RETURN_ERR_IF_APPEND_STRING_ERR(&p, end, &devPath[0], strlen(devPath));

    if (cert->data_block_size == 0 || cert->hash_block_size == 0) {
        BEGET_LOGE("error, block size is zero");
        return -1;
    }
    // append <data_block_size>
    RETURN_ERR_IF_APPEND_DIGIT_ERR(&p, end, cert->data_block_size);
    // append <hash_block_size>
    RETURN_ERR_IF_APPEND_DIGIT_ERR(&p, end, cert->hash_block_size);
    // append <num_data_blocks>
    RETURN_ERR_IF_APPEND_DIGIT_ERR(&p, end, cert->image_len / cert->data_block_size);
    // append <hash_start_block>
    RETURN_ERR_IF_APPEND_DIGIT_ERR(&p, end, cert->hashtree_offset / cert->hash_block_size);

    // append <algorithm>
    hashALgo = FsHvbGetHashAlgStr(cert->hash_algo);
    if (hashALgo == NULL) {
        BEGET_LOGE("error, hash alg %d is invalid", cert->hash_algo);
        return -1;
    }
    RETURN_ERR_IF_APPEND_STRING_ERR(&p, end, hashALgo, strlen(hashALgo));

    // append <digest>
    RETURN_ERR_IF_APPEND_OCTETS_ERR(&p, end, (char *)cert->hash_payload.digest, cert->digest_size);

    // append <salt>
    RETURN_ERR_IF_APPEND_OCTETS_ERR(&p, end, (char *)cert->hash_payload.salt, cert->salt_size);

    if (cert->fec_size > 0) {
        if (FsHvbVerityTargetAddFecArgs(cert, devPath, &p, end) != 0) {
            return -1;
        }
    }

    // remove last blank
    *(p - 1) = '\0';

    target->paras_len = strlen(target->paras);

    return 0;
}

static int FsHvbCreateVerityTarget(DmVerityTarget *target, char *devName, struct hvb_verified_data *vd)
{
    int rc;
    struct hvb_cert cert = {0};

    rc = FsHvbGetCert(&cert, devName, vd);
    if (rc != 0) {
        return rc;
    }

    rc = FsHvbConstructVerityTarget(target, devName, &cert);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, can't construct verity target", rc);
        return rc;
    }

    return rc;
}

static int FsExtHvbCreateVerityTarget(DmVerityTarget *target, const char *devName,
                                      const char *partition, struct hvb_verified_data *vd)
{
    int rc;
    struct hvb_cert cert = {0};

    rc = FsHvbGetCert(&cert, partition, vd);
    if (rc != 0) {
        return rc;
    }

    rc = FsHvbConstructVerityTarget(target, devName, &cert);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, can't construct verity target", rc);
        return rc;
    }

    return rc;
}

void FsHvbDestoryVerityTarget(DmVerityTarget *target)
{
    if (target != NULL && target->paras != NULL) {
        free(target->paras);
        target->paras = NULL;
    }
}

int FsHvbSetupHashtree(FstabItem *fsItem)
{
    int rc;
    DmVerityTarget target = {0};
    char *dmDevPath = NULL;
    char *devName = NULL;

    FS_HVB_RETURN_ERR_IF_NULL(fsItem);
    FS_HVB_RETURN_ERR_IF_NULL(g_vd);

    // fsItem->deviceName is like /dev/block/platform/xxx/by-name/system
    // for vab boot, is like /dev/block/dm-1
    // we just basename system
    devName = basename(fsItem->deviceName);
    if (devName == NULL) {
        BEGET_LOGE("error, get basename");
        return -1;
    }

    rc = FsHvbCreateVerityTarget(&target, devName, g_vd);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, create verity-table", rc);
        goto exit;
    }

    rc = FsDmCreateDevice(&dmDevPath, devName, &target);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, create dm-verity", rc);
        goto exit;
    }

    rc = FsDmInitDmDev(dmDevPath, true);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, create init dm dev", rc);
        if (dmDevPath != NULL) {
            free(dmDevPath);
        }
        goto exit;
    }

    free(fsItem->deviceName);
    fsItem->deviceName = dmDevPath;

exit:
    FsHvbDestoryVerityTarget(&target);

    return rc;
}

static int FsExtHvbSetupHashtree(const char *devName, const char *partition, char **outPath)
{
    int rc;
    DmVerityTarget target = {0};
    char *tmpDevName = NULL;
    char *dmDevPath = NULL;
    size_t devLen;

    FS_HVB_RETURN_ERR_IF_NULL(g_extVd);

    devLen = strnlen(devName, FS_HVB_MAX_PATH_LEN);
    if (devLen >= FS_HVB_MAX_PATH_LEN) {
        BEGET_LOGE("error, invalid devName");
        return -1;
    }
    tmpDevName = malloc(devLen + 1);
    if (tmpDevName == NULL) {
        BEGET_LOGE("error, fail to malloc");
        return -1;
    }

    (void)memset_s(tmpDevName, devLen + 1, 0, devLen + 1);
    if (memcpy_s(tmpDevName, devLen + 1, devName, devLen) != 0) {
        free(tmpDevName);
        BEGET_LOGE("error, fail to copy");
        return -1;
    }

    rc = FsExtHvbCreateVerityTarget(&target, tmpDevName, partition, g_extVd);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, create verity-table", rc);
        goto exit;
    }

    rc = FsDmCreateDevice(&dmDevPath, basename(tmpDevName), &target);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, create dm-verity", rc);
        goto exit;
    }

    rc = FsDmInitDmDev(dmDevPath, true);
    if (rc != 0) {
        BEGET_LOGE("error 0x%x, create init dm dev", rc);
        if (dmDevPath != NULL) {
            free(dmDevPath);
        }
        goto exit;
    }

    *outPath = dmDevPath;

exit:
    FsHvbDestoryVerityTarget(&target);
    free(tmpDevName);
    return rc;
}

int FsHvbFinal(InitHvbType hvbType)
{
    struct hvb_verified_data **currVd = NULL;

    if (GetCurrVerifiedData(hvbType, &currVd) != HVB_OK) {
        BEGET_LOGE("error GetCurrVerifiedData");
        return -1;
    }
    if (*currVd != NULL) {
        hvb_chain_verify_data_free(*currVd);
        *currVd = NULL;
    }

    return 0;
}

int FsHvbGetValueFromCmdLine(char *val, size_t size, const char *key)
{
    FS_HVB_RETURN_ERR_IF_NULL(val);
    FS_HVB_RETURN_ERR_IF_NULL(key);

    return GetExactParameterFromCmdLine(key, val, size);
}

int VerifyExtHvbImage(const char *devPath, const char *partition, char **outPath)
{
    int rc;
    FS_HVB_RETURN_ERR_IF_NULL(devPath);
    FS_HVB_RETURN_ERR_IF_NULL(partition);
    FS_HVB_RETURN_ERR_IF_NULL(outPath);

    rc = FsHvbInit(EXT_HVB);
    if (rc != 0) {
        BEGET_LOGE("ext hvb error, ret=%d", rc);
        goto exit;
    }

    rc = FsExtHvbSetupHashtree(devPath, partition, outPath);
    if (rc != 0) {
        BEGET_LOGE("error, setup hashtree fail, ret=%d", rc);
    }

exit:
    if (FsHvbFinal(EXT_HVB) != 0) {
        BEGET_LOGE("final ext hvb error, ret=%d", rc);
        return -1;
    }
    return rc;
}

bool CheckAndGetExt4Size(const char *headerBuf, uint64_t *imageSize, const char* image)
{
    ext4_super_block *superBlock = (ext4_super_block *)headerBuf;

    if (headerBuf == NULL || imageSize == NULL || image == NULL) {
        BEGET_LOGE("param is error");
        return false;
    }

    if (superBlock->s_magic == EXT4_SUPER_MAGIC) {
        *imageSize = (uint64_t)superBlock->s_blocks_count_lo * BLOCK_SIZE_UNIT;
        BEGET_LOGI("%s is ext4:[block cout]: %d, [size]: 0x%lx", image,
            superBlock->s_blocks_count_lo, *imageSize);
        return true;
    }
    return false;
}

bool CheckAndGetErofsSize(const char *headerBuf, uint64_t *imageSize, const char* image)
{
    struct erofs_super_block *superBlock = (struct erofs_super_block *)headerBuf;

    if (headerBuf == NULL || imageSize == NULL || image == NULL) {
        BEGET_LOGE("param is error");
        return false;
    }

    if (superBlock->magic == EROFS_SUPER_MAGIC) {
        *imageSize = (uint64_t)superBlock->blocks * BLOCK_SIZE_UINT;
        BEGET_LOGI("%s is erofs:[block cout]: %d, [size]: 0x%lx", image,
            superBlock->blocks, *imageSize);
        return true;
    }
    return false;
}

bool CheckAndGetExtheaderSize(const int fd, uint64_t offset,
                              uint64_t *imageSize, const char* image)
{
    ExtheaderV1 header;
    ssize_t nbytes;
    if (fd < 0 || imageSize == NULL || image == NULL) {
        BEGET_LOGE("param is error");
        return false;
    }

    if (lseek(fd, offset, SEEK_SET) < 0) {
        BEGET_LOGE("lseek %s failed for offset 0x%lx", image, offset);
        return false;
    }

    nbytes = read(fd, &header, sizeof(header));
    if (nbytes != sizeof(header)) {
        BEGET_LOGE("read %s failed.", image);
        return false;
    }

    if (header.magicNumber != EXTHDR_MAGIC) {
        BEGET_LOGW("%s extheader doesnt match, magic is 0x%lx", image, header.magicNumber);
        return false;
    }

    *imageSize = header.partSize;
    return true;
}

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
