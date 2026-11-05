**Nightly build**: the newest code, built automatically. Less tested than a
release, so keep a backup of your keys.

- **installed**: for most people.
- **portable**: everything in one folder, e.g. a USB stick (not on macOS).
- **android**: the `.apk` installs on a phone. Most phones need `arm64-v8a`.

**New: GpgFrontend for Android.** The phone app is new and needs testers. Please
try it on your phone and tell me what works and what does not.

Found a bug, or something feels wrong? Please tell me on [GitHub Issues][issues].
Every report helps, and I fix reported bugs as soon as I can.

To check a download, put it next to `SHA256SUMS.txt` and run:

```sh
sha256sum -c --ignore-missing SHA256SUMS.txt       # Linux
shasum -a 256 -c --ignore-missing SHA256SUMS.txt   # macOS
```

To prove it was built on GitHub, use [cosign][cosign]:

```sh
cosign verify-blob SHA256SUMS.txt --bundle SHA256SUMS.txt.sigstore.json \
  --certificate-identity https://github.com/saturneric/GpgFrontend/.github/workflows/build.yml@refs/heads/main \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com
```

[issues]: https://github.com/saturneric/GpgFrontend/issues
[cosign]: https://docs.sigstore.dev/cosign/system_config/installation/
