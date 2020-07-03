# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## Unreleased
### Added
- Allow sending data to object aggregated interfaces.
- Allow using UUIDv5 to derive the hardware id.
- Add support for esp-idf 4.x build system.

### Changed
- The hardware id now will use UUIDv5 by default. This means that if you were using an old version
  of the SDK, this version will change the generated device id, requiring a new credentials secret.
  If you need to keep the old device id, disable `Use UUIDv5 to derive the hardware ID` in the
  Astarte SDK settings with `make menuconfig`.

## [0.11.1] - 2020-05-18
### Added
- Add a function to unset a properties path.
- More detailed return error codes.
- Add wrappers to set values on properties interfaces.
- Add SDK version in `astarte.h`.
- Add device hardware id getter.

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
