#!/usr/bin/env python3
"""Fuegt in Bruce unter Others den Menuepunkt 'Doom' ein (1 Deklaration + 1 Menuezeile)."""
import re, sys, pathlib

root = pathlib.Path(sys.argv[1])
f = root / "src/core/menu_items/OthersMenu.cpp"
if not f.exists():
    sys.exit(f"FEHLER: {f} nicht gefunden - Bruce-Struktur hat sich geaendert. "
             "Siehe README, Abschnitt 'Menueeintrag manuell'.")
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
    sys.exit("FEHLER: Anker 'Main Menu' in OthersMenu.cpp nicht gefunden. "
             "Siehe README, Abschnitt 'Menueeintrag manuell'.")
ins = start + m.start()
s = s[:ins] + f'{m.group(1)}options.push_back({{"Doom", launchDoom}});\n' + s[ins:]
f.write_text(s, encoding="utf-8")
print("OthersMenu.cpp gepatcht")
