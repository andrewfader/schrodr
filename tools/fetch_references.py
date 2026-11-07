#!/usr/bin/env python3
"""Fetches the reference recordings named in tests/reference/manifest.tsv into a cache directory
and checks each against its SHA-256 pin. A review tool, not a test: the suite never touches the
network, it reads the measurements that schrodr-reference-extract wrote into the repository. The
recordings are never committed.

    tools/fetch_references.py [CACHE_DIR]          (default ~/.cache/schrodr/references)
    tools/fetch_references.py --pin [CACHE_DIR]    record the hash of any unpinned file

A manifest path is SOURCE:PATH. A source is either a web directory (the file is fetched from
its base URL) or a ZIP archive (fetched once, pinned by its own hash, and the file taken out of
it). A FLAC file is decoded with `flac`, an Ogg or MP3 file with `ffmpeg`, to PATH.wav beside
it, which is what schrodr-reference-extract reads. Every file lands at CACHE_DIR/SOURCE/PATH.

A file whose hash differs from its pin is refused, never silently re-measured.
"""
import hashlib
import os
import subprocess
import sys
import time
import urllib.error
import urllib.parse
import urllib.request
import zipfile

# Each git source is pinned by commit, each archive by its SHA-256.
SOURCES = {
    # University of Iowa Musical Instrument Samples (Lawrence Fritts).
    "iowa": {"base": "https://theremin.music.uiowa.edu/sound%20files/MIS/"},
    # Versilian Studios Chamber Orchestra 2, Community Edition (organ and upright by Simon Dalzell).
    "vsco": {"base": "https://raw.githubusercontent.com/sgossner/VSCO-2-CE/440300901dfe9275fd84e0b7763af1f8443ae62e/"},
    # Sonatina Symphonic Orchestra (the celesta is freesound pack 6166 by stamperadam).
    "sso": {"base": "https://raw.githubusercontent.com/peastman/sso/32bbdb169aef636b8216029a2e056424ba7c2abb/Sonatina%20Symphonic%20Orchestra/Samples/"},
    # jRhodes3d: John Learman's 1977 Rhodes Mark I Stage 73, from the harp connector.
    "jrhodes": {"base": "https://raw.githubusercontent.com/sfzinstruments/jlearman.jRhodes3d/aea5b8d3e11e2f7102593789a4e0a0e41b30271a/"},
    # Versilian Community Sample Library (harpsichords, the Renaissance organ).
    "vcsl": {"base": "https://raw.githubusercontent.com/sgossner/VCSL/c1ea7bcc3c7309650ab0da9d15c9cd1fbc4a4c7e/"},
    # Freesound's high-quality previews (Ogg Vorbis; the originals need a login).
    "freesound": {"base": "https://cdn.freesound.org/previews/"},
    # University of Edinburgh, St Cecilia's Hall sound libraries (DataShare).
    # The unfretted clavichord of the 1740s (handle 10283/9170), recorded April 2025.
    "clavichord": {"zip": "https://datashare.ed.ac.uk/server/api/core/bitstreams/64416ae6-0c5f-414a-b6ea-1d020266402a/content",
                   "sha256": "5018294c5f308a6c7693f695d834fc5d3a9ceda31a29b6ad7c25361a3e153548"},
}

HERE = os.path.dirname(os.path.abspath(__file__))
MANIFEST = os.path.join(HERE, "..", "tests", "reference", "manifest.tsv")


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def fetch(url, dest):
    # Retried: a connection can wait on the desktop firewall's prompt. A partial file resumes.
    for attempt in range(8):
        try:
            have = os.path.getsize(dest + ".part") if os.path.exists(dest + ".part") else 0
            headers = {"User-Agent": "schrodr-fetch-references"}
            if have:
                headers["Range"] = f"bytes={have}-"
            req = urllib.request.Request(url, headers=headers)
            with urllib.request.urlopen(req, timeout=120) as r:
                mode = "ab" if have and r.status == 206 else "wb"
                with open(dest + ".part", mode) as out:
                    while True:
                        block = r.read(1 << 20)
                        if not block:
                            break
                        out.write(block)
            os.replace(dest + ".part", dest)
            return
        except urllib.error.HTTPError as e:
            if e.code == 404:
                raise SystemExit(f"{url}: not found")
            print(f"  retrying {url}: {e}", file=sys.stderr)
            time.sleep(3 * (attempt + 1))
        except OSError as e:
            print(f"  retrying {url}: {e}", file=sys.stderr)
            time.sleep(3 * (attempt + 1))
    raise SystemExit(f"cannot fetch {url}")


def archive(cache, name, pin):
    src = SOURCES[name]
    dest = os.path.join(cache, "archives", name + ".zip")
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    if not os.path.exists(dest):
        print(f"fetching the {name} archive")
        fetch(src["zip"], dest)
    got = sha256(dest)
    if src["sha256"] != "-" and got != src["sha256"]:
        raise SystemExit(f"{name} archive: sha256 {got} does not match the pin {src['sha256']}")
    if src["sha256"] == "-" and not pin:
        raise SystemExit(f"the {name} archive is not pinned: set its sha256 to {got} in SOURCES after checking it")
    return dest


def obtain(cache, spec, pin):
    name, _, path = spec.partition(":")
    if name not in SOURCES or not path:
        raise SystemExit(f"{spec}: not SOURCE:PATH with a known source")
    dest = os.path.join(cache, name, path)
    if os.path.exists(dest):
        return dest
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    src = SOURCES[name]
    if "base" in src:
        print(f"fetching {spec}")
        fetch(src["base"] + urllib.parse.quote(path), dest)
    else:
        with zipfile.ZipFile(archive(cache, name, pin)) as z:
            with z.open(path) as member, open(dest + ".part", "wb") as out:
                while True:
                    block = member.read(1 << 20)
                    if not block:
                        break
                    out.write(block)
        os.replace(dest + ".part", dest)
    return dest


def main():
    args = sys.argv[1:]
    pin = "--pin" in args
    args = [a for a in args if a != "--pin"]
    cache = args[0] if args else os.path.expanduser("~/.cache/schrodr/references")
    os.makedirs(cache, exist_ok=True)
    lines = open(MANIFEST).read().splitlines()
    hashes = {}
    for line in lines:
        if not line or line.startswith("#"):
            continue
        cols = line.split("\t")
        spec, want = cols[4], cols[5]
        if spec in hashes:
            continue
        dest = obtain(cache, spec, pin)
        got = sha256(dest)
        if want != "-" and got != want:
            raise SystemExit(f"{spec}: sha256 {got} does not match the pin {want}")
        if want == "-" and not pin:
            raise SystemExit(f"{spec} is not pinned; run with --pin after checking it")
        if dest.endswith(".flac") and not os.path.exists(dest + ".wav"):
            subprocess.run(["flac", "-d", "-s", "-f", "-o", dest + ".wav", dest], check=True)
        if dest.endswith((".ogg", ".mp3")) and not os.path.exists(dest + ".wav"):
            subprocess.run(["ffmpeg", "-v", "error", "-y", "-i", dest, "-c:a", "pcm_s24le", dest + ".wav"], check=True)
        hashes[spec] = got
    if pin:
        out = []
        for line in lines:
            if line and not line.startswith("#"):
                cols = line.split("\t")
                cols[5] = hashes[cols[4]]
                line = "\t".join(cols)
            out.append(line)
        open(MANIFEST, "w").write("\n".join(out) + "\n")
    print(f"{len(hashes)} recording(s) in {cache}")


if __name__ == "__main__":
    main()
