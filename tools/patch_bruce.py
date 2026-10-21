#!/usr/bin/env python3
"""Fuegt in Bruce unter Others den Menuepunkt 'Doom' ein und pinnt FastLED auf 3.9.x."""
import re, sys, pathlib

root = pathlib.Path(sys.argv[1])

# ---- 1) FastLED auf 3.9.x festlegen (neuere Versionen brauchen neueres C++) ----
pat = re.compile(r'^([ \t]*)(?!-)\S*fastled\S*([ \t]*@[ \t]*\S+)?[ \t]*(;.*)?$', re.I | re.M)
count = 0
inis = [root / "platformio.ini"] + sorted(root.glob("boards/**/*.ini"))
for ini in inis:
    if not ini.exists():
        continue
    t = ini.read_text(encoding="utf-8")
    new, n = pat.subn(lambda m: f"{m.group(1)}fastled/FastLED@~3.9.0", t)
    if n:
        ini.write_text(new, encoding="utf-8")
        count += n
        print(f"FastLED gepinnt in {ini}: {n} Stelle(n)")
if not count:
    print("WARNUNG: keine FastLED-Zeile gefunden. Vorkommen:")
    for ini in inis:
        if ini.exists():
            for i, line in enumerate(ini.read_text(encoding="utf-8").splitlines(), 1):
                if "fastled" in line.lower():
                    print(f"  {ini}:{i}: {line}")

# ---- 2) Menueeintrag 'Doom' ----
f = root / "src/core/menu_items/OthersMenu.cpp"
if not f.exists():
    sys.exit(f"FEHLER: {f} nicht gefunden - Bruce-Struktur hat sich geaendert.")
s = f.read_text(encoding="utf-8")
if "launchDoom" in s:
    print("schon gepatcht"); sys.exit(0)

incs = list(re.finditer(r'^#include .*$', s, re.M))
pos = incs[-1].end() if incs else 0
s = s[:pos] + "\nvoid launchDoom();\n" + s[pos:]

start = s.find("OthersMenu::optionsMenu")
if start < 0:
    sys.exit("FEHLER: OthersMenu::optionsMenu nicht gefunden.")
m = re.search(r'^([ \t]*)(addOptionToMainMenu\(\);|options\.push_back\(\{\s*"Main Menu")', s[start:], re.M)
if not m:
    sys.exit("FEHLER: Anker 'Main Menu' in OthersMenu.cpp nicht gefunden.")
ins = start + m.start()
s = s[:ins] + f'{m.group(1)}options.push_back({{"Doom", launchDoom}});\n' + s[ins:]
f.write_text(s, encoding="utf-8")
print("OthersMenu.cpp gepatcht")
