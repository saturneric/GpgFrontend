# GpgFrontend Privacy Policy

Effective date: 2026-11-20

## 1. About This Policy

GpgFrontend is free and open-source software, licensed under the GNU General
Public License version 3 or later. It is developed by volunteers, not by a
company. Its source code is public at
<https://github.com/saturneric/GpgFrontend>, so every statement in this policy
can be checked against it.

This policy explains what personal data is processed when you use
GpgFrontend and gives the information required by Art. 13 of the EU General
Data Protection Regulation (GDPR). It covers the application, its bundled
modules, the update servers operated for it, the project website at
<https://gpgfrontend.bktus.com/>, and contact with the maintainer.
Services you choose to use with GpgFrontend, and the sites you download it
from, have their own policies (see Section 7). Modules from other publishers
that you choose to install are not part of GpgFrontend and are not covered by
this policy.

## 2. Controller

The controller for the processing described in Sections 4.1, 5, and 6.1 is
the GpgFrontend maintainer:

    Name:    Yu Hu ("saturneric")
    E-mail:  privacy@bktus.com
    Address: see <https://gpgfrontend.bktus.com/impressum/>

For everything else GpgFrontend does, the maintainer receives no data and is
not a controller. The software runs on your device, and any connection it
makes goes directly from your device to a service you have chosen.

## 3. Summary

- GpgFrontend contains no telemetry, analytics, crash reporting, advertising,
  or tracking.
- There are no user accounts and nothing to register.
- With its default settings, GpgFrontend does not connect to any network on
  its own.
- GpgFrontend never sends your secret keys or their passphrases anywhere on
  its own. A secret key leaves your device only when you export it to a place
  you choose.
- The Android app does not request permission to access the internet.
- The project website sets one cookie, from Cloudflare's bot detection, and
  uses no analytics or tracking (see Section 6.1).
- The maintainer receives personal data in three cases only: when you check
  for updates (Section 4.1), when you visit the project website
  (Section 6.1), and when you write to the maintainer (Section 5).

## 4. Data Sent Over the Network

GpgFrontend connects to a network only in the cases below. In each case, the
server you connect to receives your IP address and the time of the request,
as with any internet connection. This section does not apply to the Android
app, which has no network access.

HTTP requests (the update check, key servers, and the proxy test) also carry
two headers. The first is a fixed, browser-style User-Agent. It is the same
for every user of a platform (Windows, macOS, or Linux and other Unix
systems); the system version and CPU type written in it are fixed values that
do not describe your computer, and it does not contain the GpgFrontend
version. The second is an Accept-Language header with the language and
region set in your operating system (for example "de-DE"), as web browsers
send it.

### 4.1 Update Check

- Servers: update.bktus.com and builds.bktus.com, operated by the
  maintainer.
- Data processed: your IP address; the time of the request; the two headers
  described above; the files requested (the signed release index, the signed
  keys that vouch for it, release descriptions, and release notes); and the
  git commit hash of the installed build, which is part of one request path.
  The commit hash shows which official build you run. The paths of the
  release descriptions and notes name the versions offered to your build,
  which shows its release series (for example 2.2) and the release channel
  it follows (stable, beta, or nightly).
- Not sent: your name, keys, e-mail addresses, files, operating system
  version, or CPU architecture. The GpgFrontend version is not sent as such,
  but it can be derived from the commit hash. Nightly AppImage updates are
  an exception for the CPU architecture (see below).
- When: in three cases. Automatically, after start-up and then about once a
  day (a check that failed is retried after an hour), only while "Check for
  updates automatically, once a day" is on in Settings > Updates. When you
  open Help > Check for Updates, only while that setting is on and no check
  has succeeded in the last day. When you press "Check now" (labelled "Check
  again" after a first check) in Help > Check for Updates, whatever the
  setting.
- Default: off. The first-run wizard asks you, with the option unchecked.
  Builds from the Mac App Store and Flathub do not contain the update check;
  those stores deliver updates themselves. The Android app does not contain
  it either.
- Purpose: to tell you whether a newer release is available, to confirm
  that your build is an official one, and to deliver update files. The
  release index is checked against a digital signature, and every other
  update file against a checksum listed in that signed index, before it is
  used.
- Legal basis: Art. 6(1)(f) GDPR. The legitimate interest is providing users
  with authentic and secure updates. The check runs only at your request or
  after you turned it on. The commit hash is read from your device only for
  this check. This is strictly necessary to provide a service you have
  explicitly requested (§ 25(2) no. 2 TDDDG).
- Recipient: Cloudflare, Inc., as hosting provider (see Section 6.2).
- Retention: the maintainer stores no request logs. Cloudflare shows the
  maintainer aggregated statistics, such as request counts, and, for up to
  31 days, a sample of individual requests that includes IP addresses
  (<https://developers.cloudflare.com/waf/analytics/security-analytics/>).
  The maintainer does not use this data to identify users. For how long
  Cloudflare itself keeps request data, see Section 6.2.

If you use the in-place update for nightly Linux AppImage builds, GpgFrontend
also downloads the AppImage file from builds.bktus.com. The name of that file
shows the version being installed, the kind of build (for example portable),
your platform, and your CPU architecture. The same purpose, legal basis,
recipient, and retention apply.

### 4.2 Key Servers

- Servers: the key servers you configure. By default, keys.openpgp.org and
  keyserver.ubuntu.com.
- Looking up or refreshing a key: sends its key ID or fingerprint.
- Searching: sends the text you enter, such as an e-mail address.
- Publishing: sends your complete public key, including the names and e-mail
  addresses in it.
- Adding or testing a server: sends two test requests, one for each key
  server protocol, with no personal data.
- When: only when you start one of these actions. There is no background
  synchronization.

Keys published to a key server are generally public. On servers using the
HKP protocol, such as keyserver.ubuntu.com, published keys cannot be deleted.
GpgFrontend warns you before such an upload. keys.openpgp.org publishes the
names and e-mail addresses in a key only after each address has been
confirmed by e-mail.

If you publish a key that contains another person's name or e-mail address,
you are responsible for having the right to do so.

### 4.3 E-Mail

- Servers: only the IMAP and SMTP servers of the accounts you add.
- Data sent: your login credentials; the messages you send, including all
  recipients; a copy of each sent message to your Sent folder; search terms
  you enter; and your computer's host name in the SMTP greeting, as is usual
  for mail programs.
- Account setup: for providers not built into GpgFrontend, the domain of
  your e-mail address, and the usual server names under it (such as
  imap.example.org), are looked up in DNS to find its servers. You are told
  before this happens.
- When: only while you test an account, open a mailbox, refresh it, or send
  mail. There is no background polling.

Encrypted connections (TLS) are required for every mail server except one on
your own computer.

### 4.4 Network Use by GnuPG

In builds that use the GnuPG engine, the option "Fetch missing keys
automatically" lets GnuPG download a signer's public key while verifying a
signature. GnuPG looks the key up by key ID or fingerprint on the key server
set in your GnuPG configuration, and may also ask the web server of the
signer's e-mail domain (Web Key Directory). This option is off by default and
is not shown in the settings window.

On the smart card page, the "Fetch" button lets GnuPG download the public key
of the inserted card from the address stored on the card, or from a key
server. This happens only when you press it.

The option "Forbid all GnuPG network connection" (Settings > GnuPG) stops
GnuPG from using the network for the operations GpgFrontend asks it to do.
Network options in your own GnuPG configuration files are left as they are.

### 4.5 Proxy

When you test a proxy in Settings > Network, GpgFrontend sends one request,
through the proxy, to a web address you enter, and reports only whether it was
reachable. If that server redirects the request, the redirect is followed.

The proxy you set is used for the update check and for key servers. E-mail
and GnuPG connect directly.

## 5. Contacting the Maintainer

If you contact the maintainer by e-mail or post, the maintainer processes
your e-mail address, your name and postal address if given, and the content
of your message.

- Purpose: to answer your request.
- Legal basis: Art. 6(1)(f) GDPR (legitimate interest in answering
  inquiries), or Art. 6(1)(c) GDPR where your request concerns your data
  protection rights.
- Recipient: the maintainer's e-mail provider, mailbox.org (Heinlein
  Hosting GmbH, Berlin, Germany), which stores the mail on servers in
  Germany as a processor under Art. 28 GDPR. Postal mail is received,
  opened, and scanned on the maintainer's behalf by Postflex GmbH (Greven,
  Germany) under a data processing agreement pursuant to Art. 28 GDPR.
- Retention: deleted once your request is fully settled, unless a legal
  obligation requires longer storage. Correspondence about data protection
  rights is kept for up to three years after it is settled, so that the
  maintainer can show the request was handled properly (Art. 5(2) and
  Art. 6(1)(f) GDPR).

Issues and comments on GitHub are public and are processed by GitHub, Inc.
under its own privacy statement. Please do not post personal data in public
issues.

## 6. Project Website, Hosting, and International Transfers

### 6.1 Project Website

- Site: gpgfrontend.bktus.com, a static site with the manual, this policy,
  and the legal notice. Its images are loaded from img.bktus.com, and its
  downloads page loads the release list from the update servers
  (Section 4.1). The maintainer operates these hosts too.
- Data processed: your IP address; the time of the request; the page
  requested; and the headers your browser sends, such as User-Agent,
  language, and referrer.
- Cookie: Cloudflare's bot detection adds a small script to a page when
  your browser does not yet hold its cookie, and then sets one cookie,
  cf_clearance. According to Cloudflare, the script checks signals in your
  browser; it sends the result to Cloudflare in a request to the site,
  under /cdn-cgi/challenge-platform/. The cookie serves to tell browsers
  from automated traffic, to protect the site against abuse. It is valid
  for all of .bktus.com, so your browser also sends it to the maintainer's
  other hosts under that domain, and it expires one year after it is set.
  Cloudflare sets and reads it; the maintainer does not read it and does
  not use it to identify or track visitors. You can delete or block the
  cookie in your browser; the site still works, and while the cookie is
  blocked the check runs again on each page. The same cookie is set when
  Cloudflare's attack protection challenges a request. See
  <https://developers.cloudflare.com/fundamentals/reference/policies-compliances/cloudflare-cookies/>.
- What the site does not do: no analytics or tracking, no advertising, no
  accounts, no forms that send data (the search runs in your browser, on an
  index loaded from the site), no other cookies, and no content loaded from
  third-party hosts.
- Purpose: to deliver the pages, to keep the site available and secure, and
  to protect it against automated abuse.
- Legal basis: Art. 6(1)(f) GDPR. The legitimate interest is providing
  information about the software and operating the site securely. The
  script and the cookie are stored on and read from your device because
  this is strictly necessary to provide the site securely
  (§ 25(2) no. 2 TDDDG).
- Recipient: Cloudflare, Inc., as hosting provider (see Section 6.2).
- Retention: the same as for the update check (see Section 4.1).

### 6.2 Hosting and International Transfers

The update servers, the project website, and its image host (img.bktus.com)
are hosted with Cloudflare, Inc. (101 Townsend St., San Francisco, CA 94107,
USA). Cloudflare processes request data, such as IP addresses, on behalf of
the maintainer to deliver the files and to protect the service against abuse,
under a data processing agreement pursuant to Art. 28 GDPR. Cloudflare states
no fixed period for this data: it keeps personal data for as long as the
purposes named in its privacy policy, or legal obligations, require (Section
11 of the policy linked below). The sampled request records it shows the
maintainer are kept for up to 31 days
(<https://developers.cloudflare.com/waf/analytics/security-analytics/>).

Because Cloudflare is a US company and operates a global network, this data
may be processed outside the European Economic Area. Such transfers are based
on the EU-US Data Privacy Framework, under which Cloudflare is certified
(adequacy decision, Art. 45 GDPR), and on the standard contractual clauses in
that agreement (Art. 46(2)(c) GDPR). See
<https://www.cloudflare.com/privacypolicy/>. The data processing agreement,
including the standard contractual clauses, is available at
<https://www.cloudflare.com/dpa/>.

## 7. Third-Party Services

When you use key servers, mail servers, DNS resolvers, a proxy, or GnuPG's
own network functions, your device communicates directly with those services.
The maintainer does not receive this data. Those services are independent
controllers, and their own privacy policies apply.

The same is true of the app store, download site, or Linux distribution you
obtain GpgFrontend from: it processes data about your download under its own
privacy policy and does not pass personal data about you to the maintainer.

Links, such as those to the project website or GitHub, open in your web
browser only when you click them.

## 8. Data Stored on Your Device

GpgFrontend stores its data only on your device: in your profile folder, in
your GnuPG home folder, in your system's settings and credential store, and in
temporary folders. None of it is sent to the maintainer, and it stays on your
device until you delete it.

- Settings you choose, stored without encryption. They include your proxy
  user name and password, if set; the details of your e-mail accounts (name,
  address, server names, and user name, but not the password); and your key
  server list.
- Keys. Secret keys are protected by their passphrase, if you set one. Key
  files can be read only by your user account.
- Caches, unsaved editor text kept for recovery, and saved e-mail passwords,
  all stored encrypted. On desktop systems, the key for this encryption is
  kept in the profile folder by default, so it protects no more than the
  profile folder itself. You can protect it with your system's credential
  store or a PIN in the settings. On Android, it is protected by the system
  keystore.
- Notes you write are saved where you choose, encrypted or not, as you
  choose. While an encrypted note is open, an encrypted working copy of it is
  kept in the application's data folder and deleted when you close the note.
- Temporary files. E-mail attachments you open are saved, decrypted if they
  were encrypted, to a temporary folder so that another program can open
  them.
- Logs. A log file is kept in the profile folder, up to about 30 MB, with the
  oldest entries removed first. By default only critical errors are logged.
  More detailed log levels may record file paths, key IDs, user IDs, and
  GnuPG output. On Android, log lines also go to the system log. Logs are
  never sent automatically. Please review a log before attaching it to a bug
  report.

## 9. Your Rights

Under the GDPR, you have the right to:

- access the personal data concerning you (Art. 15);
- have inaccurate data corrected (Art. 16);
- have your data deleted (Art. 17);
- restrict its processing (Art. 18);
- receive your data in a portable format, where this right applies
  (Art. 20);
- lodge a complaint with a supervisory authority (Art. 77), in particular
  in the EU member state of your habitual residence, your place of work, or
  the place of the alleged infringement.

Right to object (Art. 21 GDPR): where processing is based on Art. 6(1)(f)
GDPR, you have the right to object to it at any time on grounds relating to
your particular situation. For the update check, you can also simply turn off
"Check for updates automatically, once a day" in Settings > Updates and not
press "Check now", "Check again", "Download", or "Install update" in Help >
Check for Updates; then no update request is made at all.

To exercise these rights, contact the maintainer (Section 2). Because the
maintainer keeps no personal data about users of the application, in most
cases there is no stored data to provide or delete.

You are not required by law or by contract to provide any personal data.
GpgFrontend works fully offline; without the network features in Section 4
you only miss update notices and the services those features connect to.

No automated decision-making, including profiling (Art. 22 GDPR), takes
place.

## 10. Changes to This Policy

If GpgFrontend's data processing changes, this policy will be updated before
or together with the release that introduces the change. The effective date
at the top will change, and the full history of this policy is public in the
project's git repository.
