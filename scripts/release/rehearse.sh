#!/usr/bin/env bash
# Copyright (C) 2021-2024 Saturneric <eric@bktus.com>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# The update chain of main's nightly, end to end, on this machine (a beta
# from release/X.Y goes the same way):
#
#   CI stage -> signing host promote -> index -> HTTPS mirror -> verify
#
#   scripts/release/rehearse.sh <dir> [--serve]
#
# Everything is real but the buckets (directories, through rclone) and
# Sigstore (a fake cosign binding the bundle to this workflow, ref and
# commit, as the real one does). The build is this repository's HEAD on
# main, with dummy files for artifacts; the keys, the trust root and the
# index-keys list are made fresh in <dir> and are worth nothing. The mirror
# is https://localhost:<port>/ with a throwaway CA in <dir>/ca.pem;
# gfrelease verify then reads it over HTTPS as a client would, deep: every
# artifact downloaded and its hash checked.
#
# With --serve the mirror stays up (Ctrl-C ends it), and <dir>/client.env
# holds what a rehearsal client needs: the bases, the trust files and the
# CA, and the CMake options that build such a client (GF_REHEARSAL_CMAKE).
# Needs minisign, rclone, openssl, curl and git.
set -euo pipefail

dir="${1:?usage: $0 <dir> [--serve]}"
serve="${2:-}"
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
top="$(git -C "$here" rev-parse --show-toplevel)"
gfr=(python3 "$here/gfrelease")

for tool in minisign rclone openssl curl git; do
  command -v "$tool" >/dev/null || { echo "rehearse: $tool is missing" >&2; exit 1; }
done
[ ! -e "$dir" ] || { echo "rehearse: $dir exists; give a new directory" >&2; exit 1; }
mkdir -p "$dir"/{keys,staging,www/update,www/builds,state,candidate} \
  "$dir"/trust/{root,index-keys}
dir="$(cd "$dir" && pwd)"
export RCLONE_CONFIG=/dev/null

echo "== keys and a TLS certificate for localhost"
for key in index root-a root-b g; do
  minisign -G -W -s "$dir/keys/$key.key" -p "$dir/keys/$key.pub" >/dev/null
done
openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-256 -nodes -days 2 \
  -subj "/CN=gfrelease rehearsal CA" -keyout "$dir/ca.key" -out "$dir/ca.pem" \
  -addext basicConstraints=critical,CA:TRUE \
  -addext keyUsage=critical,keyCertSign,cRLSign 2>/dev/null
openssl req -newkey ec -pkeyopt ec_paramgen_curve:P-256 -nodes \
  -subj "/CN=localhost" -keyout "$dir/tls.key" -out "$dir/tls.csr" 2>/dev/null
openssl x509 -req -in "$dir/tls.csr" -CA "$dir/ca.pem" -CAkey "$dir/ca.key" \
  -CAcreateserial -days 2 -out "$dir/tls.pem" \
  -extfile <(printf '%s\n' 'subjectAltName=DNS:localhost,IP:127.0.0.1' \
    'keyUsage=critical,digitalSignature' 'extendedKeyUsage=serverAuth') 2>/dev/null
port="$(python3 -c 'import socket; s=socket.socket(); s.bind(("127.0.0.1",0)); print(s.getsockname()[1])')"
base="https://localhost:${port}"

echo "== the product, pointed at the rehearsal mirror, and its trust"
repo=saturneric/GpgFrontend
workflow=.github/workflows/build.yml
# Root 1 and index-keys list 1 by hand: `gfrelease root new` and
# `index-keys renew` only ever follow one there is.
python3 -I - "$here/gfrelease" "$top/release/product.json" "$dir" "$base" \
  "$repo" "$workflow" <<'PY'
import json, sys
from pathlib import Path
gfr, src, d, base, repo, workflow = sys.argv[1:]
sys.path.insert(0, gfr)
import layout, minisig, trust
p = json.load(open(src))
p["bases"] = {"update": [f"{base}/update/"], "builds": [f"{base}/builds/"]}
json.dump(p, open(f"{d}/product.json", "w"), indent=2)
pub = {n: minisig.public_key_of(Path(d, "keys", f"{n}.pub"))
       for n in ("root-a", "root-b", "g", "index")}
ids = {n: minisig.key_id(k) for n, k in pub.items()}
doc = {"schema": 1, "product": p["slug"], "version": 1,
       "expires": "2099-12-31T00:00:00Z"}
root = dict(doc, type=trust.ROOT_KIND,
            keys={ids[n]: pub[n] for n in ("root-a", "root-b", "g")},
            roles={"root": {"keyids": [ids["root-a"], ids["root-b"]],
                            "threshold": 2},
                   "index_list": {"keyids": [ids["g"]], "threshold": 1}},
            host_policy={"repository": repo, "workflow": workflow,
                         "stable_pgp": {"fingerprints": ["A" * 40]}})
Path(d, "trust/root/1.root.json").write_bytes(trust.dump(root))
keys = dict(doc, type=trust.INDEX_KEYS_KIND, keys={ids["index"]: pub["index"]})
path = Path(d, "trust/index-keys/1.json")
path.write_bytes(trust.dump(keys))
minisig.sign(Path(d, "keys/g.key"), path,
             layout.index_keys_comment(p["slug"], 1))
PY
cp "$top/release/policy.toml" "$dir/policy.toml"
slug="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["slug"])' "$dir/product.json")"
prefix="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["artifact_prefix"])' "$dir/product.json")"
export GFR_PRODUCT="$dir/product.json" GFR_POLICY="$dir/policy.toml" \
  GFR_ROOT_DIR="$dir/trust/root" GFR_INDEX_KEYS_DIR="$dir/trust/index-keys"
for key in root-a root-b; do
  "${gfr[@]}" root sign "$dir/trust/root/1.root.json" -s "$dir/keys/$key.key"
done
"${gfr[@]}" root verify

echo "== CI: a candidate of HEAD, signed for main, staged"
commit="$(git -C "$top" rev-parse HEAD)"
version="$(python3 "$here/gitversion.py" --repo "$top" --rev "$commit" --ref main)"
filesafe="$(python3 "$here/gitversion.py" --repo "$top" --rev "$commit" --ref main \
  --field filesafe)"
for leg in linux-x86_64.AppImage windows-x86_64.msi windows-x86_64-portable.zip \
           macos-15.dmg android-arm64-v8a.apk; do
  head -c 4096 /dev/urandom > "$dir/candidate/${prefix}${filesafe}-${leg}"
done
(cd "$dir/candidate" && sha256sum -- * | LC_ALL=C sort -k2 > SHA256SUMS.txt)
cat > "$dir/cosign" <<PY
#!$(command -v python3)
import hashlib, json, sys
args = sys.argv[2:]
opt = lambda n: args[args.index(n) + 1] if n in args else None
blob = hashlib.sha256(open(args[-1], "rb").read()).hexdigest()
if sys.argv[1] == "sign-blob":
    json.dump({"identity": opt("--identity"), "sha": opt("--sha"), "blob": blob},
              open(opt("--bundle"), "w"))
    sys.exit(0)
claims = json.load(open(opt("--bundle")))
want = {"identity": opt("--certificate-identity"),
        "sha": opt("--certificate-github-workflow-sha"), "blob": blob}
sys.exit(0 if claims == want else "fake cosign: verification failed")
PY
chmod +x "$dir/cosign"
"$dir/cosign" sign-blob --bundle "$dir/candidate/SHA256SUMS.txt.sigstore.json" \
  --identity "https://github.com/${repo}/${workflow}@refs/heads/main" \
  --sha "$commit" "$dir/candidate/SHA256SUMS.txt"
"${gfr[@]}" stage --candidate "$dir/candidate" --staging "$dir/staging" \
  --ref refs/heads/main --commit "$commit" --version "$version" \
  --filesafe "$filesafe" --built-at "$(date -u +%Y-%m-%dT%H:%M:%SZ)"

echo "== signing host: promote, sign, publish the index"
export GFR_STAGING="$dir/staging" GFR_BUILDS="$dir/www/builds/$slug" \
  GFR_UPDATE="$dir/www/update/$slug" GFR_STATE_DIR="$dir/state" \
  GFR_GIT_DIR="$top" GFR_COSIGN="$dir/cosign" \
  GFR_INDEX_KEY="$dir/keys/index.key" GFR_INDEX_PUB="$dir/keys/index.pub"
"${gfr[@]}" run

echo "== the mirror, over HTTPS"
python3 - "$dir" "$port" > "$dir/server.log" 2>&1 <<'PY' &
import functools, http.server, ssl, sys
d, port = sys.argv[1], int(sys.argv[2])
handler = functools.partial(http.server.SimpleHTTPRequestHandler, directory=f"{d}/www")
httpd = http.server.ThreadingHTTPServer(("127.0.0.1", port), handler)
ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
ctx.load_cert_chain(f"{d}/tls.pem", f"{d}/tls.key")
httpd.socket = ctx.wrap_socket(httpd.socket, server_side=True)
httpd.serve_forever()
PY
server=$!
trap 'kill "$server" 2>/dev/null || true' EXIT
for _ in $(seq 50); do
  curl -sf --cacert "$dir/ca.pem" "$base/update/$slug/v1/index.json" >/dev/null && break
  sleep 0.1
done

echo "== anyone: verify over HTTPS, deep"
unset GFR_UPDATE GFR_BUILDS
"${gfr[@]}" verify --cafile "$dir/ca.pem" --deep

cat > "$dir/client.env" <<EOF
GF_REHEARSAL_PRODUCT=$slug
GF_REHEARSAL_UPDATE_BASE=$base/update/
GF_REHEARSAL_UPDATE_DIR=$dir/www/update
GF_REHEARSAL_BUILDS_BASE=$base/builds/
GF_REHEARSAL_TRUST=$dir/trust
GF_REHEARSAL_CA=$dir/ca.pem
GF_REHEARSAL_NIGHTLY=$version
# A client of this mirror: configure a build tree with these (and never
# distribute it), then Help > Check for Updates.
GF_REHEARSAL_CMAKE="-DGF_RELEASE_UPDATE_BASES_OVERRIDE=$base/update/ -DGF_RELEASE_BUILDS_BASES_OVERRIDE=$base/builds/ -DGF_RELEASE_TRUST_OVERRIDE=$dir/trust -DGF_RELEASE_CA_OVERRIDE=$dir/ca.pem"
EOF
echo "rehearsal: nightly $version published and verified at $base"
if [ "$serve" = "--serve" ]; then
  echo "serving; client settings in $dir/client.env (Ctrl-C ends)"
  wait "$server"
fi
