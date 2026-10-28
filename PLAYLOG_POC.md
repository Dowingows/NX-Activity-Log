# PlayLog manual send POC

PlayLog Switch adds a manual Nintendo Switch history send to PlayLog. It does not monitor or send game history in the background. The user must open the app and choose **Enviar sessões concluídas ao PlayLog** in Settings each time.

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

The default is `https://playlog.domlu.house`. On the SD card, edit `/config/PlayLog-Switch/config.ini` and set:

```ini
[playlog]
base_url=https://playlog.domlu.house
```

Use only the base URL; the app adds `/api/sync/sessions`. Restart PlayLog Switch after changing the value. The PlayLog API currently accepts these sync requests without authentication, so the endpoint should remain private to the intended network while that API policy is in place.

## Connection check and payload/response log

Settings includes **Checar conexão com PlayLog**, which requests `GET /api/health` and saves its result to `/switch/PlayLog-Switch/playlog-connection-debug.json` on the SD card. After sending, the app shows only a short summary; request payloads, HTTP statuses, response bodies, and transport errors are written to `/switch/PlayLog-Switch/playlog-send-debug.json`. These files contain game titles and play history; remove them after debugging if you do not want to keep the copies.

Requests use HTTPS certificate verification. A network/TLS failure is recorded as a failed transport with its payload retained in the debug file.

## Build status

Build with `make` from the repository root on a devkitPro environment. The `.nro` is copied to `sdcard/switch/PlayLog-Switch/PlayLog-Switch.nro`.
