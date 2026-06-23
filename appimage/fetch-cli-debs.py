#!/usr/bin/env python3
"""
Placeholder — no external CLI packages to fetch for CS Server Manager.

The CS Server Manager relies on HLDS (Half-Life Dedicated Server) which
is installed separately by the user via Steam or SteamCMD. There is no
apt repository to pull packages from.

If a future build variant needs to bundle additional dependencies,
implement the fetching logic here and call it from build-appimage.sh.
"""

import sys

print("No external CLI packages to fetch for CS Server Manager.", file=sys.stderr)
sys.exit(0)
