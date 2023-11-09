<!---
  Copyright 2018-2023 SECO Mind Srl

  SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
-->

# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.1.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]
### Added
- `user_data` field is added to the `astarte_device_config_t` and `astarte_device_*_event_t`
  structs. This field can be used to pass user data to callback functions.

### Changed
- Return value of `uuid_generate_v5` and `astarte_hwid_encode` functions from `void` to
`astarte_err_t`.`

## [1.2.0] - 2023-07-19
### Added
- Add support for server validation through ESP x509 Certificate Bundle.
- Add maximum JWT size to SDK configuration options.
- Add new deserialization utilities contained in `astarte_bson_deserializer.h`.
- Add README section on how to use the (de)serialization utilities.
- Add the typedefines `astarte_bson_serializer_handle_t`, `credential_type_t`,
  `astarte_ptr_list_entry_t` and `astarte_pairing_config_t`.
- Add new `astarte_bson_serializer_new` function.

### Deprecated
- BSON deserialization utility `astarte_bson.h`. Use the new deserialization functions contained
  in `astarte_bson_deserializer.h`.
- Direct use of the enums and structs `astarte_err_t`, `astarte_byte_array_t`,
  `astarte_bson_serializer_t`, `credential_type_t`, `astarte_list_head_t`, `astarte_ptr_list_entry_t`
  and `astarte_pairing_config`.
- Serializer initialization function `astarte_bson_serializer_init`. Use the new
  `astarte_bson_serializer_new` instead.

### Removed
- Support for ESP IDF v3.x.

### Fixed
- Fixed failure in writing the private TLS key following a hard reset performed during the partition
formatting procedure (Available since IDF v5.1).

## [1.1.3] - 2023-05-18
### Fixed
- Fix crash when using an invalid client certificate.

## [1.1.2] - 2023-04-13
### Fixed
- Fix hardware ID generation for IDF 5.0.

## [1.1.1] - 2023-04-07

## [1.0.5] - 2023-04-07
### Added
- Add Empty Cache support.

## [1.1.0] - 2023-02-23
### Added
- Add support to ESP-IDF v5.0

### Fixed
- Fix a bug preventing parsing of bson files containing arrays.

## [1.0.4] - Unreleased
### Added
- Add check that prevents sending an interface with both major and minor version set to 0

## [1.0.3] - 2022-07-05

## [1.0.2] - 2022-04-14
### Added
- Add support to server-owned property unset, using `unset_event_callback`.

### Changed
- Functions such as `astarte_device_set_string_property` should make use of `const char *`, instead
  of `char *`.

## [1.0.1] - 2021-12-23
### Added
- Allow passing an explicit realm to `astarte_device_config_t`.
- Add function for generating random UUIDv4.
- Add astarte_bson_serializer_append_<T>_array functions to BSON serializer (for each supported
  type).
- Add astarte_device_stream_<T>_array(_with_timestamp) functions to astarte_device (for each
  supported type) in order to enable sending arrays to array typed datastreams.

## [1.0.0] - 2021-07-02

## [1.0.0-rc.0] - 2021-05-10
### Changed
- `astarte_device_add_interface` **incompatible API change**, the caller now has to provide a
  pointer to an `astarte_interface_t` struct.
- Make sure the device only subscribes to server owned interfaces. This avoids that published
  messages are sent back to the device.

## [1.0.0-beta.2] - 2021-03-23
### Added
- Add `astarte_device_stop` function to stop the internal MQTT client.
- Add connection and disconnection callbacks called after device connection/disconnection.

### Changed
- `astarte_device_start` now returns `astarte_err_t` to check if the device was succesfully started.

## [1.0.0-beta.1] - 2021-02-16
### Added
- Allow sending data to object aggregated interfaces.
- Allow using UUIDv5 to derive the hardware id.
- Add support for esp-idf 4.x build system.

### Changed
- The hardware id now will use UUIDv5 by default. This means that if you were using an old version
  of the SDK, this version will change the generated device id, requiring a new credentials secret.
  If you need to keep the old device id, disable `Use UUIDv5 to derive the hardware ID` in the
  Astarte SDK settings with `make menuconfig`.

## [0.11.4] - 2021-01-26

## [0.11.2] - 2020-08-31
### Added
- Add device hardware id getter.

### Fixed
- Fix a bug preventing device reinitialization if credentials were not retrieved correctly.

## [0.11.1] - 2020-05-18
### Added
- Add a function to unset a properties path.
- More detailed return error codes.
- Add wrappers to set values on properties interfaces.
- Add SDK version in `astarte.h`.

### Changed
- Most functions will no longer return ASTARTE_ERR, but a more specific error. Code checking for
  `err == ASTARTE_ERR` will most likely need to be changed.

### Fixed
- Correctly interpret the document length as a 32 bit number when appending a BSON document.

## [0.11.0] - 2020-04-13
### Added
- Allow passing an explicit credentials secret to the device instead of depending from the on-board
  agent registration.
- Add `astarte_device_is_connected` function.

## [0.11.0-rc.1] - 2020-03-26
### Changed
- Deprecate implicit credentials initialization, the user should now call `astarte_credentials_init`
  in its `main` before calling `astarte_device_init`.
- Improve memory footprint.
- Use ECDSA for keys instead of RSA.

### Fixed
- Avoid reinitializing a valid device due to connectivity errors.

## [0.11.0-rc.0] - 2020-02-27
### Fixed
- Handle client certificate expiration by requesting new credentials to Pairing API.
- Fix crash due to printing a NULL string in astarte_pairing.

## [0.10.2] - 2019-12-09
### Added
- Allow setting an explicit hwid instead of using the hardware generated one.
- Add UUID library, allowing to generate a UUIDv5 to be used as hwid.

## [0.10.1] - 2019-10-02

## [0.10.0] - 2019-04-17

## [0.10.0-rc.1] - 2019-04-10
### Fixed
- MQTT handler was not compiling with -Werror.

## [0.10.0-rc.0] - 2019-04-04
### Added
- Initial SDK release, supports pairing and publish/receive of basic Astarte types.
