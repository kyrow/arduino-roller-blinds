Import("env")
from pathlib import Path

ENV_FILE = Path(env["PROJECT_DIR"]) / ".env"
EXPORTED = ("WIFI_SSID", "WIFI_PASS", "HOSTNAME")

if ENV_FILE.exists():
    for raw in ENV_FILE.read_text().splitlines():
        line = raw.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip().strip('"').strip("'")
        if key in EXPORTED:
            env.Append(CPPDEFINES=[(key, env.StringifyMacro(value))])
            print(f"load_env: injected {key}")
else:
    print(f"load_env: {ENV_FILE} not found, skipping")
