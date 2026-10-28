# PlayLog manual send POC

This fork adds a manual Nintendo Switch history send to PlayLog. It does not monitor or send game history in the background. The user must open NX Activity Log and choose **Enviar sessões concluídas ao PlayLog** in Settings each time.

## What is sent

The app sends each completed PlaySession as one `POST /api/sync/sessions` request. The request uses the PlayLog session-event contract:

- `platform`: `nintendo_switch`
- `system`: `Switch`
- `game_title`: title reported by the Switch
- `started_at`, `ended_at`: UTC ISO-8601 timestamps
- `duration_seconds`: session playtime reported by NX Activity Log
- `status`: `ended`
- `device_id` and `session_id`: stable identifiers for retry-safe upserts

Open/incomplete sessions are skipped. Re-sending the history is safe: PlayLog upserts a session by `device_id` and `session_id`, and identifiers include the Switch account, title ID, and session start/end timestamps. The Switch accounts are sent to the same PlayLog account because this API currently has no account authentication or per-profile mapping.

## Endpoint configuration

The default is `https://playlog.domlu.house`. On the SD card, edit `/config/NX-Activity-Log/config.ini` and set:

```ini
[playlog]
base_url=https://playlog.domlu.house
```

Use only the base URL; the app adds `/api/sync/sessions`. Restart NX Activity Log after changing the value. The PlayLog API currently accepts these sync requests without authentication, so the endpoint should remain private to the intended network while that API policy is in place.

## Payload and response debug

After a manual send finishes, the app displays a summary and the request payloads with each HTTP status and response body. The full log is also written to `/switch/NX-Activity-Log/playlog-send-debug.json` on the SD card. This file contains game titles and play history; remove it after debugging if you do not want to keep the copy.

Requests use HTTPS certificate verification. A network/TLS failure is recorded as a failed transport with its payload retained in the debug file.

## Build status

Build with the project Makefile from the repository root on a devkitPro environment with the Switch toolchain and submodules initialized. The current Mac checkout did not have `DEVKITPRO` or the Switch compiler available, and the submodule working directories were empty, so this POC has not yet been compiled to an `.nro` or exercised on the console.
