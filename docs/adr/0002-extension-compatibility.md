# ADR 0002 — Extension compatibility policy

Status: Accepted for v0.x

## Decision

Support Chrome/Chromium Manifest V3 extensions natively through Qt WebEngine in normal profiles. Treat Firefox WebExtensions as an import/compatibility problem with explicit static analysis and conversion, not as guaranteed direct execution.

## Privacy rule

Off-the-record private windows remain extension-free under the initial engine. Knogn will not bypass the engine restriction with a hidden persistent profile because doing so would undermine the private-window storage contract.
