Import("env")

import atexit
import os
import shutil
import json
from datetime import datetime


def copy_firmware_to_release(source, target, env):
    build_dir = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    firmware_dir = os.path.join(project_dir, "firmware")

    os.makedirs(firmware_dir, exist_ok=True)

    binaries = [
        ("bootloader.bin", 0x1000),
        ("partitions.bin", 0x8000),
        ("firmware.bin",   0x10000),
    ]

    copied_any = False
    for filename, _ in binaries:
        src = os.path.join(build_dir, filename)
        dst = os.path.join(firmware_dir, filename)
        if os.path.exists(src):
            if not os.path.exists(dst) or os.path.getmtime(src) > os.path.getmtime(dst):
                shutil.copy2(src, dst)
                print(f"  [firmware] copied {filename}")
                copied_any = True
            else:
                print(f"  [firmware] {filename} already up to date")
        else:
            print(f"  [firmware] WARNING: {filename} not found at {src}")

    if copied_any:
        version = datetime.now().strftime("Build %Y-%m-%d %H:%M")
        manifest = {
            "name": "ISS Tracker (CYD 4in)",
            "version": version,
            "new_install_prompt_erase": True,
            "builds": [
                {
                    "chipFamily": "ESP32",
                    "parts": [
                        {"path": "bootloader.bin", "offset": 0x1000},
                        {"path": "partitions.bin", "offset": 0x8000},
                        {"path": "firmware.bin",   "offset": 0x10000},
                    ],
                }
            ],
        }
        manifest_path = os.path.join(firmware_dir, "manifest.json")
        with open(manifest_path, "w") as f:
            json.dump(manifest, f, indent=2)
        print(f"  [firmware] manifest.json updated ({version})")


# Fires when firmware.bin is freshly built (incremental build with changes)
env.AddPostAction("$BUILD_DIR/firmware.bin", copy_firmware_to_release)

# Also fires at the end of every build run — catches the case where PlatformIO
# skipped the rebuild (nothing changed) but firmware/ is out of sync.
def _sync_on_exit():
    copy_firmware_to_release(None, None, env)

atexit.register(_sync_on_exit)
