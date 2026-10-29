# Tracking providers

Research notes for Step 6, the tracking provider list: each vendor's Unreal integration, how to detect
it, and what it supplies.

> **Status: none of this is verified yet.** These notes were gathered 2026-10-29 from web search.
> Fab and some vendor pages couldn't be opened from the research session, so most entries come from
> search-result excerpts of the cited page, not from reading the page itself. Treat every plugin name
> and port as a lead. Before a provider uses a name or port, confirm it against the installed plugin on
> the PC (its `.uplugin` file and its Live Link source settings) or the vendor's own docs, then mark the row
> **Verified** with where it came from.
>
> Never invent a default port. If no vendor source states one, the provider leaves the field empty and
> asks the user to enter it.

## Detection summary

| Provider | Detect by (plugin name) | Delivers via | Lens data | Default port | Status |
|---|---|---|---|---|---|
| FreeD (built-in) | `LiveLinkFreeD` (engine) | Live Link FreeD source, UDP | Focus, focal length, user-defined (usually iris) encoders | **40000**, IP `0.0.0.0` when the camera sends ([Epic docs][freed]) | Port from Epic docs; plugin name to confirm in the install |
| Live Link XR (built-in) | to confirm in the install | Live Link XR source (OpenXR trackers) | No | n/a | Unverified |
| Live Link VRPN (built-in) | to confirm in the install | Live Link VRPN source | No | not found | Unverified |
| Live Link Hub (built-in) | to confirm in the install | Hub app streaming to editor sessions ([Epic docs][hub]) | Whatever Hub forwards | not found | 5.8 "production-ready" per [Epic forum post][ue58forum]; what the toolkit should configure is to be determined on 5.8 |
| Mo-Sys StarTracker | `MoSysVPPro` folder (paid VP Pro); Fab "Mo-Sys LiveLink" plugin name not found | Own Live Link source "Mo-Sys UDP" ([VP Free PDF][mosysfree]) | not found | not found (set on the subject) | Folder name from [VP Pro manual][mosysmanual]; `.uplugin` name unconfirmed |
| ZEISS CinCraft Scenario | not found | Own Live Link sources per data stream, plus a lens subject that drives the Lens component's distortion ([docs][zeisscam]) | Yes (distortion via lens subject) | not found | Product name from [Fab listing][zeissfab] |
| stYpe | not found | FreeD (free) or paid stYpe protocol plugin; StypeLand includes lens files ([forum, not vendor][stypeforum]) | Via StypeLand | not found | Weak: no vendor page found |
| Ncam | not found (modules `NcamMedia`, `NcamProfile` mentioned, [source][ncammods]) | Live Link ([docs][ncamdocs]) | not found | not found | Unverified |
| OptiTrack | `OptitrackLiveLink` (community source only, [link][optifolder]) | Own Live Link source "OptiTrack Source" over NatNet ([docs][optidocs]) | No | not found (match Motive's Streaming pane) | Unverified |
| Vicon | not found (MSI installer) | Own source "Vicon Datastream Source" ([page][vicon]) | No | 801 default, 804 low-latency, 8802 always-live ([Vicon help][viconports]) | Ports from vendor help pages |
| HTC VIVE Mars | none: no separate plugin | Live Link over the Message Bus ("VIVE Live Link"), or FreeD ([how-to][marsll]) | FIZ via Live Link; focus-only primes need FreeD ([guide][marslens]) | 6666 is only an *example* static endpoint ([how-to][marsendpoint]) | Unverified |

## Engine version support (as found)

| Provider | 5.5 | 5.6 | 5.7 | 5.8 | Source |
|---|---|---|---|---|---|
| Mo-Sys VP Pro | ? | yes (5.6.1) | yes | yes | [product page][mosys] |
| ZEISS UE Connect | ? | yes (2.0.1) | ? | announced July 2026 | [release notes][zeissrn], [news][zeiss58] |
| Vicon Live Link | ? | yes (1.11) | yes (1.12) | ? | [page][vicon] |
| OptiTrack Live Link | yes (latest build found) | ? | ? | ? | [forum][optiforum] |
| stYpe, Ncam, VIVE Mars | ? | ? | ? | ? | not found |

## Licence and cost (for the README's "what you need" section)

- **Mo-Sys:** VP Pro is a paid 12-month subscription per camera ([reseller][mosysprice]); VP Free is free. Both need StarTracker hardware.
- **ZEISS UE Connect:** free for CinCraft users. Needs Scenario hardware.
- **stYpe:** paid, needs hardware (source is a forum post).
- **Ncam:** ARSuiteLite is free and open source; ARSuite is a subscription.
- **OptiTrack:** free plugin. Needs Motive and the cameras.
- **Vicon:** free download. Needs Vicon software.
- **VIVE Mars:** hardware kit.

## To do on the PC

1. For each installed vendor plugin, record the `.uplugin` file name, the Live Link source class name and
   its settings struct (ports, subject naming) from the installed files. Move each row to **Verified**.
2. Confirm the built-in plugin names (FreeD, XR, VRPN, Hub) in each engine install, 5.5 to 5.8.
3. On 5.8, read what Live Link Hub changes about setup and decide what the Hub provider configures.
4. Decide how detection works for vendors whose name stays unknown. Options: match on the Live Link source
   class rather than the plugin name, or a notice-only provider.

[freed]: https://dev.epicgames.com/documentation/unreal-engine/live-link-freed--in-unreal-engine
[hub]: https://dev.epicgames.com/documentation/unreal-engine/live-link-hub-in-unreal-engine
[ue58forum]: https://forums.unrealengine.com/t/unreal-engine-5-8-released/2729274
[mosys]: https://www.mo-sys.com/product/camera-tracking/virtual-production/mo-sys-vp-pro
[mosysfree]: https://software.mo-sys.com/docs/MoSysVPFree/MoSysVPFree_UE5.pdf
[mosysmanual]: https://support.mo-sys.com/media/VPPro/5.3.1.0/VPProManual
[mosysprice]: https://www.scan.co.uk/products/mo-sys-vp-pro-virtual-production-toolkit-12-month-subscription-for-1-camera
[zeissfab]: https://www.fab.com/listings/92b60c6d-b7df-4af3-8774-5735055f2368
[zeisscam]: https://docs-cincraft.zeiss.com/ue-connect/adding-tracking-data-to-a-camera
[zeissrn]: https://docs-cincraft.zeiss.com/ue-connect/release-notes
[zeiss58]: https://nofilmschool.com/ue-connect-for-cincraft-scenario-update
[stypeforum]: https://forums.unrealengine.com/t/virtual-studio-stype-red-spy/1300185
[ncamdocs]: https://docs.ncam-tech.com/arsuite
[ncammods]: https://www.ncam-tech.com/?p=857
[optidocs]: https://docs.optitrack.com/plugins/optitrack-unreal-engine-plugin/unreal-engine-optitrack-live-link-plugin
[optifolder]: https://github.com/Digital-Media/sco_2024/wiki/Recipe---Metamotion
[optiforum]: https://forums.naturalpoint.com/viewtopic.php?p=88211
[vicon]: https://www.vicon.com/software/third-party/unreal-engine/
[viconports]: https://vicon-help.atlassian.net/wiki/spaces/Unreal5Plugin18/pages/14845083/Unreal+Plugin+source+machine+settings
[marsll]: https://www.vive.com/eu/support/camtrack/category_howto/connecting-to-mars-using-the-live-link-plug-in.html
[marslens]: https://vive-mars.notion.site/Applying-Lens-File-to-CineCameraActor-0bebd239ba5d437d963df26148ad1df3
[marsendpoint]: https://www.vive.com/ca/support/camtrack/category_howto/adding-mars-as-an-endpoint-in-unreal-engine.html
