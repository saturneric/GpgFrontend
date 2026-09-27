This is a **nightly build** of GpgFrontend: the latest code, built automatically.
It is less tested than official releases, so don't use it for anything you
can't afford to lose.

#### Which file do I download?

- **installed**: the normal choice. Keys and settings go in the usual place.
- **portable**: everything stays next to the app, e.g. on a USB stick. Anyone
  holding the stick has your keys. (Not available for macOS.)

The two do not share data.

#### Good to know

- Things may change or break between nightly builds.
- Downloads are larger because they include debugging information. The app
  runs just as fast.
- Please report bugs and ideas on GitHub Issues.

#### Checking your download

These files are not GPG-signed; for that, wait for an official release.
`SHA256SUMS.txt` lists every file with its checksum. To check a download,
keep it in the same folder as `SHA256SUMS.txt` and run:

```sh
sha256sum -c --ignore-missing SHA256SUMS.txt       # Linux
shasum -a 256 -c --ignore-missing SHA256SUMS.txt   # macOS
```

To also confirm it was built by this project on GitHub, first run
[cosign](https://docs.sigstore.dev/cosign/system_config/installation/):

```sh
cosign verify-blob SHA256SUMS.txt --bundle SHA256SUMS.txt.sigstore.json \
  --certificate-identity https://github.com/saturneric/GpgFrontend/.github/workflows/build.yml@refs/heads/main \
  --certificate-oidc-issuer https://token.actions.githubusercontent.com
```

Thank you for supporting GpgFrontend!
