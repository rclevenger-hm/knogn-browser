# ADR 0001 — Qt WebEngine as the initial compatibility engine

Status: Accepted for v0.x

## Decision

Use Qt 6.11+ WebEngine/Chromium as Knogn's first rendering engine while keeping product architecture separable from the renderer.

## Why

- mature compatibility with the modern web;
- native C++/Qt embedding rather than an Electron application shell;
- Windows/Linux/macOS support;
- Chromium process sandboxing;
- profile, request, cookie/storage and lifecycle controls;
- Manifest V3 Chrome extension support;
- a feasible path to a usable browser without maintaining an entire rendering engine fork.

## Tradeoffs

- Knogn inherits Chromium's engine size and update cadence;
- some Chromium behavior is controlled by flags rather than first-class Qt APIs;
- private/off-the-record profiles cannot load user extensions;
- Firefox extension compatibility requires translation, not direct Gecko execution;
- a Chromium-derived engine places a ceiling on how small the final binary can become.

## Future

Servo or another engine may be evaluated behind an adapter once the browser-service and privacy contracts are mature enough to run against multiple engines.
