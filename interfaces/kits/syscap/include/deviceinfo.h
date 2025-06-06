/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef DEVICEINFO_CSDK_H
#define DEVICEINFO_CSDK_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/**
 * Obtains the device type represented by a string,
 * which can be {@code phone} (or {@code default} for phones), {@code wearable}, {@code liteWearable},
 * {@code tablet}, {@code tv}, {@code car}, or {@code smartVision}.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found device type, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetDeviceType(void);

/**
 * Obtains the device manufacturer represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found device manufacturer, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetManufacture(void);

/**
 * Obtains the device brand represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found device brand, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetBrand(void);

/**
 * Obtains the product name speaded in the market
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found market name, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetMarketName(void);

/**
 * Obtains the product series represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found product series, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetProductSeries(void);

/**
 * Obtains the product model represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found product model, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetProductModel(void);

/**
 * Obtains the product model alias represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found product model, or failed to invoke the internal interface.
 * @since 14
 */
const char *OH_GetProductModelAlias(void);

/**
 * Obtains the software model represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found software model, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetSoftwareModel(void);

/**
 * Obtains the hardware model represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found hardware model, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetHardwareModel(void);

/**
 * Obtains the bootloader version number represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found bootloader version number, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetBootloaderVersion(void);

/**
 * Obtains the application binary interface (Abi) list represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found Abi list, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetAbiList(void);

/**
 * Obtains the security patch tag represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found security patch tag, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetSecurityPatchTag(void);

/**
 * Obtains the product version displayed for customer represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found the product version displayed, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetDisplayVersion(void);

/**
 * Obtains the incremental version represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found the incremental version, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetIncrementalVersion(void);

/**
 * Obtains the OS release type represented by a string.
 *
 * <p>The OS release category can be {@code Release}, {@code Beta}, or {@code Canary}.
 * The specific release type may be {@code Release}, {@code Beta1}, or others alike.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found the OS release type, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetOsReleaseType(void);

/**
 * Obtains the OS full version name represented by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found the OS full version name, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetOSFullName(void);

/**
 * Obtains the SDK API major version number.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return 0 - Not found the SDK API version number, or failed to invoke the internal interface.
 * @since 10
 */
int OH_GetSdkApiVersion(void);

/**
 * Obtains the sdk minor api version number.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return 0 ~ 999 - the sdk minor api version
 *         -1 - not found the sdk minor api version number, or failed to invoke the internal interface.
 * @since 19
 */
int OH_GetSdkMinorApiVersion(void);

/**
 * Obtains the sdk patch api version number.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return 0 ~ 999 - the sdk patch api version
 *         -1 - not found the sdk patch api version number, or failed to invoke the internal interface.
 * @since 19
 */
int OH_GetSdkPatchApiVersion(void);

/**
 * Obtains the first API version number.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return 0 - Not found the first API version number, or failed to invoke the internal interface.
 * @since 10
 */
int OH_GetFirstApiVersion(void);

/**
 * Obtains the version ID by a string.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found version ID, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetVersionId(void);

/**
 * Obtains the build type of the current running OS.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found build type, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetBuildType(void);

/**
 * Obtains the build user of the current running OS.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found build user, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetBuildUser(void);

/**
 * Obtains the build host of the current running OS.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found build host, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetBuildHost(void);

/**
 * Obtains the build time of the current running OS.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found build time, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetBuildTime(void);

/**
 * Obtains the version hash of the current running OS.
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found version hash, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetBuildRootHash(void);

/**
 * Obtains the Distribution OS name represented by a string.
 *
 * <p>Independent Software Vendor (ISV) may distribute OHOS with their own OS name.
 * If ISV not specified, it will return an empty string
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found distribution OS name, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetDistributionOSName(void);

/**
 * Obtains the ISV distribution OS version represented by a string.
 * If ISV not specified, it will return the same value as OH_GetOSFullName
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found distribution OS version, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetDistributionOSVersion(void);

/**
 * Obtains the ISV distribution OS api version represented by a integer.
 * If ISV not specified, it will return the same value as OH_GetSdkApiVersion
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found distribution OS api version, or failed to invoke the internal interface.
 * @since 10
 */
int OH_GetDistributionOSApiVersion(void);

/**
 * Obtains the ISV distribution OS release type represented by a string.
 * If ISV not specified, it will return the same value as OH_GetOsReleaseType
 * @syscap SystemCapability.Startup.SystemInfo
 * @return NULL - Not found distribution OS release type, or failed to invoke the internal interface.
 * @since 10
 */
const char *OH_GetDistributionOSReleaseType(void);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif
#endif
