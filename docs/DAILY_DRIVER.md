# Daily-driver milestone

Knogn 0.2.0 is the first milestone aimed at ordinary day-to-day browser use rather than a minimal browser shell.

## Implemented in 0.2.0

### Settings

Knogn now has persistent settings for:

- search provider, including a custom search URL;
- home page;
- download folder and whether to prompt for every download;
- third-party site-state blocking;
- DNT and Global Privacy Control preference headers;
- local spell checking;
- password manager and auto-fill behavior;
- system routing, direct routing, HTTP proxy/tunnel, and SOCKS5 proxy/tunnel configuration.

Privacy policy and request-interceptor settings are cached when a browser profile is created so they do not add a settings lookup to each network request.

## Bookmarks / favorites

Bookmarks are stored locally under Knogn's application-data directory. The browser provides:

- `Ctrl+D` / star button to save the current page;
- a bookmarks menu;
- a bookmarks toolbar;
- a bookmark manager with open/remove controls;
- import from Netscape bookmark HTML (the export format used by Chrome, Edge, Firefox and others);
- import from Chromium-family `Bookmarks` JSON files.

## Passwords and logins

Knogn never stores password text in its normal settings or bookmark files. Password secrets go through QtKeychain with insecure/plaintext fallback explicitly disabled. Depending on the OS, this maps to Windows Credential Store, macOS Keychain, or an available Linux secret-service/keyring backend.

Current 0.2.0 behavior provides:

- manual save of a login for the current site;
- origin-matched saved-login lookup;
- username/password form auto-fill;
- saved-login manager that shows metadata but not password text;
- browser password CSV import for exports containing URL, username and password columns;
- no saving of new credentials from private windows.

Automatic post-submit password capture and passkeys/WebAuthn credential management remain later password-manager work.

## VPN and network routing

An active operating-system VPN already carries Knogn traffic because WebEngine uses the host network route. The Knogn settings page therefore distinguishes:

- **System routing / VPN** — use the OS route, including a configured WireGuard/OpenVPN/provider VPN;
- **Direct** — bypass configured proxies;
- **HTTP proxy / tunnel** — route only Knogn through the specified HTTP endpoint;
- **SOCKS5 proxy / tunnel** — route only Knogn through the specified SOCKS endpoint.

On Windows the settings UI also opens the native Windows VPN configuration screen. A true bundled VPN provider is intentionally not represented as a proxy: provider-integrated WireGuard/OpenVPN support requires a privileged tunnel service, lifecycle management and provider credentials and should be implemented as its own subsystem.

## Windows application identity

The Windows build now embeds the Knogn `.ico` resource into `Knogn.exe`, supplies the same icon to NSIS and Installed Apps metadata, and exposes Knogn as a desktop-shortcut option during installation.

## Media compatibility remains an acceptance blocker

0.2.0 does **not** claim full Chrome/Firefox media parity. The browser-side HTML5/MSE/fullscreen/hardware-acceleration work is present, but H.264/AAC and DRM/Widevine support still depends on the WebEngine distribution. `knogn://media` remains the runtime source of truth, and issue #7 remains open until released packages demonstrate the expected media matrix.
