# Security Policy for DCSS

Last updated Sep 2026.

## Supported Versions

Currently, this repository only supports the **0.34.x** release series:

| Version | Supported          |
| ------- | ------------------ |
| 0.34.x  | :white_check_mark: |
| <0.34   | :x:                |
| >0.34   | :x:                |

We may, if the vulnerability is severe and affects online play, attempt to
patch earlier 0.34.x point releases. Other release series are unsupported at this time.

Online servers generally run a webtiles server version drawn from trunk, even
if they allow play on older versions of dcss, so any vulnerability in an
up-to-date webtiles server is covered (e.g. in the python code).

## Reporting a Vulnerability

Open an issue on github in this repository, or contact the devteam in
`#crawl-dev` on Libera IRC. If you would prefer to report the issue in private,
we recommend either contacting one of the currently active devs directly (e.g.
via an IRC private message), or sending an email to [security@dcss.io] with a
subject line including the phrase `dcss security report`. Currently this email
forwards to @rawlins / advil, who will send an acknowledgement and report the
issue to the devteam more generally.

If you have access (devteam and server owners) you can directly report a
security issue in private by opening an issue in the https://github.com/crawl/dcss-security
repository.
