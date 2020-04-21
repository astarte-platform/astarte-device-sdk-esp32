# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## Unreleased
### Added
- Allow sending data to object aggregated interfaces.

## [0.11.0] - Unreleased
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
