/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "parameter.h"

#include <stdint.h>
#include <stdlib.h>

#include "param_comm.h"
#include "init_param.h"
#include "param_init.h"
#include "init_utils.h"
#include "sysparam_errno.h"
#include "securec.h"
#include "beget_ext.h"

int WaitParameter(const char *key, const char *value, int timeout)
{
    BEGET_CHECK(!(key == NULL || value == NULL), return EC_INVALID);
    int ret = SystemWaitParameter(key, value, timeout);
    BEGET_CHECK_ONLY_ELOG(ret == 0, "WaitParameter failed! the errNum is: %d", ret);
    return GetSystemError(ret);
}

uint32_t FindParameter(const char *key)
{
    BEGET_CHECK(key != NULL, return (uint32_t)(-1));
    uint32_t handle = 0;
    int ret = SystemFindParameter(key, &handle);
    if (ret != 0) {
        return (uint32_t)(-1);
    }
    return handle;
}

uint32_t GetParameterCommitId(uint32_t handle)
{
    uint32_t commitId = 0;
    int ret = SystemGetParameterCommitId(handle, &commitId);
    BEGET_CHECK(ret == 0, return (uint32_t)(-1));
    return commitId;
}

int GetParameterName(uint32_t handle, char *name, uint32_t len)
{
    if (name == NULL) {
        return EC_INVALID;
    }
    int ret = SystemGetParameterName(handle, name, len);
    if (ret == 0) {
        return strlen(name);
    }
    BEGET_CHECK_ONLY_ELOG(ret == 0, "GetParameterName failed! the errNum is: %d", ret);
    return GetSystemError(ret);
}

int GetParameterValue(uint32_t handle, char *value, uint32_t len)
{
    if (value == NULL) {
        return EC_INVALID;
    }
    uint32_t size = len;
    int ret = SystemGetParameterValue(handle, value, &size);
    if (ret == 0) {
        return strlen(value);
    }
    BEGET_CHECK_ONLY_ELOG(ret == 0, "GetParameterValue failed! the errNum is: %d", ret);
    return GetSystemError(ret);
}

int GetParameter(const char *key, const char *def, char *value, uint32_t len)
{
    if ((key == NULL) || (value == NULL)) {
        return EC_INVALID;
    }
    int ret = GetParameter_(key, def, value, len);
    return (ret != 0) ? ret : strlen(value);
}

int SetParameter(const char *key, const char *value)
{
    if ((key == NULL) || (value == NULL)) {
        return EC_INVALID;
    }
    int ret = SystemSetParameter(key, value);
    BEGET_CHECK_ONLY_ELOG(ret == 0, "SetParameter failed! the errNum is:%d", ret);
    return GetSystemError(ret);
}

int SaveParameters(void)
{
    int ret = SystemSaveParameters();
    return GetSystemError(ret);
}

const char *GetDeviceType(void)
{
    static const char *productType = NULL;
    const char *deviceType = GetProperty("const.product.devicetype", &productType);
    if (deviceType != NULL) {
        return deviceType;
    }
    return GetProperty("const.build.characteristics", &productType);
}

const char *GetProductModel(void)
{
    return GetProductModel_();
}

const char *GetProductModelAlias(void)
{
    return GetProductModelAlias_();
}

const char *GetManufacture(void)
{
    return GetManufacture_();
}

const char *GetBrand(void)
{
    static const char *productBrand = NULL;
    return GetProperty("const.product.brand", &productBrand);
}

const char *GetMarketName(void)
{
    static const char *marketName = NULL;
    return GetProperty("const.product.name", &marketName);
}

const char *GetProductSeries(void)
{
    static const char *productSeries = NULL;
    return GetProperty("const.build.product", &productSeries);
}

const char *GetSoftwareModel(void)
{
    static const char *softwareModel = NULL;
    return GetProperty("const.software.model", &softwareModel);
}

const char *GetHardwareModel(void)
{
    static const char *hardwareModel = NULL;
    return GetProperty("const.product.hardwareversion", &hardwareModel);
}

const char *GetHardwareProfile(void)
{
    static const char *hardwareProfile = NULL;
    return GetProperty("const.product.hardwareprofile", &hardwareProfile);
}

const char *GetAbiList(void)
{
    static const char *productAbiList = NULL;
    return GetProperty("const.product.cpu.abilist", &productAbiList);
}

const char *GetBootloaderVersion(void)
{
    static const char *productBootloader = NULL;
    return GetProperty("const.product.bootloader.version", &productBootloader);
}

int GetFirstApiVersion(void)
{
    static const char *firstApiVersion = NULL;
    GetProperty("const.product.firstapiversion", &firstApiVersion);
    if (firstApiVersion == NULL) {
        return 0;
    }
    return atoi(firstApiVersion);
}

const char *GetDisplayVersion(void)
{
    static const char *displayVersion = NULL;
    return GetProperty("const.product.software.version", &displayVersion);
}

const char *GetIncrementalVersion(void)
{
    static const char *incrementalVersion = NULL;
    return GetProperty("const.product.incremental.version", &incrementalVersion);
}

const char *GetOsReleaseType(void)
{
    static const char *osReleaseType = NULL;
    return GetProperty("const.ohos.releasetype", &osReleaseType);
}

static const char *GetSdkApiVersion_(void)
{
    static const char *sdkApiVersion = NULL;
    return GetProperty("const.ohos.apiversion", &sdkApiVersion);
}

const char *GetBuildType(void)
{
    static const char *buildType = NULL;
    return GetProperty("const.product.build.type", &buildType);
}

const char *GetBuildUser(void)
{
    static const char *buildUser = NULL;
    return GetProperty("const.product.build.user", &buildUser);
}

const char *GetBuildHost(void)
{
    static const char *buildHost = NULL;
    return GetProperty("const.product.build.host", &buildHost);
}

const char *GetBuildTime(void)
{
    static const char *buildTime = NULL;
    return GetProperty("const.product.build.date", &buildTime);
}

const char *GetSerial(void)
{
    return GetSerial_();
}

int GetDevUdid(char *udid, int size)
{
    return GetDevUdid_(udid, size);
}

static const char *BuildOSFullName(void)
{
    const char release[] = "Release";
    const char *releaseType = GetOsReleaseType();
    const char *fullName = GetFullName_();
    if (fullName == NULL || releaseType == NULL) {
        return NULL;
    }
    if (strncmp(releaseType, release, sizeof(release) - 1) != 0) {
        char *value = calloc(1, OS_FULL_NAME_LEN);
        if (value == NULL) {
            return NULL;
        }
        int length = sprintf_s(value, OS_FULL_NAME_LEN, "%s(%s)", fullName, releaseType);
        if (length < 0) {
            free(value);
            return NULL;
        }
        return value;
    }
    return strdup(fullName);
}

const char *GetOSFullName(void)
{
    static const char *osFullName = NULL;
    if (osFullName != NULL) {
        return osFullName;
    }
    osFullName = BuildOSFullName();
    if (osFullName == NULL) {
        return EMPTY_STR;
    }
    return osFullName;
}

static const char *BuildVersionId(void)
{
    char value[VERSION_ID_MAX_LEN] = {0};
    if (GetDeviceType() == NULL) {
        return NULL;
    }

    int len = sprintf_s(value, VERSION_ID_MAX_LEN, "%s/%s/%s/%s/%s/%s/%s/%s/%s/%s",
        GetDeviceType(), GetManufacture(), GetBrand(), GetProductSeries(),
        GetOSFullName(), GetProductModel(), GetSoftwareModel(),
        GetSdkApiVersion_(), GetIncrementalVersion(), GetBuildType());
    if (len <= 0) {
        return NULL;
    }
    const char *versionId = strdup(value);
    return versionId;
}

const char *GetVersionId(void)
{
    static const char *ohosVersionId = NULL;
    if (ohosVersionId != NULL) {
        return ohosVersionId;
    }
    ohosVersionId = BuildVersionId();
    if (ohosVersionId == NULL) {
        return EMPTY_STR;
    }
    return ohosVersionId;
}

int GetSdkApiVersion(void)
{
    static const char *sdkApiVersion = NULL;
    GetProperty("const.ohos.apiversion", &sdkApiVersion);
    if (sdkApiVersion == NULL) {
        return 0;
    }
    return atoi(sdkApiVersion);
}

int GetSdkMinorApiVersion(void)
{
    static const char *sdkApiMinorVersion = NULL;
    GetProperty("const.ohos.apiminorversion", &sdkApiMinorVersion);
    if (sdkApiMinorVersion == NULL) {
        return -1;
    }
    return atoi(sdkApiMinorVersion);
}

int GetSdkPatchApiVersion(void)
{
    static const char *sdkApiPatchVersion = NULL;
    GetProperty("const.ohos.apipatchversion", &sdkApiPatchVersion);
    if (sdkApiPatchVersion == NULL) {
        return -1;
    }
    return atoi(sdkApiPatchVersion);
}

const char *GetSecurityPatchTag(void)
{
    static const char *securityPatchTag = NULL;
    return GetProperty("const.ohos.version.security_patch", &securityPatchTag);
}

const char *GetBuildRootHash(void)
{
    static const char *buildRootHash = NULL;
    return GetProperty("const.ohos.buildroothash", &buildRootHash);
}

int32_t GetIntParameter(const char *key, int32_t def)
{
    char value[MAX_INT_LEN] = {0};
    uint32_t size = sizeof(value);
    int ret = SystemGetParameter(key, value, &size);
    if (ret != 0) {
        return def;
    }

    long long int result = 0;
    if (StringToLL(value, &result) != 0) {
        return def;
    }
    if (result <= INT32_MIN || result >= INT32_MAX) {
        return def;
    }
    return (int32_t)result;
}

uint32_t GetUintParameter(const char *key, uint32_t def)
{
    char value[MAX_INT_LEN] = {0};
    uint32_t size = sizeof(value);
    int ret = SystemGetParameter(key, value, &size);
    if (ret != 0) {
        return def;
    }

    unsigned long long int result = 0;
    if (StringToULL(value, &result) != 0) {
        return def;
    }
    if (result >= UINT32_MAX) {
        return def;
    }
    return (uint32_t)result;
}

const char *GetDistributionOSName(void)
{
    static const char *distributionOsName = NULL;
    GetProperty("const.product.os.dist.name", &distributionOsName);
    if (distributionOsName == NULL) {
        distributionOsName = EMPTY_STR;
    }
    return distributionOsName;
}

const char *GetDistributionOSVersion(void)
{
    static const char *distributionOsVersion = NULL;
    GetProperty("const.product.os.dist.version", &distributionOsVersion);
    if (distributionOsVersion == NULL) {
        distributionOsVersion = GetOSFullName();
    }
    return distributionOsVersion;
}

int GetDistributionOSApiVersion(void)
{
    static const char *distributionOsApiVersion = NULL;
    GetProperty("const.product.os.dist.apiversion", &distributionOsApiVersion);
    if (distributionOsApiVersion == NULL) {
        distributionOsApiVersion = GetSdkApiVersion_();
    }
    return atoi(distributionOsApiVersion);
}
const char *GetDistributionOSApiName(void)
{
    static const char *distributionOsApiName = NULL;
    GetProperty("const.product.os.dist.apiname", &distributionOsApiName);
    if (distributionOsApiName == NULL) {
        distributionOsApiName = EMPTY_STR;
    }
    return distributionOsApiName;
}

const char *GetDistributionOSReleaseType(void)
{
    static const char *distributionOsReleaseType = NULL;
    GetProperty("const.product.os.dist.releasetype", &distributionOsReleaseType);
    if (distributionOsReleaseType == NULL) {
        distributionOsReleaseType = GetOsReleaseType();
    }
    return distributionOsReleaseType;
}

int GetPerformanceClass(void)
{
    static const char *performanceClass = NULL;
    GetProperty("const.sys.performance_class", &performanceClass);
    if (performanceClass == NULL) {
        return PERFORMANCE_CLASS_HIGH_LEVEL;
    }
    int performanceClassValue = atoi(performanceClass);
    if (performanceClassValue < PERFORMANCE_CLASS_HIGH_LEVEL || performanceClassValue > PERFORMANCE_CLASS_LOW_LEVEL) {
        return PERFORMANCE_CLASS_HIGH_LEVEL;
    }
    return performanceClassValue;
}
