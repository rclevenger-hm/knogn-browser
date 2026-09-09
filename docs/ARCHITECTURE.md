# Architecture

## 1. Native shell

Qt Widgets owns windows, tabs, omnibox, menus, downloads, permissions and extension install UX. Browser chrome does not run in a JavaScript UI runtime.

## 2. Browser services

Bookmarks, history, sessions, settings, permissions and downloads will be local services with explicit state ownership. SQLite or small native stores are preferred over background daemons.

## 3. Privacy policy

`PrivacyProfile` configures profile persistence, third-party state filtering, push behavior, WebRTC exposure, speculative networking and extension boundaries. `PrivacyInterceptor` applies request-level privacy preference headers.

Privacy policy must remain testable independently of visual UI behavior.

## 4. Performance/lifecycle policy

Knogn uses Qt WebEngine's page lifecycle support. Background tabs are moved toward the engine's safe `recommendedState`; the engine may freeze or discard pages when safe. A discarded renderer consumes virtually no CPU/memory until the page is revisited and reloaded.

This is the beginning of a broader tab-resource scheduler rather than the final algorithm.

## 5. Engine adapter direction

Milestone 0.1 talks to Qt WebEngine directly in a small number of classes. As browser services grow, engine operations should converge behind narrow interfaces for:

- page creation/navigation;
- lifecycle state;
- permissions;
- downloads;
- extensions;
- request policy;
- storage/profile management.

That boundary will allow evaluation of Servo or other engines without rewriting the browser shell.

## Performance strategy

- native UI, no Electron runtime;
- suppress browser-owned background services not needed by Knogn;
- reduce speculative network work;
- freeze/discard safe background pages;
- keep optional features lazy;
- benchmark cold/warm startup, idle RSS, 1/10/50-tab RSS, CPU, page responsiveness and battery impact;
- refuse benchmark wins that require disabling sandboxing or other security boundaries.

## Platform strategy

Desktop v1 targets Windows, Linux and macOS. Android will require a mobile shell and platform-specific engine decisions. iOS must be evaluated independently because engine/distribution rules differ by region and channel.
