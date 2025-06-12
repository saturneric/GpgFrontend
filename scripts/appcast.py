import json
import datetime
from xml.sax.saxutils import escape

GITHUB_DOWNLOAD_URL = "https://github.com/saturneric/GpgFrontend/releases/download/"
BKTUS_DOWNLOAD_URL = "https://ftp.bktus.com/GpgFrontend/"
VALID_EXT = (".dmg", ".zip", ".msi", ".AppImage", ".exe")

with open("build/releases.json", "r", encoding="utf-8") as f:
    releases = json.load(f)

xml = ['<?xml version="1.0" encoding="utf-8"?>']
xml.append(
    '<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">'
)
xml.append("<channel>")
xml.append("  <title>GpgFrontend Updates</title>")
xml.append("  <link>https://github.com/saturneric/GpgFrontend/releases</link>")
xml.append(
    "  <description>GpgFrontend Release Feed (auto-generated from GitHub)</description>"
)
xml.append("  <language>en</language>")

index = 0
for rel in releases:
    if not rel.get("assets"):
        continue

    if rel.get("prerelease") == True:
        continue

    if index > 3:
        continue

    title = rel.get("name") or rel.get("tag_name", "")
    version = rel.get("tag_name", "")
    pub_date = rel.get("published_at", "")[:19]
    if not pub_date:
        continue
    dt = datetime.datetime.strptime(pub_date, "%Y-%m-%dT%H:%M:%S")
    pub_date_rss = dt.strftime("%a, %d %b %Y %H:%M:%S +0000")
    changelog = rel.get("body", "").replace("\r\n", "\n").replace("\r", "\n")
    relnotes_url = rel.get("html_url", "")

    assets = [
        a
        for a in rel["assets"]
        if any(a["name"].endswith(x) for x in VALID_EXT)
        and not a["name"].endswith(".sig")
    ]

    if not assets:
        continue

    xml.append("  <item>")
    xml.append(f"    <title>{escape(title)}</title>")
    xml.append(f"    <pubDate>{pub_date_rss}</pubDate>")
    xml.append(f"    <description><![CDATA[\n{changelog}\n]]></description>")
    if relnotes_url:
        xml.append(
            f"    <sparkle:releaseNotesLink>{escape(relnotes_url)}</sparkle:releaseNotesLink>"
        )

    for asset in assets:
        url = asset["browser_download_url"]
        size = asset.get("size", 0)
        plat = asset["name"]
        xml.append(
            f'    <enclosure url="{escape(url.replace(GITHUB_DOWNLOAD_URL, BKTUS_DOWNLOAD_URL))}" sparkle:version="{escape(version.lstrip("vV"))}" length="{size}" '
            f'type="application/octet-stream" sparkle:shortVersionString="{escape(version.lstrip("vV"))}" '
            f'sparkle:platform="{escape(plat)}"/>'
        )
    xml.append("  </item>")

    index += 1

xml.append("</channel>")
xml.append("</rss>")

with open("build/appcast.xml", "w", encoding="utf-8") as f:
    f.write("\n".join(xml))

print("appcast.xml generated!")
