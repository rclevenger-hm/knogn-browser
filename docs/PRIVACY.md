# Knogn Privacy Contract

This is a product contract, not marketing copy. Changes that violate these statements are regressions unless the contract is deliberately revised and reviewed.

## Browser-owned network traffic

A clean Knogn startup must not transmit browsing history, tab state, typed URLs, search queries, stable identifiers, crash reports, feature usage, device fingerprints, or engagement data to a Knogn-operated service.

Milestone 0.1 configures Chromium to disable background networking, component updating, domain reliability, sync and Breakpad crash reporting. Qt WebEngine push messaging is disabled; Qt documents that its push service uses Firebase Cloud Messaging.

Future security-data updates may require network access. If introduced, they must be authenticated, inspectable, narrowly scoped, disableable, and must not carry browsing history or stable user identifiers.

## Website state

Third-party state is denied by default. Qt WebEngine's cookie filter also controls cookie-like tracking surfaces including IndexedDB, DOM storage, filesystem API, service workers and AppCache.

Normal profiles may persist first-party persistent cookies. Session permissions are stored in memory. Private profiles are off-the-record and keep normally persistent web state in memory only.

## Speculation

DNS prefetch and hyperlink auditing are disabled. Features that preconnect, prefetch or speculate must be reviewed as both performance and privacy changes.

## WebRTC

Public interfaces only are exposed by default to reduce local-network address disclosure.

## Preference signals

Knogn sends `DNT: 1` and `Sec-GPC: 1` on HTTP(S) requests. These are preference signals. Websites may ignore them.

## Search

The initial default search provider is DuckDuckGo. Search is still an external network action and the provider can observe a query. A later milestone will make search engines configurable and support local/address-only workflows without a mandatory provider.

## Extensions

Extensions are third-party code and may observe browsing activity according to the permissions the user grants. Knogn does not describe an extension-equipped profile as equivalent to an extension-free private window.

## Verification targets

Milestone 0.1 has source-level privacy contract tests. The next network-verification milestone must run Knogn through an intercepting test proxy and prove that a clean startup produces no unsolicited external browser-owned requests.
