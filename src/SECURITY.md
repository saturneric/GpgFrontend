# Security Policy

## Supported Versions

The application white check mark is still in support, without it means there
version will no longer have a update including security fixture. That's beacuse
currently I have a job and a hard learning plain, which means I don't have too
much time to dealing with this project now, sincerely, I am sorry for that.
Maybe this situation will get better in the future. So it is recommand to use
the latest version.

| Version | Supported          |
| ------- | ------------------ |
| 1.x.x   | :x:                |
| 2.x.x   | :x:                |
| 2.1.x   | :white_check_mark: |

## Platform & Library Support

According to security concern and my limiting time to contributing, GpgFrontend
will gradually drop the support of some old platforms. 

Starting from v1.0.0, x32 operating system is no longer support. Mainly because
I have limit time and the fact that x32 machime is getting less and less in the
pc market, so I plan to stop spending tons of time dealing with lots of complex
problem which might made me crazy. x64 is just enough, or you can use gpg4usb.

Starting from v2.0.1, Ubuntu 16.04 is no longer support. Mainly because this
Ubuntu 16.04 LTS's life time had come into an end at during that time. Keep
using this version of Ubuntu is not safty anymore.

Starting from v2.1.0, macOS 10.15 is no longer support. Mainly because this
system will no longer support by github action in the near future. Also, I think
this version is old enough to drop it out, and Apple might not longer to support
it soon.

## Reporting a Vulnerability

If you want to report a vulnerability, it's not good to raise an issue in
public. You should send an email to eric@bktus.com to contract me. In the email,
You can describe the vulnerability you have just discovered and request a
hot-fix. Or you can just provide a git patch to fix it and send it to me using
the email. This is a better way to solve the vulnerability as quick as possible.