**Nightly build**: the newest code, built automatically. Less tested than a
release, so keep a backup of your keys.

- **installed**: for most people.
- **portable**: everything in one folder, e.g. a USB stick (not on macOS).

Found a bug? Tell us on [GitHub Issues](https://github.com/saturneric/GpgFrontend/issues).

To check a download, put it next to `SHA256SUMS.txt` and run:

```sh
sha256sum -c --ignore-missing SHA256SUMS.txt       # Linux
shasum -a 256 -c --ignore-missing SHA256SUMS.txt   # macOS
```

To prove it was built on GitHub, use
[cosign](https://docs.sigstore.dev/cosign/system_config/installation/):

```sh
cosign verify-blob SHA256SUMS.txt --bundle SHA256SUMS.txt.sigstore.json \
  --certificate-identity https://github.com/saturneric/GpgFrontend/.github/workflows/build.yml@refs/heads/main \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com
```
