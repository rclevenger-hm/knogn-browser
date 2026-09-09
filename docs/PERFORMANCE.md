# Knogn Performance

Knogn's performance goal is measurable: it should match or beat mainstream browsers on common interactive workloads while using less unnecessary background work. Until repeatable measurements support that claim, Knogn should be described as performance-focused rather than categorically faster.

## What to measure

A single benchmark is not enough. Track these separately:

1. **Raw network throughput** — download/upload Mbps and latency on the same connection.
2. **Cold page load** — first navigation after a fresh browser launch/profile.
3. **Warm page load** — repeat navigation with cache populated.
4. **Back/forward navigation** — history navigation where BFCache can avoid a full reload.
5. **JavaScript responsiveness** — Speedometer-class interactive workloads.
6. **Rendering** — animation/frame consistency and WebGL/canvas workloads.
7. **Startup** — process launch to first usable window and first page ready.
8. **Memory** — one tab, ten tabs, and background-tab idle/discard behavior.
9. **CPU at idle** — foreground idle and background-tab idle.

## Fair comparison protocol

For Chrome comparisons:

- use the same machine, power mode, network, DNS resolver, VPN/proxy state and test endpoint;
- compare Knogn private mode with Chrome Incognito when testing ephemeral profiles;
- disable third-party extensions in both browsers unless extension overhead is the subject of the test;
- alternate browser order between runs rather than completing all runs in one browser first;
- run at least five samples per browser and compare the median, not the best result;
- record browser/engine versions;
- for network throughput, reject runs affected by obvious network congestion or server throttling;
- distinguish Mbps from page-load milliseconds. A throughput difference does not automatically imply equivalent differences in browsing responsiveness.

## 0.1.1 performance pass

The first measured feedback on 0.1.0 showed Chrome Incognito ahead in raw network throughput on the same system. 0.1.1 therefore removes Knogn's broad Chromium `--disable-background-networking` switch and replaces it with targeted suppression of browser-owned services. It also enables Qt WebEngine's back/forward cache, which Qt leaves disabled by default.

Privacy controls retained in 0.1.1 include:

- third-party cookie/state filtering;
- disabled push service;
- disabled browser sync, component updates, domain reliability and crash reporting;
- disabled hyperlink auditing;
- restricted WebRTC interface exposure;
- DNT and GPC request signals;
- disabled speculative DNS prefetch.

## Performance contract

`tests/performance_contract.py` guards against several self-inflicted regressions:

- broad background-networking suppression;
- disabled GPU acceleration;
- forced software compositing;
- forced single-process Chromium mode;
- disabled BFCache.

These are guardrails, not performance proof. Real benchmark results still decide whether Knogn is faster.
