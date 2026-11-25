# Release notes

One file per tag, in [docs/releases](releases). The Release workflow publishes
`docs/releases/<tag>.md` as that tag's release description; if the file is not
there, it falls back to a generic description and says so in the log.

| Tag | Date | What it is |
|---|---|---|
| [v0.0.3](releases/v0.0.3.md) | next | eight palettes, the accent picker gone, the rules gone, README split, screenshots per theme |
| [v0.0.2](releases/v0.0.2.md) | 24 Nov 2026 | progressive JPEG covers, centred Now Playing readout, the release pipeline, CJK fonts no longer shipped |
| [v0.0.1](releases/v0.0.1.md) | 23 Nov 2026 | the first packaged build of this tree |
| [v1.1](releases/v1.1.md) | 27 Oct 2026 | the last release of the original H2 port this fork came from |
| [v1.0](releases/v1.0.md) | 24 Oct 2026 | the first packaged build of that port |

## Making a release

Write the notes **before** the tag, because the workflow checks out the tag
itself:

```bash
cp docs/releases/v0.0.3.md docs/releases/v0.0.4.md   # and fill it in
git commit -m "release: v0.0.4 notes"
git tag v0.0.4 && git push origin v0.0.4
```

Pushing the tag runs `.github/workflows/release.yml`: it builds `--type=n`,
renames `rockbox.zip` to `rockbox-h2-v0.0.4.zip`, creates the release if the tag
has none (title "H2 Rockbox v0.0.4", the notes file as the description), uploads
the archive, and prints its sha256. A release you made by hand keeps its own
description — the workflow only rewrites a body that is empty or that it wrote
itself. A tag with a `-` (e.g. `v0.0.4-rc1`) is published as a prerelease.
